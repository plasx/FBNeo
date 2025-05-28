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
    
    // CPS2 specific functions
    extern INT32 CpsRunInit();
    extern INT32 CpsRunExit();
    extern void CpsRwGetInp();
    
    // ROM loading
    extern INT32 BurnLoadRom(UINT8* Dest, INT32* pnWrote, INT32 i);
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
static const char* g_szCurrentROMPath = NULL;

// Function to get current timestamp in microseconds
static uint64_t GetMicrosecondTimestamp() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

// Store the current ROM path
void Metal_SetCurrentROMPath(const char* romPath) {
    g_szCurrentROMPath = romPath;
    printf("[Metal_SetCurrentROMPath] ROM path set to: %s\n", romPath ? romPath : "NULL");
}

// Get the current ROM path
const char* Metal_GetCurrentROMPath() {
    return g_szCurrentROMPath;
}

// Metal CPS2 system implementation
INT32 Metal_CPS2_Init() {
    printf("[Metal_CPS2_Init] Initializing CPS2 system\n");
    
    // Initialize CPS2 system - this sets up the CPS2 hardware emulation
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

// Forward declaration for ROM loading
extern "C" INT32 Metal_LoadCPS2ROMs(const char* romPath, int gameIndex);

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
    
    // Get the current ROM path
    const char* romPath = Metal_GetCurrentROMPath();
    
    if (!romPath || romPath[0] == '\0') {
        printf("[Metal_CPS2_LoadGame] ERROR: No ROM path specified\n");
        return 1;
    }
    
    // Find and select Marvel vs. Capcom
    int nDrvIndex = BurnDrvFind("mvsc");
    if (nDrvIndex < 0) {
        printf("[Metal_CPS2_LoadGame] ERROR: Could not find game 'mvsc'\n");
        return 1;
    }
    
    printf("[Metal_CPS2_LoadGame] Found 'mvsc' at driver index %d\n", nDrvIndex);
    
    // Select the driver
    INT32 nRet = BurnDrvSelect(nDrvIndex);
    if (nRet != 0) {
        printf("[Metal_CPS2_LoadGame] ERROR: BurnDrvSelect failed with code %d\n", nRet);
        return nRet;
    }
    
    // Initialize the driver - this will call Cps2Init() internally
    nRet = BurnDrvInit();
    if (nRet != 0) {
        printf("[Metal_CPS2_LoadGame] ERROR: BurnDrvInit failed with code %d\n", nRet);
        return nRet;
    }
    
    g_bGameInitialized = true;
    g_nCurrentGame = gameIndex;
    g_nFrameCounter = 0;
    
    // Verify frame buffer is set up
    printf("[Metal_CPS2_LoadGame] Frame buffer info:\n");
    printf("  pBurnDraw: %p\n", pBurnDraw);
    printf("  nBurnPitch: %d\n", nBurnPitch);
    printf("  nBurnBpp: %d\n", nBurnBpp);
    
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
        printf("[Metal_RunFrame] ERROR: No game loaded\n");
        return 1;
    }
    
    // Process input before running the frame
    extern void Metal_ProcessInput();
    Metal_ProcessInput();
    
    // Track timing
    static uint64_t frameStartTime = 0;
    uint64_t currentTime = GetMicrosecondTimestamp();
    
    if (frameStartTime == 0) {
        frameStartTime = currentTime;
    }
    
    // Run one frame of emulation - this calls Cps2Frame() internally
    INT32 nRet = BurnDrvFrame();
    
    if (nRet != 0) {
        printf("[Metal_RunFrame] ERROR: BurnDrvFrame failed with code %d\n", nRet);
        return nRet;
    }
    
    // Calculate frame time
    uint64_t frameEndTime = GetMicrosecondTimestamp();
    uint64_t frameTime = frameEndTime - frameStartTime;
    frameStartTime = frameEndTime;
    
    // Update frame counter
    g_nFrameCounter++;
    
    // Log performance every 60 frames
    if (g_nFrameCounter % 60 == 0) {
        printf("[Metal_RunFrame] Frame %d time: %llu µs (%.2f FPS)\n", 
               g_nFrameCounter, frameTime, 1000000.0f / frameTime);
        
        // Verify frame buffer content
        if (pBurnDraw) {
            UINT32* pixels = (UINT32*)pBurnDraw;
            int nonZeroCount = 0;
            for (int i = 0; i < 1000 && i < (384 * 224); i++) {
                if (pixels[i] != 0) nonZeroCount++;
            }
            printf("[Metal_RunFrame] Frame buffer content: %d/1000 non-zero pixels\n", nonZeroCount);
        }
    }
    
    return 0;
}

// Get the current frame buffer
void* Metal_GetFrameBuffer() {
    // Return the real frame buffer from FBNeo
    return pBurnDraw;
}

// Get raw frame buffer (same as regular frame buffer for CPS2)
void* Metal_GetRawFrameBuffer() {
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
    
    // Map input data to CPS input variables
    // CPS2 uses specific memory locations for input
    
    // Process reset input
    CpsReset = (inputData[0] & 0x01) ? 1 : 0;
    
    // Process player 1 inputs
    if (size > 1) {
        CpsInp001[0] = inputData[1];  // Player 1 controls
    }
    
    // Process player 2 inputs
    if (size > 2) {
        CpsInp000[0] = inputData[2];  // Player 2 controls
    }
    
    // Process system inputs
    if (size > 3) {
        CpsInp020[0] = inputData[3];  // Coin/Start buttons
    }
}

// CPS2 memory allocation - handled by the real CPS2 core
INT32 Metal_CPS2_AllocateMemory(int romSize, int gfxSize, int z80Size, int qsndSize) {
    printf("[Metal_CPS2_AllocateMemory] Memory allocation handled by CPS2 core\n");
    printf("  ROM size: %d bytes\n", romSize);
    printf("  GFX size: %d bytes\n", gfxSize);
    printf("  Z80 size: %d bytes\n", z80Size);
    printf("  QSound size: %d bytes\n", qsndSize);
    
    // Memory allocation is handled by Cps2Init() in the real implementation
    return 0;
}

void Metal_CPS2_FreeMemory() {
    printf("[Metal_CPS2_FreeMemory] Memory deallocation handled by CPS2 core\n");
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

// Direct connections to real CPS2 functions
extern "C" INT32 mvscInit() {
    printf("[mvscInit] Calling real Cps2Init\n");
    return Cps2Init();
}

extern "C" INT32 mvscFrame() {
    // Run the real CPS2 frame
    return Cps2Frame();
}

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
    // Forward to the real validation function
    return Metal_ValidateROMFile(path);
}

// Function to help find a matching CPS2 ROM
extern "C" INT32 Metal_FindCPS2ROM(const char* path) {
    // Forward to the real function
    return FindCps2Rom(path);
}

#ifdef __cplusplus
}
#endif 