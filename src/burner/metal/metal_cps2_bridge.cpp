// CPS2 Bridge for Metal implementation
// This file connects the Metal frontend with the CPS2 emulation core

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

// Basic type definitions (if not already defined)
#ifndef TCHAR_DEFINED
typedef char TCHAR;
#define TCHAR_DEFINED
#endif

// Include external dependencies with care to avoid conflicts
extern "C" {
    // Declare external variables from FBNeo core
    extern int nBurnDrvActive;
    extern unsigned char* pBurnDraw;
    extern int nBurnPitch;
    extern int nBurnBpp;
    extern int nBurnSoundLen;
    extern int nBurnSoundRate;
    extern short* pBurnSoundOut;
    
    // CPS input variables that will be needed for CPS2 games
    extern unsigned char CpsInp000[8];
    extern unsigned char CpsInp001[8];
    extern unsigned char CpsInp010[8];
    extern unsigned char CpsInp011[8];
    extern unsigned char CpsInp018[8];
    extern unsigned char CpsInp020[8];
    extern unsigned char CpsInp021[8];
    extern unsigned char CpsInp119[8];
    extern unsigned char CpsReset;
    
    // External functions from FBNeo core
    int BurnDrvGetVisibleSize(int* pnWidth, int* pnHeight);
    int BurnDrvGetHardwareCode();
    int BurnDrvGetFlags();
    int BurnDrvInit();
    int BurnDrvExit();
    int BurnDrvFrame();
    int Cps2Init();
    int CpsExit();
    int BurnLoadRom(unsigned char* Dest, int* pnWrote, int i);
    struct BurnRomInfo;
    int BurnDrvGetRomInfo(struct BurnRomInfo* pri, unsigned int i);
    const char* BurnDrvGetTextA(unsigned int i);
}

