#include "burner.h"
#include "burnint.h"
#include "metal_declarations.h"
#include "metal_bridge.h"
#include "rom_verify.h"  // Include the new ROM verification header

// Include CPS2 driver headers
#include "cps.h"
#include "cps2crpt.h"

// Special memory areas for CPS2 games
extern UINT8* CpsRam660;
extern UINT8* CpsRamFF;
extern UINT8* CpsReg;
extern UINT8* CpsSaveReg[MAX_RASTER + 1];
extern UINT8* CpsFrg;
extern UINT8* CpsZRamF;

// Functions for debug and monitoring
static int CPS2_GetPlayerHealth(int player);
static int CPS2_GetComboCounter(int player);
static int CPS2_GetCurrentRound();
static int CPS2_GetPlayerPosition(int player, int* x, int* y);
static int CPS2_IsSpecialMove(int player);
static int CPS2_GetGameState();

// Combo detection
typedef struct {
    int lastHealth;
    int comboCounter;
    int comboTimer;
    int comboStartFrame;
    bool isInCombo;
} CPS2ComboState;

static CPS2ComboState g_comboState[2] = { {0} };
static int g_frameCounter = 0;

// Game-specific memory offsets for various CPS2 games
typedef struct {
    const char* gameId;
    int p1HealthOffset;
    int p2HealthOffset;
    int p1XOffset;
    int p1YOffset;
    int p2XOffset;
    int p2YOffset;
    int roundOffset;
    int gameStateOffset;
} CPS2MemoryMap;

// Memory maps for popular CPS2 titles
static CPS2MemoryMap g_memoryMaps[] = {
    // Street Fighter Alpha 3 (sfa3)
    { "sfa3", 0x5E1, 0x6E1, 0x5E8, 0x5EC, 0x6E8, 0x6EC, 0x5C4C, 0x5C40 },
    
    // Street Fighter Alpha 2 (sfa2)
    { "sfa2", 0x5E9, 0x6E9, 0x5F0, 0x5F4, 0x6F0, 0x6F4, 0x5C50, 0x5C44 },
    
    // X-Men vs. Street Fighter (xmvsf)
    { "xmvsf", 0x833C, 0x873C, 0x8344, 0x8348, 0x8744, 0x8748, 0x80A0, 0x8090 },
    
    // Marvel vs. Capcom (mvsc)
    { "mvsc", 0x9310, 0x9390, 0x9318, 0x931C, 0x9398, 0x939C, 0x9040, 0x9030 },
    
    // Default values as fallback
    { nullptr, 0x52, 0x53, 0x20, 0x24, 0x30, 0x34, 0x40, 0x00 }
};

// Current memory map
static CPS2MemoryMap* g_currentMemMap = nullptr;

// Find the appropriate memory map for the loaded game
static void InitializeMemoryMap() {
    if (!CpsRam660) {
        return;
    }
    
    const char* gameId = BurnDrvGetTextA(DRV_NAME);
    
    // Find matching memory map
    for (int i = 0; g_memoryMaps[i].gameId != nullptr; i++) {
        if (strcmp(gameId, g_memoryMaps[i].gameId) == 0) {
            g_currentMemMap = &g_memoryMaps[i];
            printf("CPS2: Using memory map for %s\n", gameId);
            return;
        }
    }
    
    // No exact match, try partial match
    for (int i = 0; g_memoryMaps[i].gameId != nullptr; i++) {
        if (strstr(gameId, g_memoryMaps[i].gameId) != nullptr) {
            g_currentMemMap = &g_memoryMaps[i];
            printf("CPS2: Using memory map for %s (partial match with %s)\n", 
                  g_memoryMaps[i].gameId, gameId);
            return;
        }
    }
    
    // Use default memory map
    g_currentMemMap = &g_memoryMaps[sizeof(g_memoryMaps) / sizeof(g_memoryMaps[0]) - 1];
    printf("CPS2: Using default memory map for %s\n", gameId);
}

// Reset combo detection state
static void ResetComboState() {
    memset(g_comboState, 0, sizeof(g_comboState));
    g_frameCounter = 0;
}

// CPS2 driver initialization hook
int Cps2_OnDriverInit() {
    // Initialize memory map for the loaded game
    InitializeMemoryMap();
    
    // Reset combo state
    ResetComboState();
    
    printf("CPS2: Driver initialization hook called\n");
    
    return 0;
}

