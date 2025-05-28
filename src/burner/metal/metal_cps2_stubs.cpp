#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>  // For sqrt function
#include <time.h>
#include <sys/time.h>

// Define basic types if not already defined
#ifndef INT32
typedef int INT32;
#endif

#ifndef UINT8
typedef uint8_t UINT8;
#endif

#ifndef UINT16
typedef uint16_t UINT16;
#endif

#ifndef UINT32
typedef uint32_t UINT32;
#endif

// Forward declarations for FBNeo core functions
extern "C" {
    // Basic FBNeo functions
    INT32 BurnDrvInit();
    INT32 BurnDrvExit();
    INT32 BurnDrvFrame();
    INT32 BurnDrvFind(const char* szName);
    INT32 BurnDrvSelect(INT32 nDrvNum);
    
    // Real CPS2 functions to be connected
    INT32 Cps2Init();
    INT32 Cps2Frame();
    void CpsRedraw();
    
    // Globals from burn.h that we need
    extern UINT8* pBurnDraw;
    extern INT32 nBurnPitch;
    extern INT32 nBurnBpp;
    
    // Declare input variables
    extern UINT8 CpsReset;
    extern UINT8 CpsInp000[8];
    extern UINT8 CpsInp001[8];
    extern UINT8 CpsInp010[8];
    extern UINT8 CpsInp011[8];
    extern UINT8 CpsInp018[8];
    extern UINT8 CpsInp020[8];
    extern UINT8 CpsInp021[8];
}