// CPS2 specific implementation
namespace CPS2Metal {

// Configuration and state variables
static bool g_bCPS2Initialized = false;
static bool g_bInGame = false;
static int g_nGameWidth = 384;
static int g_nGameHeight = 224;
static int g_nFrameCount = 0;

// Memory management
static unsigned char* g_pCpsRom = nullptr;      // Main 68K program ROM
static unsigned char* g_pCpsGfx = nullptr;      // Graphics ROM
static unsigned char* g_pCpsZ80Rom = nullptr;   // Z80 program ROM
static unsigned char* g_pCpsQSnd = nullptr;     // QSound samples
static unsigned char* g_pCpsKey = nullptr;      // CPS2 encryption key

// Initialize CPS2 system
int InitCPS2System() {
    printf("[CPS2Metal] Initializing CPS2 system\n");
    
    if (g_bCPS2Initialized) {
        printf("[CPS2Metal] CPS2 already initialized\n");
        return 0;
    }
    
    // Set hardware configuration
    extern int Cps;  // From cps.h, 1=CPS1, 2=CPS2
    Cps = 2;         // We're focusing on CPS2 emulation
    
    // Initialize CPS2 core
    int result = Cps2Init();
    if (result != 0) {
        printf("[CPS2Metal] Failed to initialize CPS2 core: %d\n", result);
        return result;
    }
    
    g_bCPS2Initialized = true;
    printf("[CPS2Metal] CPS2 system initialized successfully\n");
    return 0;
}

// Exit CPS2 system
int ExitCPS2System() {
    printf("[CPS2Metal] Shutting down CPS2 system\n");
    
    if (!g_bCPS2Initialized) {
        printf("[CPS2Metal] CPS2 not initialized\n");
        return 0;
    }
    
    // Clean up allocated memory
    FreeMemory();
    
    // Exit CPS core
    int result = CpsExit();
    if (result != 0) {
        printf("[CPS2Metal] Failed to exit CPS core: %d\n", result);
        return result;
    }
    
    g_bCPS2Initialized = false;
    g_bInGame = false;
    printf("[CPS2Metal] CPS2 system shut down successfully\n");
    return 0;
}

// Allocate memory for CPS2 ROMs
int AllocateMemory(int romSize, int gfxSize, int z80Size, int qsndSize) {
    printf("[CPS2Metal] Allocating memory for CPS2 ROMs\n");
    
    // Free any existing memory first
    FreeMemory();
    
    // Allocate memory for ROMs
    if (romSize > 0) {
        g_pCpsRom = (unsigned char*)malloc(romSize);
        if (!g_pCpsRom) {
            printf("[CPS2Metal] Failed to allocate CPS ROM memory (%d bytes)\n", romSize);
            FreeMemory();
            return 1;
        }
        memset(g_pCpsRom, 0, romSize);
        printf("[CPS2Metal] Allocated %d bytes for CPS ROM\n", romSize);
    }
    
    if (gfxSize > 0) {
        g_pCpsGfx = (unsigned char*)malloc(gfxSize);
        if (!g_pCpsGfx) {
            printf("[CPS2Metal] Failed to allocate CPS GFX memory (%d bytes)\n", gfxSize);
            FreeMemory();
            return 1;
        }
        memset(g_pCpsGfx, 0, gfxSize);
        printf("[CPS2Metal] Allocated %d bytes for CPS GFX\n", gfxSize);
    }
    
    if (z80Size > 0) {
        g_pCpsZ80Rom = (unsigned char*)malloc(z80Size);
        if (!g_pCpsZ80Rom) {
            printf("[CPS2Metal] Failed to allocate Z80 ROM memory (%d bytes)\n", z80Size);
            FreeMemory();
            return 1;
        }
        memset(g_pCpsZ80Rom, 0, z80Size);
        printf("[CPS2Metal] Allocated %d bytes for Z80 ROM\n", z80Size);
    }
    
    if (qsndSize > 0) {
        g_pCpsQSnd = (unsigned char*)malloc(qsndSize);
        if (!g_pCpsQSnd) {
            printf("[CPS2Metal] Failed to allocate QSound memory (%d bytes)\n", qsndSize);
            FreeMemory();
            return 1;
        }
        memset(g_pCpsQSnd, 0, qsndSize);
        printf("[CPS2Metal] Allocated %d bytes for QSound samples\n", qsndSize);
    }
    
    // Allocate memory for CPS2 key (if needed)
    g_pCpsKey = (unsigned char*)malloc(1024);  // Small buffer for decryption key
    if (!g_pCpsKey) {
        printf("[CPS2Metal] Failed to allocate CPS2 key memory\n");
        FreeMemory();
        return 1;
    }
    memset(g_pCpsKey, 0, 1024);
    
    return 0;
}

// Free allocated memory
void FreeMemory() {
    if (g_pCpsRom) {
        free(g_pCpsRom);
        g_pCpsRom = nullptr;
    }
    
    if (g_pCpsGfx) {
        free(g_pCpsGfx);
        g_pCpsGfx = nullptr;
    }
    
    if (g_pCpsZ80Rom) {
        free(g_pCpsZ80Rom);
        g_pCpsZ80Rom = nullptr;
    }
    
    if (g_pCpsQSnd) {
        free(g_pCpsQSnd);
        g_pCpsQSnd = nullptr;
    }
    
    if (g_pCpsKey) {
        free(g_pCpsKey);
        g_pCpsKey = nullptr;
    }
}

// Load a CPS2 game
int LoadCPS2Game(int gameIndex) {
    printf("[CPS2Metal] Loading CPS2 game (index %d)\n", gameIndex);
    
    if (!g_bCPS2Initialized) {
        printf("[CPS2Metal] CPS2 system not initialized\n");
        return 1;
    }
    
    if (g_bInGame) {
        // Exit current game first
        ExitCPS2Game();
    }
    
    // Select the game
    extern int BurnDrvSelect(int nDrvNum);
    int result = BurnDrvSelect(gameIndex);
    if (result != 0) {
        printf("[CPS2Metal] Failed to select game: %d\n", result);
        return result;
    }
    
    // Get game dimensions
    BurnDrvGetVisibleSize(&g_nGameWidth, &g_nGameHeight);
    printf("[CPS2Metal] Game dimensions: %dx%d\n", g_nGameWidth, g_nGameHeight);
    
    // Initialize the driver
    result = BurnDrvInit();
    if (result != 0) {
        printf("[CPS2Metal] Failed to initialize game: %d\n", result);
        return result;
    }
    
    g_bInGame = true;
    g_nFrameCount = 0;
    printf("[CPS2Metal] Game loaded successfully\n");
    return 0;
}

// Exit the current CPS2 game
int ExitCPS2Game() {
    printf("[CPS2Metal] Exiting CPS2 game\n");
    
    if (!g_bInGame) {
        printf("[CPS2Metal] No game is currently running\n");
        return 0;
    }
    
    // Exit the driver
    int result = BurnDrvExit();
    if (result != 0) {
        printf("[CPS2Metal] Failed to exit game: %d\n", result);
        return result;
    }
    
    g_bInGame = false;
    printf("[CPS2Metal] Game exited successfully\n");
    return 0;
}

// Run a single frame of the CPS2 game
int RunCPS2Frame(bool render) {
    if (!g_bInGame) {
        printf("[CPS2Metal] No game is currently running\n");
        return 1;
    }
    
    // Set rendering flag
    if (render) {
        extern unsigned char* pBurnDraw;
        if (!pBurnDraw) {
            printf("[CPS2Metal] Warning: pBurnDraw is NULL, rendering will be skipped\n");
        }
    } else {
        extern unsigned char* pBurnDraw;
        pBurnDraw = nullptr;  // Disable rendering for this frame
    }
    
    // Run the frame
    int result = BurnDrvFrame();
    if (result != 0) {
        printf("[CPS2Metal] Error running frame: %d\n", result);
        return result;
    }
    
    g_nFrameCount++;
    
    // Print occasional debug info
    if (g_nFrameCount % 60 == 0) {
        printf("[CPS2Metal] Frame %d completed\n", g_nFrameCount);
    }
    
    return 0;
}

// Get the current frame count
int GetFrameCount() {
    return g_nFrameCount;
}

// Get game dimensions
void GetGameDimensions(int& width, int& height) {
    width = g_nGameWidth;
    height = g_nGameHeight;
}

// Process input for CPS2 games
void ProcessInput(unsigned char* inputData, int size) {
    if (!g_bInGame || !inputData || size < 8) {
        return;
    }
    
    // Map the input data to CPS2 input variables
    memcpy(CpsInp000, inputData, 8);
    memcpy(CpsInp001, inputData + 8, 8);
    
    if (size >= 32) {
        memcpy(CpsInp010, inputData + 16, 8);
        memcpy(CpsInp011, inputData + 24, 8);
    }
    
    if (size >= 48) {
        memcpy(CpsInp018, inputData + 32, 8);
        memcpy(CpsInp020, inputData + 40, 8);
    }
    
    if (size >= 56) {
        memcpy(CpsInp021, inputData + 48, 8);
    }
    
    // Process reset button
    CpsReset = inputData[size - 1] & 0x01;
}

} // namespace CPS2Metal