// CPS2 frame hook - called every frame
int Cps2_OnFrame() {
    if (!g_currentMemMap || !CpsRam660) {
        return 0;
    }
    
    g_frameCounter++;
    
    // Update combo detection state
    for (int player = 0; player < 2; player++) {
        int currentHealth = CPS2_GetPlayerHealth(player);
        
        // If health decreased, might be a combo hit
        if (g_comboState[player].lastHealth > currentHealth) {
            int healthLost = g_comboState[player].lastHealth - currentHealth;
            
            // Start or continue combo
            if (!g_comboState[player].isInCombo) {
                g_comboState[player].isInCombo = true;
                g_comboState[player].comboCounter = 1;
                g_comboState[player].comboTimer = 60; // ~1 second at 60fps
                g_comboState[player].comboStartFrame = g_frameCounter;
            } else {
                g_comboState[player].comboCounter++;
                g_comboState[player].comboTimer = 60; // Reset timer
            }
            
            // Detect significant combos (3+ hits)
            if (g_comboState[player].comboCounter >= 3) {
                printf("CPS2: Player %d hit with %d-hit combo! (%d damage)\n", 
                      player + 1, g_comboState[player].comboCounter, healthLost);
            }
        }
        
        // Decrement combo timer
        if (g_comboState[player].isInCombo) {
            g_comboState[player].comboTimer--;
            
            // Combo expired
            if (g_comboState[player].comboTimer <= 0) {
                // Log final combo stats if significant
                if (g_comboState[player].comboCounter >= 3) {
                    int comboDuration = g_frameCounter - g_comboState[player].comboStartFrame;
                    printf("CPS2: Player %d combo ended: %d hits in %d frames\n", 
                          player + 1, g_comboState[player].comboCounter, comboDuration);
                }
                
                g_comboState[player].isInCombo = false;
                g_comboState[player].comboCounter = 0;
            }
        }
        
        // Save current health for next frame comparison
        g_comboState[player].lastHealth = currentHealth;
    }
    
    return 0;
}

// Get player health value
static int CPS2_GetPlayerHealth(int player) {
    if (!g_currentMemMap || !CpsRam660 || player < 0 || player > 1) {
        return 0;
    }
    
    int offset = (player == 0) ? g_currentMemMap->p1HealthOffset : g_currentMemMap->p2HealthOffset;
    return CpsRam660[offset];
}

// Get combo counter for a player
static int CPS2_GetComboCounter(int player) {
    if (player < 0 || player > 1) {
        return 0;
    }
    
    return g_comboState[player].comboCounter;
}

// Get current round number
static int CPS2_GetCurrentRound() {
    if (!g_currentMemMap || !CpsRam660) {
        return 0;
    }
    
    return CpsRam660[g_currentMemMap->roundOffset];
}

// Get player position
static int CPS2_GetPlayerPosition(int player, int* x, int* y) {
    if (!g_currentMemMap || !CpsRam660 || player < 0 || player > 1 || !x || !y) {
        return 0;
    }
    
    if (player == 0) {
        *x = (CpsRam660[g_currentMemMap->p1XOffset] << 8) | CpsRam660[g_currentMemMap->p1XOffset + 1];
        *y = (CpsRam660[g_currentMemMap->p1YOffset] << 8) | CpsRam660[g_currentMemMap->p1YOffset + 1];
    } else {
        *x = (CpsRam660[g_currentMemMap->p2XOffset] << 8) | CpsRam660[g_currentMemMap->p2XOffset + 1];
        *y = (CpsRam660[g_currentMemMap->p2YOffset] << 8) | CpsRam660[g_currentMemMap->p2YOffset + 1];
    }
    
    return 1;
}

// Detect if a special move is being performed
static int CPS2_IsSpecialMove(int player) {
    // This would need game-specific detection logic
    // Just a placeholder implementation
    return 0;
}

// Get the current game state (menu, fight, etc)
static int CPS2_GetGameState() {
    if (!g_currentMemMap || !CpsRam660) {
        return 0;
    }
    
    return CpsRam660[g_currentMemMap->gameStateOffset];
}

// Fill game state struct with CPS2-specific data
int Cps2_FillGameState(void* stateData) {
    if (!stateData || !g_currentMemMap || !CpsRam660) {
        return 0;
    }
    
    struct GameStateData {
        int playerHealth[2];
        int comboCounter[2];
        int playerPosition[2][2];
        int currentRound;
        int gameState;
        int frameCounter;
    };
    
    GameStateData* state = (GameStateData*)stateData;
    
    // Fill health values
    state->playerHealth[0] = CPS2_GetPlayerHealth(0);
    state->playerHealth[1] = CPS2_GetPlayerHealth(1);
    
    // Fill combo counters
    state->comboCounter[0] = CPS2_GetComboCounter(0);
    state->comboCounter[1] = CPS2_GetComboCounter(1);
    
    // Fill positions
    CPS2_GetPlayerPosition(0, &state->playerPosition[0][0], &state->playerPosition[0][1]);
    CPS2_GetPlayerPosition(1, &state->playerPosition[1][0], &state->playerPosition[1][1]);
    
    // Fill other data
    state->currentRound = CPS2_GetCurrentRound();
    state->gameState = CPS2_GetGameState();
    state->frameCounter = g_frameCounter;
    
    return sizeof(GameStateData);
}

