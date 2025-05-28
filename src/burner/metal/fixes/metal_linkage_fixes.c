/**
 * Metal Linkage Fixes
 * 
 * This file contains implementations for functions that are needed
 * for proper linking but may be missing from other parts of the code.
 */

#include <stdint.h>
#include <stdlib.h>
#include "../metal_declarations.h"
#include "../metal_renderer_defines.h"
#include <stdio.h>

// Frame buffer globals that must be defined for linking
UINT8* pBurnDraw_Metal = NULL;
INT32 nBurnPitch_Metal = 0;
INT32 nBurnBpp_Metal = 0;

// Globals needed for the C code to avoid struct/enum dependency errors
void* pCheatInfo = NULL;
void* pDataRomDesc = NULL;
void* pRDI = NULL;

// Function to set AI defaults since we can't include ai_definitions.h in C code
int g_nAIDifficulty = 0;
int g_nAIPlayer = 0;
bool g_bAITrainingMode = false;
bool g_bAIDebugOverlay = false;

// Metal texture functions that must be defined for linking
int Metal_UpdateTexture(void* data, int width, int height, int pitch) {
    if (data && width > 0 && height > 0 && pitch > 0) {
        // In a real implementation, this would update a Metal texture
        return 0;
    }
    return 1;
}

// Basic MetalRenderer implementations for linking
void MetalRenderer_SetOption(int option, int value) {
    // In a real implementation, this would set renderer options
}

int MetalRenderer_DrawFrame(int draw) {
    return 0;
}

// Initialize Metal with view pointer
int Metal_Init(void) {
    printf("Metal_Init stub called\n");
    return 0;
}

// Function to query if Metal is active
int Metal_IsActive(void) {
    return 1; // Always return true for the standalone build
}

// Get renderer info string
const char* Metal_GetRendererInfo(void) {
    return "FBNeo Metal Renderer";
}

// Register Metal callbacks with the core
void Metal_RegisterCallbacks(void* initFunc, void* renderFunc, void* shutdownFunc) {
    printf("Metal_RegisterCallbacks stub called\n");
}

// Input handling
int BurnDrvGetInputAssembly(void) {
    return 0;
}

// SekTotalCycles if not defined elsewhere
#ifndef INCLUDE_SEK_STUBS
INT32 SekTotalCycles(void) {
    return 0;
}

void SekSetRESETLine(INT32 cpu, INT32 state) {
    // Do nothing in the standalone implementation
}

void SekClose(void) {
    // Do nothing in the standalone implementation
}

INT32 SekInit(INT32 nCount, INT32 nCPUType) {
    return 0;
}

void SekOpen(INT32 i) {
    // Do nothing in the standalone implementation
}

void SekReset(void) {
    // Do nothing in the standalone implementation
}

INT32 SekRun(INT32 nCycles) {
    return 0;
}

void SekSetIRQLine(INT32 nIRQLine, INT32 nState) {
    // Do nothing in the standalone implementation
}
#endif

// ZetRun if not defined elsewhere
#ifndef INCLUDE_ZET_STUBS
INT32 ZetRun(INT32 nCycles) {
    return 0;
}

void ZetSetIRQLine(INT32 nIRQLine, INT32 nStatus) {
    // Do nothing in the standalone implementation
}

void ZetReset(void) {
    // Do nothing in the standalone implementation
}
#endif

// Input related functions if not defined elsewhere
#ifndef INCLUDE_INPUT_STUBS
void BurnSetMouseDivider(INT32 nDivider) {
    // Do nothing in the standalone implementation
}

void AnalogDeadZone(INT32 nJoy, UINT8 nAxis, INT32 nDeadZone) {
    // Do nothing in the standalone implementation
}

INT32 ProcessAnalog(INT32 nJoy, UINT8 nAxis, INT32 nMinVal, INT32 nMidVal, INT32 nMaxVal, INT32 nMakeDeadZones, char* szAxis) {
    return 0;
}
#endif 

// Stubs for the functions that cause C++ vs C linkage issues
void IpsApplyPatches(unsigned char* base, char* rom_name, unsigned int rom_crc, unsigned char readonly) {
    // Empty implementation
}

void HiscoreReset(INT32 bDisableInversionWriteback) {
    // Empty implementation
}

void BurnGetLocalTime(void* nTime) {
    // Empty implementation
}

INT32 BurnDrvCartridgeSetup(INT32 nCommand) {
    return 0;
}

// Extern declarations of the callback functions with proper C linkage
INT32 (*BurnExtCartridgeSetupCallback)(INT32 nCommand) = NULL;

// Metal shutdown
int Metal_Exit(void) {
    printf("Metal_Exit stub called\n");
    return 0;
}

// Metal save state
INT32 Metal_SaveState(void* buffer, int* size) {
    printf("Metal_SaveState stub called\n");
    return 0;
}

// Metal load state
INT32 Metal_LoadState(void* buffer, int size) {
    printf("Metal_LoadState stub called\n");
    return 0;
}

// Metal set option
void Metal_SetOption(int option, int value) {
    printf("Metal_SetOption stub called\n");
}

// Metal get option
int Metal_GetOption(int option) {
    printf("Metal_GetOption stub called\n");
    return 0;
}

// Metal set viewport
void Metal_SetViewport(int x, int y, int width, int height) {
    printf("Metal_SetViewport stub called\n");
}

// Metal verify frame pipeline
int Metal_VerifyFramePipeline(int width, int height) {
    printf("Metal_VerifyFramePipeline stub called\n");
    return 0;
} 