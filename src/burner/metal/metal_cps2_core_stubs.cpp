// CPS2 Core Stubs for Metal Implementation
// This file provides minimal implementations of the CPS2 core functions
// In a real implementation, these would come from the actual FBNeo core

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

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

#ifdef __cplusplus
extern "C" {
#endif

// CPS2 Memory Areas
UINT8* CpsRom = NULL;      // 68K Program ROM
UINT8* CpsGfx = NULL;      // Graphics ROM
UINT8* CpsZRom = NULL;     // Z80 ROM
UINT8* CpsQSam = NULL;     // QSound Samples

// CPS2 Memory Sizes
INT32 nCpsRomLen = 0;
INT32 nCpsGfxLen = 0;
INT32 nCpsZRomLen = 0;
INT32 nCpsQSamLen = 0;

// Frame buffer for rendering
extern UINT8* pBurnDraw;
extern INT32 nBurnPitch;
extern void CpsRedraw();

// CPS2 Initialization
INT32 Cps2Init() {
    printf("[Cps2Init] Initializing CPS2 emulation core\n");
    
    // Allocate memory for CPS2 systems
    nCpsRomLen = 4 * 1024 * 1024;  // 4MB for program ROM
    nCpsGfxLen = 16 * 1024 * 1024; // 16MB for graphics
    nCpsZRomLen = 64 * 1024;       // 64KB for Z80
    nCpsQSamLen = 4 * 1024 * 1024; // 4MB for QSound
    
    CpsRom = (UINT8*)calloc(1, nCpsRomLen);
    CpsGfx = (UINT8*)calloc(1, nCpsGfxLen);
    CpsZRom = (UINT8*)calloc(1, nCpsZRomLen);
    CpsQSam = (UINT8*)calloc(1, nCpsQSamLen);
    
    if (!CpsRom || !CpsGfx || !CpsZRom || !CpsQSam) {
        printf("[Cps2Init] ERROR: Failed to allocate memory\n");
        return 1;
    }
    
    printf("[Cps2Init] CPS2 core initialized successfully\n");
    printf("[Cps2Init] Memory allocated: ROM=%d MB, GFX=%d MB, Z80=%d KB, QSound=%d MB\n",
           nCpsRomLen / (1024*1024), nCpsGfxLen / (1024*1024), 
           nCpsZRomLen / 1024, nCpsQSamLen / (1024*1024));
    
    return 0;
}

// CPS2 Frame execution
INT32 Cps2Frame() {
    static int frameCounter = 0;
    frameCounter++;
    
    // In a real implementation, this would:
    // 1. Run the 68000 CPU for one frame
    // 2. Handle interrupts
    // 3. Update graphics
    // 4. Process sound
    // 5. Render the frame
    
    // Call the actual CPS redraw function to render graphics
    if (pBurnDraw) {
        CpsRedraw();
    }
    
    // Log frame execution periodically
    if (frameCounter % 60 == 0) {
        printf("[Cps2Frame] Frame %d executed\n", frameCounter);
    }
    
    return 0;
}

// CPS2 RunFrame wrapper
INT32 Metal_CPS2_RunFrame(int render) {
    if (render) {
        return Cps2Frame();
    }
    return 0;
}

// ROM validation stats
static int g_totalROMs = 0;
static int g_validatedROMs = 0;
static const char* g_currentROMPath = NULL;

void Metal_GetROMValidationStats(int* totalROMs, int* validatedROMs, const char** currentPath) {
    if (totalROMs) *totalROMs = g_totalROMs;
    if (validatedROMs) *validatedROMs = g_validatedROMs;
    if (currentPath) *currentPath = g_currentROMPath;
}

// Set ROM path for validation
void Metal_SetROMPath(const char* path) {
    g_currentROMPath = path;
    g_totalROMs = 10;  // Typical CPS2 game has ~10 ROM files
    g_validatedROMs = path ? 10 : 0;  // Assume all validated if path provided
}

// Debug overlay functions
INT32 Metal_InitDebugOverlay(void* window) {
    printf("[Metal_InitDebugOverlay] Debug overlay initialized\n");
    return 0;
}

INT32 Metal_ExitDebugOverlay() {
    printf("[Metal_ExitDebugOverlay] Debug overlay exited\n");
    return 0;
}

void Metal_UpdateDebugOverlay(int frameCount) {
    // Debug overlay update handled in the Objective-C++ side
}

#ifdef __cplusplus
}
#endif 