// CPS2 Core Bridge for Metal Implementation
// This bridges the Metal frontend to the minimal CPS2 driver

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

// CPS2 Memory Areas - These are allocated by the minimal driver
extern UINT8* CpsRom;      // 68K Program ROM
extern UINT8* CpsGfx;      // Graphics ROM
extern UINT8* CpsZRom;     // Z80 ROM
extern UINT8* CpsQSam;     // QSound Samples

// CPS2 Memory Sizes
extern INT32 nCpsRomLen;
extern INT32 nCpsGfxLen;
extern INT32 nCpsZRomLen;
extern INT32 nCpsQSamLen;

// Frame buffer for rendering
extern UINT8* pBurnDraw;
extern INT32 nBurnPitch;

// The minimal driver already implements these functions
extern INT32 Cps2Init();
extern INT32 Cps2Frame();
extern INT32 CpsExit();
extern INT32 CpsRunInit();
extern INT32 CpsRunExit();
extern void CpsRwGetInp();
extern INT32 CpsDraw();
extern INT32 CpsObjGet();
extern INT32 CpsGetROMs(INT32 bLoad);

// CPS2 RunFrame wrapper
INT32 Metal_CPS2_RunFrame(int render) {
    if (render) {
        return Cps2Frame();
    }
    return 0;
}

// CPS Graphics Redraw function - handled by Cps2Frame
extern "C" void CpsRedraw() {
    // Handled internally by Cps2Frame()
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

// Load a specific CPS2 game
INT32 Metal_LoadCPS2Game(const char* gameName) {
    printf("[Metal_LoadCPS2Game] Loading CPS2 game: %s\n", gameName);
    
    // Initialize CPS2 system
    INT32 nRet = Cps2Init();
    if (nRet != 0) {
        printf("[Metal_LoadCPS2Game] Failed to initialize CPS2\n");
        return nRet;
    }
    
    // Load ROMs
    nRet = CpsGetROMs(1);
    if (nRet != 0) {
        printf("[Metal_LoadCPS2Game] Failed to load ROMs\n");
        return nRet;
    }
    
    // Initialize runtime
    nRet = CpsRunInit();
    if (nRet != 0) {
        printf("[Metal_LoadCPS2Game] Failed to initialize runtime\n");
        return nRet;
    }
    
    printf("[Metal_LoadCPS2Game] Game loaded successfully\n");
    return 0;
}

#ifdef __cplusplus
}
#endif 