// External C API for Metal frontend to call
extern "C" {

// Initialize CPS2 system
int Metal_CPS2_Init() {
    return CPS2Metal::InitCPS2System();
}

// Exit CPS2 system
int Metal_CPS2_Exit() {
    return CPS2Metal::ExitCPS2System();
}

// Load a CPS2 game
int Metal_CPS2_LoadGame(int gameIndex) {
    return CPS2Metal::LoadCPS2Game(gameIndex);
}

// Exit the current CPS2 game
int Metal_CPS2_ExitGame() {
    return CPS2Metal::ExitCPS2Game();
}

// Run a single frame of the CPS2 game
int Metal_CPS2_RunFrame(int render) {
    return CPS2Metal::RunCPS2Frame(render != 0);
}

// Get the current frame count
int Metal_CPS2_GetFrameCount() {
    return CPS2Metal::GetFrameCount();
}

// Get game dimensions
void Metal_CPS2_GetGameDimensions(int* width, int* height) {
    if (width && height) {
        CPS2Metal::GetGameDimensions(*width, *height);
    }
}

// Process input for CPS2 games
void Metal_CPS2_ProcessInput(unsigned char* inputData, int size) {
    CPS2Metal::ProcessInput(inputData, size);
}

// Allocate memory for CPS2 ROMs
int Metal_CPS2_AllocateMemory(int romSize, int gfxSize, int z80Size, int qsndSize) {
    return CPS2Metal::AllocateMemory(romSize, gfxSize, z80Size, qsndSize);
}

// Free CPS2 memory
void Metal_CPS2_FreeMemory() {
    CPS2Metal::FreeMemory();
}

// Stub for the CPS variable
INT32 Cps = 2;  // Set to 2 for CPS2

// Stub for QSound disabling
INT32 Cps2DisableQSnd = 0;

// Metal verification stub
void Metal_VerifyCps2Emulation(int frameCount) {
    printf("[CPS2Metal] Verification at frame %d\n", frameCount);
}

// Stubs for missing Metal_SaveState functions
INT32 Metal_InitSaveState() { return 0; }
INT32 Metal_ExitSaveState() { return 0; }
INT32 Metal_QuickSave() { return 0; }
INT32 Metal_QuickLoad() { return 0; }
INT32 Metal_SaveState(int slot) { return 0; }
INT32 Metal_LoadState(int slot) { return 0; }
INT32 Metal_GetCurrentSaveSlot() { return 0; }
INT32 Metal_GetSaveStateStatus(int slot) { return 0; }

// Stubs for BurnLib functions
INT32 BurnLibInit_Metal() { return 0; }
INT32 BurnLibExit_Metal() { return 0; }

} // extern "C" 