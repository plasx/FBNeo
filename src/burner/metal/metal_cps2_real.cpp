// Real CPS2 Driver Integration for Metal Frontend
// This connects the actual FBNeo CPS2 emulation to the Metal renderer

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "metal_compat_layer.h"

// CPS2 Driver includes from FBNeo
extern "C" {
    // CPS system variables
    extern INT32 Cps;
    extern UINT8* CpsGfx;
    extern UINT8* CpsRom; 
    extern UINT8* CpsZRom;
    extern UINT8* CpsQSam;
    extern UINT32 nCpsGfxLen;
    extern UINT32 nCpsRomLen;
    extern UINT32 nCpsZRomLen;
    extern UINT32 nCpsQSamLen;
    
    // Frame buffer
    extern UINT8* pBurnDraw;
    extern INT32 nBurnPitch;
    extern INT32 nBurnBpp;
    
    // CPS2 functions from the real driver
    extern INT32 Cps2Init();
    extern INT32 CpsExit();
    extern INT32 Cps2Frame();
    extern INT32 CpsRunInit();
    extern INT32 CpsRunExit();
    extern void CpsRwGetInp();
    extern INT32 CpsDraw();
    extern INT32 CpsObjGet();
    
    // ROM loading
    extern INT32 CpsGetROMs(INT32 bLoad);
    extern INT32 BurnLoadRom(UINT8* Dest, INT32* pnWrote, INT32 i);
    
    // Driver selection
    extern INT32 BurnDrvFind(const char* name);
    extern INT32 BurnDrvSelect(INT32 i);
    extern UINT32 nBurnDrvCount;
    extern INT32 nBurnDrvActive;
}

// State tracking
static bool g_bCPS2Initialized = false;
static bool g_bGameLoaded = false;
static int g_nFrameCount = 0;

// Initialize real CPS2 emulation
INT32 Metal_CPS2_RealInit() {
    printf("[Metal_CPS2_RealInit] Initializing real CPS2 emulation\n");
    
    if (g_bCPS2Initialized) {
        printf("[Metal_CPS2_RealInit] Already initialized\n");
        return 0;
    }
    
    // The CPS2 system will be initialized when a game is loaded
    g_bCPS2Initialized = true;
    
    printf("[Metal_CPS2_RealInit] CPS2 system ready\n");
    return 0;
}

// Exit CPS2 emulation
INT32 Metal_CPS2_RealExit() {
    printf("[Metal_CPS2_RealExit] Shutting down CPS2 emulation\n");
    
    if (g_bGameLoaded) {
        CpsRunExit();
        CpsExit();
        g_bGameLoaded = false;
    }
    
    g_bCPS2Initialized = false;
    return 0;
}

// Load a CPS2 game
INT32 Metal_CPS2_RealLoadGame(const char* gameName) {
    printf("[Metal_CPS2_RealLoadGame] Loading game: %s\n", gameName);
    
    if (!g_bCPS2Initialized) {
        printf("[Metal_CPS2_RealLoadGame] ERROR: CPS2 not initialized\n");
        return 1;
    }
    
    // Exit current game if one is loaded
    if (g_bGameLoaded) {
        CpsRunExit();
        CpsExit();
        g_bGameLoaded = false;
    }
    
    // Find the driver
    INT32 nDrvIndex = BurnDrvFind(gameName);
    if (nDrvIndex < 0) {
        printf("[Metal_CPS2_RealLoadGame] ERROR: Game '%s' not found\n", gameName);
        return 1;
    }
    
    // Select the driver
    INT32 nRet = BurnDrvSelect(nDrvIndex);
    if (nRet != 0) {
        printf("[Metal_CPS2_RealLoadGame] ERROR: Failed to select driver\n");
        return 1;
    }
    
    // Initialize the CPS2 driver - this loads ROMs and sets up emulation
    nRet = Cps2Init();
    if (nRet != 0) {
        printf("[Metal_CPS2_RealLoadGame] ERROR: Cps2Init failed with code %d\n", nRet);
        return 1;
    }
    
    g_bGameLoaded = true;
    g_nFrameCount = 0;
    
    // Log memory allocation info
    printf("[Metal_CPS2_RealLoadGame] Game loaded successfully\n");
    printf("  CpsGfx: %p (size: %u)\n", CpsGfx, nCpsGfxLen);
    printf("  CpsRom: %p (size: %u)\n", CpsRom, nCpsRomLen);
    printf("  CpsZRom: %p (size: %u)\n", CpsZRom, nCpsZRomLen);
    printf("  CpsQSam: %p (size: %u)\n", CpsQSam, nCpsQSamLen);
    printf("  pBurnDraw: %p (pitch: %d, bpp: %d)\n", pBurnDraw, nBurnPitch, nBurnBpp);
    
    return 0;
}

// Run one frame of CPS2 emulation
INT32 Metal_CPS2_RealRunFrame() {
    if (!g_bGameLoaded) {
        //printf("[Metal_CPS2_RealRunFrame] No game loaded\n");
        return 1;
    }
    
    // Run one frame of CPS2 emulation
    INT32 nRet = Cps2Frame();
    
    if (nRet != 0) {
        printf("[Metal_CPS2_RealRunFrame] ERROR: Cps2Frame failed with code %d\n", nRet);
        return nRet;
    }
    
    g_nFrameCount++;
    
    // Log periodically
    if (g_nFrameCount % 60 == 0) {
        printf("[Metal_CPS2_RealRunFrame] Frame %d completed\n", g_nFrameCount);
        
        // Check frame buffer content
        if (pBurnDraw) {
            UINT32* pixels = (UINT32*)pBurnDraw;
            int nonZero = 0;
            for (int i = 0; i < 1000; i++) {
                if (pixels[i] != 0) nonZero++;
            }
            printf("[Metal_CPS2_RealRunFrame] Frame buffer: %d/1000 non-zero pixels\n", nonZero);
        }
    }
    
    return 0;
}

// Get frame dimensions
void Metal_CPS2_RealGetDimensions(int* width, int* height) {
    // CPS2 standard resolution
    if (width) *width = 384;
    if (height) *height = 224;
} 