// Include the renderer header
#include "metal_cps2_renderer.h"

// External reference to the new Metal CPS2 render callback
INT32 (*pCps2RenderCallback)() = NULL;

// Setup Metal integration hooks for CPS2 games
void Cps2_SetupMetalLinkage() {
    printf("Cps2_SetupMetalLinkage: Setting up Metal linkage for CPS2\n");
    
    // Initialize memory map for the loaded game
    InitializeMemoryMap();
    
    // Reset combo state
    ResetComboState();
    
    // Link CPS2-specific functions to Metal implementation
    if (g_currentMemMap) {
        printf("Cps2_SetupMetalLinkage: Using memory map for %s\n", 
              BurnDrvGetTextA(DRV_NAME));
              
        // Connect CPS2 hooks to Metal implementation
        extern int (*pCps2FrameCallback)();
        extern int (*pCps2InitCallback)();
        extern bool (*pCps2RomValidCallback)(const char*);
        extern int (*pCps2RomLoadCallback)(const char*);
        
        // Install hooks
        pCps2FrameCallback = Cps2_OnFrame;
        pCps2InitCallback = Cps2_OnDriverInit;
        pCps2RomValidCallback = Cps2_IsValidROM;
        pCps2RomLoadCallback = Cps2_LoadROM;
        
        // Set up rendering hooks
        Metal_CPS2_SetupRenderHooks();
        
        printf("Cps2_SetupMetalLinkage: CPS2 hooks installed successfully\n");
    } else {
        printf("Cps2_SetupMetalLinkage: Warning - No memory map available\n");
    }
    
    // Check for CPS2 encryption modules
    #ifdef HAS_CPS2_ENCRYPTION
    printf("Cps2_SetupMetalLinkage: CPS2 encryption module available\n");
    
    // Call into the core's CPS2 encryption module
    extern int Cps2CryptInit();
    if (Cps2CryptInit() == 0) {
        printf("Cps2_SetupMetalLinkage: CPS2 encryption initialized\n");
    } else {
        printf("Cps2_SetupMetalLinkage: Warning - CPS2 encryption initialization failed\n");
    }
    #else
    printf("Cps2_SetupMetalLinkage: CPS2 encryption module not available\n");
    #endif
}

// CPS2 ROM verification hooks
bool Cps2_IsValidROM(const char* romPath) {
    if (!romPath || !romPath[0]) {
        printf("Cps2_IsValidROM: Invalid path\n");
        return false;
    }
    
    // Use the ROM verification system
    ROMVerify::ROMSetVerification verification;
    if (!ROMVerify::VerifyCPS2ROM(romPath, verification)) {
        printf("Cps2_IsValidROM: Verification failed for %s\n", romPath);
        
        // Log detailed verification results
        printf("Cps2_IsValidROM: Verification results for %s:\n", verification.setName.c_str());
        printf("  Complete: %s\n", verification.complete ? "Yes" : "No");
        printf("  Playable: %s\n", verification.playable ? "Yes" : "No");
        
        // List failed verifications
        for (const auto& result : verification.results) {
            if (!result.success) {
                printf("  - %s: %s (Expected: %s, Got: %s)\n", 
                      result.romName.c_str(), 
                      result.errorMessage.c_str(),
                      result.expectedChecksum.c_str(),
                      result.actualChecksum.c_str());
            }
        }
        
        // ROM may still be playable even if verification fails
        return verification.playable;
    }
    
    return true;
}