#ifdef __cplusplus
extern "C" {
#endif

// CPS global variables
INT32 Cps = 2;  // Set to 2 for CPS2
INT32 Cps2DisableQSnd = 0;

// Game variables for Metal implementation
static bool g_bDriverInitialized = false;
static bool g_bGameInitialized = false;
static int g_nCurrentGame = -1;
static int g_nFrameCounter = 0;

// Forward declarations for test pattern
static void GenerateTestPattern();
static bool g_bFrameBufferUpdated = false;

// Function to get current timestamp in microseconds
static uint64_t GetMicrosecondTimestamp() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

// Metal CPS2 system implementation
INT32 Metal_CPS2_Init() {
    printf("[Metal_CPS2_Init] Initializing CPS2 system\n");
    
    // Initialize CPS2 system
    g_bDriverInitialized = true;
    
    return 0;
}

INT32 Metal_CPS2_Exit() {
    printf("[Metal_CPS2_Exit] Shutting down CPS2 system\n");
    
    if (g_bGameInitialized) {
        // Exit the current game if one is running
        BurnDrvExit();
        g_bGameInitialized = false;
    }
    
    g_bDriverInitialized = false;
    return 0;
}

// Add forward declaration for the ROM loading function
extern "C" INT32 Metal_LoadCPS2ROMs(const char* romPath, int gameIndex);

// Then update the Metal_CPS2_LoadGame function to call it

INT32 Metal_CPS2_LoadGame(int gameIndex) {
    printf("[Metal_CPS2_LoadGame] Loading game index %d\n", gameIndex);
    
    if (!g_bDriverInitialized) {
        printf("[Metal_CPS2_LoadGame] ERROR: CPS2 system not initialized\n");
        return 1;
    }
    
    // If a game is already running, exit it first
    if (g_bGameInitialized) {
        BurnDrvExit();
        g_bGameInitialized = false;
    }
    
    // Select the game by index
    // For now, we only have mvsc (index 0)
    if (gameIndex != 0) {
        printf("[Metal_CPS2_LoadGame] ERROR: Only mvsc (index 0) is supported\n");
        return 1;
    }
    
    // Get the current ROM path
    extern const char* Metal_GetCurrentROMPath();
    const char* romPath = Metal_GetCurrentROMPath();
    
    if (!romPath || romPath[0] == '\0') {
        printf("[Metal_CPS2_LoadGame] ERROR: No ROM path specified\n");
        return 1;
    }
    
    // Load ROMs
    INT32 nRet = Metal_LoadCPS2ROMs(romPath, gameIndex);
    if (nRet != 0) {
        printf("[Metal_CPS2_LoadGame] ERROR: Failed to load ROMs: %d\n", nRet);
        return nRet;
    }
    
    // Find and select Marvel vs. Capcom
    int nDrvIndex = BurnDrvFind("mvsc");
    if (nDrvIndex < 0) {
        printf("[Metal_CPS2_LoadGame] ERROR: Could not find game 'mvsc'\n");
        return 1;
    }
    
    BurnDrvSelect(nDrvIndex);
    
    // Initialize the driver
    nRet = BurnDrvInit();
    if (nRet != 0) {
        printf("[Metal_CPS2_LoadGame] ERROR: BurnDrvInit failed with code %d\n", nRet);
        return nRet;
    }
    
    g_bGameInitialized = true;
    g_nCurrentGame = gameIndex;
    g_nFrameCounter = 0;
    
    printf("[Metal_CPS2_LoadGame] Game loaded successfully\n");
    
    return 0;
}

INT32 Metal_CPS2_ExitGame() {
    printf("[Metal_CPS2_ExitGame] Exiting current game\n");
    
    if (!g_bGameInitialized) {
        printf("[Metal_CPS2_ExitGame] WARNING: No game is currently running\n");
        return 0;
    }
    
    // Exit the driver
    INT32 nRet = BurnDrvExit();
    
    g_bGameInitialized = false;
    g_nCurrentGame = -1;
    
    printf("[Metal_CPS2_ExitGame] Game exited with code %d\n", nRet);
    
    return nRet;
}

// Update the Metal_RunFrame function to run one frame of CPS2 emulation
INT32 Metal_RunFrame(INT32 bDraw) {
    if (!g_bDriverInitialized) {
        printf("[Metal_RunFrame] ERROR: CPS2 system not initialized\n");
        return 1;
    }
    
    if (!g_bGameInitialized) {
        // No game loaded, generate a test pattern
        if (bDraw && pBurnDraw) {
            // If we're in draw mode, create a test pattern
            GenerateTestPattern();
            
            // Mark frame buffer as updated
            g_bFrameBufferUpdated = true;
            g_nFrameCounter++;
            
            // Log every 60 frames
            if (g_nFrameCounter % 60 == 0) {
                printf("[Metal_RunFrame] Test pattern generated, frame %d\n", g_nFrameCounter);
            }
            
            return 0;
        }
        
        return 0;
    }
    
    // If a real game is loaded, run actual emulation
    
    // Set drawing mode
    extern INT32 nBurnLayer;
    INT32 oldLayer = nBurnLayer;
    
    if (bDraw) {
        // Enable all layers
        nBurnLayer = 0xFF;
    } else {
        // Disable visual layers for speed
        nBurnLayer = 0;
    }
    
    // Run one frame of emulation
    INT32 nRet = 0;
    
    // Track timing
    static uint64_t frameStartTime = 0;
    uint64_t currentTime = GetMicrosecondTimestamp();
    
    if (frameStartTime == 0) {
        frameStartTime = currentTime;
    }
    
    // Run the emulation frame
    nRet = Cps2Frame();
    
    // Calculate frame time
    uint64_t frameEndTime = GetMicrosecondTimestamp();
    uint64_t frameTime = frameEndTime - frameStartTime;
    frameStartTime = frameEndTime;
    
    // Log performance every 60 frames
    g_nFrameCounter++;
    if (g_nFrameCounter % 60 == 0) {
        printf("[Metal_RunFrame] Frame %d time: %llu µs (%.2f FPS)\n", 
               g_nFrameCounter, frameTime, 1000000.0f / frameTime);
    }
    
    // Restore drawing settings
    nBurnLayer = oldLayer;
    
    // Mark frame buffer as updated
    g_bFrameBufferUpdated = true;
    
    return nRet;
}

// Get the current frame buffer
void* Metal_GetFrameBuffer() {
    if (!g_bGameInitialized && !g_bFrameBufferUpdated) {
        // Generate a test pattern if no game is loaded
        GenerateTestPattern();
        g_bFrameBufferUpdated = true;
    }
    
    return pBurnDraw;
}

INT32 Metal_CPS2_GetFrameCount() {
    return g_nFrameCounter;
}

void Metal_CPS2_GetGameDimensions(int* width, int* height) {
    // CPS2 games have a standard resolution of 384x224
    if (width) *width = 384;
    if (height) *height = 224;
}

void Metal_CPS2_ProcessInput(unsigned char* inputData, int size) {
    // Process input data
    if (!inputData || size <= 0) {
        return;
    }
    
    // TODO: Map input data to CPS input variables
    // This is a simplified implementation that will need to be expanded
    
    // Process reset input
    CpsReset = (inputData[0] & 0x01) ? 1 : 0;
    
    // Process player 1 inputs
    if (size > 1) {
        CpsInp000[0] = inputData[1];
    }
    
    // Process player 2 inputs
    if (size > 2) {
        CpsInp000[1] = inputData[2];
    }
}

// CPS2 memory allocation stub - real implementation would allocate memory for ROMs, etc.
INT32 Metal_CPS2_AllocateMemory(int romSize, int gfxSize, int z80Size, int qsndSize) {
    printf("[Metal_CPS2_AllocateMemory] Stub implementation\n");
    printf("  ROM size: %d bytes\n", romSize);
    printf("  GFX size: %d bytes\n", gfxSize);
    printf("  Z80 size: %d bytes\n", z80Size);
    printf("  QSound size: %d bytes\n", qsndSize);
    
    // Memory allocation is handled by BurnDrvInit in the real implementation
    return 0;
}

void Metal_CPS2_FreeMemory() {
    printf("[Metal_CPS2_FreeMemory] Stub implementation\n");
    // Memory deallocation is handled by BurnDrvExit in the real implementation
}

// Metal verification function for CPS2 emulation
void Metal_VerifyCps2Emulation(int frameCount) {
    printf("[Metal_VerifyCps2Emulation] Frame %d: Verifying CPS2 emulation\n", frameCount);
    
    // Log the current status of the CPS2 emulation
    printf("  Driver initialized: %s\n", g_bDriverInitialized ? "Yes" : "No");
    printf("  Game initialized: %s\n", g_bGameInitialized ? "Yes" : "No");
    printf("  Current game index: %d\n", g_nCurrentGame);
    printf("  Frame counter: %d\n", g_nFrameCounter);
    
    // Check if frame buffer is available for drawing
    printf("  Frame buffer: %p\n", pBurnDraw);
    if (pBurnDraw) {
        printf("  Frame pitch: %d\n", nBurnPitch);
        printf("  Bits per pixel: %d\n", nBurnBpp);
        
        // Sample a few pixels to verify content
        UINT32* pixels = (UINT32*)pBurnDraw;
        printf("  Sample pixels:");
        for (int i = 0; i < 5 && i < (384 * 224); i++) {
            printf(" 0x%08X", pixels[i]);
        }
        printf("\n");
    }
}

// BurnLib stubs - forward to the implementations in metal_bridge_simple.cpp
extern INT32 BurnLibInit_Metal();
extern INT32 BurnLibExit_Metal();

// ROM validation stubs are defined in metal_rom_validation.cpp
extern INT32 Metal_ValidateROMFile(const char*);
extern INT32 FindCps2Rom(const char*);

// ======= Direct stubs to real functions =======

// Map our mvscInit to the real driver's Cps2Init
extern "C" INT32 mvscInit() {
    printf("[mvscInit] Calling real Cps2Init\n");
    return Cps2Init();
}

// Map our mvscFrame to the real driver's Cps2Frame
extern "C" INT32 mvscFrame() {
    static int frameCount = 0;
    frameCount++;
    
    // Log every 60 frames
    if (frameCount % 60 == 0) {
        printf("[mvscFrame] Frame %d\n", frameCount);
    }
    
    return BurnDrvFrame();
}

// Map our mvscExit to the real driver's DrvExit
extern "C" INT32 mvscExit() {
    printf("[mvscExit] Exiting CPS2 game\n");
    return BurnDrvExit();
}

// Z80 and QSound stubs (these would need real implementations in the future)
extern "C" void CZetOpen(INT32 nCPU) {}
extern "C" void CZetClose() {}
extern "C" INT32 CZetRun(INT32 nCycles) { return 0; }
extern "C" void CZetSetIRQLine(INT32 nIRQLine, INT32 nStatus) {}
extern "C" INT32 CZetNmi() { return 0; }
extern "C" INT32 CZetReset() { return 0; }
extern "C" UINT8 CZetRead(UINT16 address) { return 0; }
extern "C" void CZetWrite(UINT16 address, UINT8 data) {}

// Stub for Metal ROM validation
extern "C" INT32 Metal_ValidateROM(const char* path) {
    // In a real implementation, we would validate ROM checksums
    printf("[Metal_ValidateROM] %s\n", path);
    
    // Just check if the file exists
    FILE* f = fopen(path, "rb");
    if (f) {
        fclose(f);
        printf("ROM file exists: %s\n", path);
        return 0; // Success
    }
    
    printf("ROM file not found: %s\n", path);
    return 1; // Error
}

// Function to help find a matching CPS2 ROM
extern "C" INT32 Metal_FindCPS2ROM(const char* path) {
    printf("Finding CPS2 ROM match for: %s\n", path);
    
    // In a real implementation, we would scan the ZIP and identify the game
    
    // Check if this is a known CPS2 ROM
    const char* filename = strrchr(path, '/');
    if (filename) filename++; else filename = path;
    
    if (strstr(filename, "mvsc") || strstr(filename, "mvc")) {
        printf("Found matching CPS2 game: mvsc (index 0)\n");
        return 0; // mvsc index
    } else if (strstr(filename, "sf") || strstr(filename, "street")) {
        printf("Found matching CPS2 game: sf (index 1)\n");
        return 1; // sf index
    }
    
    // Default to mvsc for testing
    printf("No exact match found, defaulting to mvsc (index 0)\n");
    return 0;
}

// Generate a test pattern
static void GenerateTestPattern() {
    if (!pBurnDraw) {
        printf("GenerateTestPattern: ERROR - pBurnDraw is NULL\n");
        return;
    }
    
    int width = 384;  // Standard CPS2 width
    int height = 224; // Standard CPS2 height
    
    UINT32* dst32 = (UINT32*)pBurnDraw;
    
    // Create a checkered pattern with blue "CPS2" logo
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            UINT32 color = 0;
            
            // Create a grid pattern as background
            int squareX = (x / 16) % 2;
            int squareY = (y / 16) % 2;
            
            if ((squareX == 0 && squareY == 0) || (squareX == 1 && squareY == 1)) {
                // Dark gray
                color = 0xFF333333;
            } else {
                // Light gray
                color = 0xFF666666;
            }
            
            // Add animated moving stripes
            if ((x + g_nFrameCounter) % 64 < 32) {
                color = (color & 0xFF7F7F7F) | ((g_nFrameCounter % 256) << 16); // Add some red
            }
            
            // Draw CPS2 logo if driver is initialized
            if (g_bDriverInitialized) {
                // Define CPS2 logo in the center of the screen
                int centerX = width / 2;
                int centerY = height / 2;
                
                // "CPS2" text - simple pixel font
                bool inLogo = false;
                
                // 'C'
                if (x >= centerX - 60 && x < centerX - 40 && 
                    ((y >= centerY - 20 && y < centerY - 15) ||  // Top bar
                     (y >= centerY + 15 && y < centerY + 20) ||  // Bottom bar
                     (x >= centerX - 60 && x < centerX - 55))) { // Left bar
                    inLogo = true;
                }
                
                // 'P'
                if (x >= centerX - 30 && x < centerX - 10 && 
                    ((y >= centerY - 20 && y < centerY + 20 && x >= centerX - 30 && x < centerX - 25) || // Left bar
                     (y >= centerY - 20 && y < centerY - 15) ||  // Top bar
                     (y >= centerY - 5 && y < centerY) ||        // Middle bar
                     (x >= centerX - 15 && x < centerX - 10 && y >= centerY - 20 && y < centerY))) { // Right bar of P
                    inLogo = true;
                }
                
                // 'S'
                if (x >= centerX + 0 && x < centerX + 20 && 
                    ((y >= centerY - 20 && y < centerY - 15) ||  // Top bar
                     (y >= centerY - 5 && y < centerY) ||        // Middle bar
                     (y >= centerY + 15 && y < centerY + 20) ||  // Bottom bar
                     (x >= centerX + 0 && x < centerX + 5 && y >= centerY - 20 && y < centerY - 5) || // Top-left bar
                     (x >= centerX + 15 && x < centerX + 20 && y >= centerY && y < centerY + 20))) { // Bottom-right bar
                    inLogo = true;
                }
                
                // '2'
                if (x >= centerX + 30 && x < centerX + 50 && 
                    ((y >= centerY - 20 && y < centerY - 15) ||  // Top bar
                     (y >= centerY - 5 && y < centerY) ||        // Middle bar
                     (y >= centerY + 15 && y < centerY + 20) ||  // Bottom bar
                     (x >= centerX + 45 && x < centerX + 50 && y >= centerY - 20 && y < centerY - 5) || // Top-right bar
                     (x >= centerX + 30 && x < centerX + 35 && y >= centerY && y < centerY + 20))) { // Bottom-left bar
                    inLogo = true;
                }
                
                if (inLogo) {
                    // Calculate pulsating blue
                    int blue = 128 + (int)(127.0f * sinf(g_nFrameCounter * 0.1f));
                    color = 0xFF000000 | blue;
                }
            }
            
            // Store the pixel
            dst32[y * width + x] = color;
        }
    }
    
    // Add frame counter text
    char frameText[50];
    snprintf(frameText, sizeof(frameText), "Frame: %d", g_nFrameCounter);
    
    // Very simple text rendering
    int textX = 10;
    int textY = 10;
    UINT32 textColor = 0xFFFFFFFF; // White
    
    // Draw each character
    for (int i = 0; frameText[i] != '\0'; i++) {
        char c = frameText[i];
        
        // Only handle basic characters
        if (c >= 32 && c < 127) {
            // Draw a simple block for each character
            for (int py = 0; py < 8; py++) {
                for (int px = 0; px < 6; px++) {
                    // Skip spaces between characters
                    if (px == 5) continue;
                    
                    // Simple binary representation of characters (just blocks)
                    if (c != ' ') {
                        int pixelX = textX + i * 8 + px;
                        int pixelY = textY + py;
                        
                        // Ensure we're in bounds
                        if (pixelX >= 0 && pixelX < width && pixelY >= 0 && pixelY < height) {
                            // For digits, draw a simple block representation
                            if (c >= '0' && c <= '9') {
                                if (px > 0 && px < 4 && py > 0 && py < 7) {
                                    // Skip middle of '0'
                                    if (c == '0' && px > 1 && px < 3 && py > 1 && py < 6) {
                                        continue;
                                    }
                                    
                                    dst32[pixelY * width + pixelX] = textColor;
                                }
                            } else {
                                // For other characters, just draw a simple block
                                dst32[pixelY * width + pixelX] = textColor;
                            }
                        }
                    }
                }
            }
        }
        
        // Move to the next character position
        textX += 6;
    }
    
    // If CPS2 driver is initialized, add status text
    if (g_bDriverInitialized) {
        const char* statusText = "CPS2 Driver Ready - Load ROM";
        
        // Draw at the bottom of the screen
        textX = 10;
        textY = height - 20;
        
        // Draw each character (simplified approach)
        for (int i = 0; statusText[i] != '\0'; i++) {
            char c = statusText[i];
            
            // Only handle basic characters
            if (c >= 32 && c < 127) {
                // Draw a simple block for each character
                for (int py = 0; py < 8; py++) {
                    for (int px = 0; px < 6; px++) {
                        // Skip spaces between characters
                        if (px == 5) continue;
                        
                        // Simple binary representation of characters (just blocks)
                        if (c != ' ') {
                            int pixelX = textX + i * 8 + px;
                            int pixelY = textY + py;
                            
                            // Ensure we're in bounds
                            if (pixelX >= 0 && pixelX < width && pixelY >= 0 && pixelY < height) {
                                dst32[pixelY * width + pixelX] = 0xFFFFFF00; // Yellow
                            }
                        }
                    }
                }
            }
            
            // Move to the next character position
            textX += 8;
        }
    }
    
    // If a game is initialized, add game name
    if (g_bGameInitialized) {
        const char* gameText = "Marvel vs. Capcom";
        
        // Draw at the top of the screen
        textX = width - 200;
        textY = 10;
        
        // Draw each character (simplified approach)
        for (int i = 0; gameText[i] != '\0'; i++) {
            char c = gameText[i];
            
            // Only handle basic characters
            if (c >= 32 && c < 127) {
                // Draw a simple block for each character
                for (int py = 0; py < 8; py++) {
                    for (int px = 0; px < 6; px++) {
                        // Skip spaces between characters
                        if (px == 5) continue;
                        
                        // Simple binary representation of characters (just blocks)
                        if (c != ' ') {
                            int pixelX = textX + i * 8 + px;
                            int pixelY = textY + py;
                            
                            // Ensure we're in bounds
                            if (pixelX >= 0 && pixelX < width && pixelY >= 0 && pixelY < height) {
                                dst32[pixelY * width + pixelX] = 0xFF00FF00; // Green
                            }
                        }
                    }
                }
            }
            
            // Move to the next character position
            textX += 8;
        }
    }
}

#ifdef __cplusplus
}
#endif 