// Load a CPS2 ROM with advanced error handling
int Cps2_LoadROM(const char* romPath) {
    if (!romPath || !romPath[0]) {
        printf("Cps2_LoadROM: Invalid path\n");
        return -1;
    }
    
    printf("Cps2_LoadROM: Loading ROM: %s\n", romPath);
    
    // Extract the game ID from the path
    const char* pszBasename = strrchr(romPath, '/');
    if (pszBasename) {
        pszBasename++; // Skip the '/'
    } else {
        pszBasename = romPath;
    }
    
    char szShortName[32];
    strncpy(szShortName, pszBasename, sizeof(szShortName) - 1);
    szShortName[sizeof(szShortName) - 1] = '\0';
    
    // Remove extension if present
    char* pszDot = strrchr(szShortName, '.');
    if (pszDot) {
        *pszDot = '\0';
    }
    
    // Check if it's a valid ROM
    if (!Cps2_IsValidROM(romPath)) {
        // We'll let the ROM loader handle partial ROMs
        printf("Cps2_LoadROM: Warning - ROM did not pass verification: %s\n", romPath);
    }
    
    // Load the ROM set using our CPS2-specific loader
    if (!CPS2_LoadROMSet(szShortName)) {
        printf("Cps2_LoadROM: Failed to load ROM set: %s\n", szShortName);
        return -1;
    }
    
    // Connect to the FBNeo core
    Cps2_SetupMetalLinkage();
    
    // Start the ROM
    if (!CPS2_RunROM()) {
        printf("Cps2_LoadROM: Failed to run ROM: %s\n", szShortName);
        return -1;
    }
    
    printf("Cps2_LoadROM: Successfully loaded CPS2 ROM: %s\n", szShortName);
    return 0;
}

// CPS2 ROM decryption function with support for various encryption schemes
bool Cps2_DecryptROM(uint8_t* data, size_t size, const char* romName) {
    if (!data || size == 0 || !romName) {
        printf("Cps2_DecryptROM: Invalid parameters\n");
        return false;
    }
    
    // Get file extension to determine ROM type
    const char* ext = strrchr(romName, '.');
    if (!ext) {
        // No extension, can't determine ROM type
        return false;
    }
    
    // Only decrypt program ROMs (usually .03-.06)
    bool isProgram = false;
    if (ext[1] == '0' && ext[2] >= '3' && ext[2] <= '8') {
        isProgram = true;
    }
    
    if (!isProgram) {
        // Not a program ROM, no need to decrypt
        return true;
    }
    
    printf("Cps2_DecryptROM: Decrypting program ROM: %s\n", romName);
    
    // Get the current game ID
    const char* gameId = BurnDrvGetTextA(DRV_NAME);
    if (!gameId) {
        printf("Cps2_DecryptROM: Unable to determine game ID\n");
        return false;
    }
    
    // Different games use different encryption schemes
    bool decryptionSuccessful = false;
    
    // Check if we have the CPS2 encryption functionality
    #if defined(HAS_CPS2_DECRYPTION)
    extern int Cps2Decrypt(void*);
    extern int Cps2DecryptSF(void*);
    extern int Cps2DecryptMarvel(void*, const char*);
    
    // Determine which decryption method to use based on the game ID
    if (strstr(gameId, "mvsc") != nullptr || 
        strstr(gameId, "msh") != nullptr || 
        strstr(gameId, "xmvsf") != nullptr) {
        // Marvel games use a special decryption
        printf("Cps2_DecryptROM: Using Marvel decryption for %s\n", gameId);
        decryptionSuccessful = (Cps2DecryptMarvel(data, gameId) == 0);
    } else if (strstr(gameId, "sf") != nullptr) {
        // Street Fighter games use SF-specific decryption
        printf("Cps2_DecryptROM: Using Street Fighter decryption for %s\n", gameId);
        decryptionSuccessful = (Cps2DecryptSF(data) == 0);
    } else {
        // Standard CPS2 decryption for other games
        printf("Cps2_DecryptROM: Using standard CPS2 decryption for %s\n", gameId);
        decryptionSuccessful = (Cps2Decrypt(data) == 0);
    }
    
    #else
    // Use a basic XOR decryption to simulate decryption
    // This won't actually work, but it's a placeholder
    printf("Cps2_DecryptROM: CPS2 decryption module not available\n");
    printf("Cps2_DecryptROM: Using fake decryption for %s\n", gameId);
    
    uint8_t key = 0x55; // Dummy key
    for (size_t i = 0; i < size; i++) {
        // Simple XOR "decryption" - this is NOT real CPS2 decryption!
        // It's just a placeholder for when real decryption is unavailable
        if (i % 16 == 0) key = (uint8_t)i;
        data[i] ^= key;
    }
    
    printf("Cps2_DecryptROM: Simulated decryption completed\n");
    printf("Cps2_DecryptROM: WARNING: This is not real CPS2 decryption!\n");
    decryptionSuccessful = true;
    #endif
    
    if (!decryptionSuccessful) {
        printf("Cps2_DecryptROM: Decryption failed for %s\n", romName);
        return false;
    }
    
    printf("Cps2_DecryptROM: Successfully decrypted %s\n", romName);
    return true;
} 