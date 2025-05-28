// Stub implementation for Metal frontend to avoid compiling all of FBneo
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>

// Basic type definitions 
typedef int INT32;
typedef unsigned int UINT32;
typedef unsigned short UINT16; 
typedef unsigned char UINT8;
typedef short INT16;
typedef int BOOL;

// Global variables needed by the core
INT32 nBurnSoundLen = 0;
INT32 nBurnSoundRate = 0;
UINT8 *pBurnSoundOut = NULL;

// ----------------------------------------------------------------------------
// Driver metadata and configuration
// ----------------------------------------------------------------------------

// Get driver text
// REMOVED: Duplicate implementation of BurnDrvGetTextA
// Using the implementation from driver_functions.c
const char* BurnDrvGetTextA_UNUSED(UINT32 i) {
    // This function has been removed to avoid duplicates
    return "Unused";
}
// Get driver index
// REMOVED: Duplicate implementation of BurnDrvGetIndex
// Using the implementation from driver_functions.c
INT32 BurnDrvGetIndex_UNUSED(const char* name) {
    // This function has been removed to avoid duplicates
    return -1;
}
// Select driver
// REMOVED: Duplicate implementation of BurnDrvSelect
// Using the implementation from driver_functions.c
INT32 BurnDrvSelect_UNUSED(INT32 nDriver) {
    // This function has been removed to avoid duplicates
    return 0;
}
INT32 BurnDrvGetFullSize(INT32* pnWidth, INT32* pnHeight) {
    if (pnWidth) *pnWidth = 384;
    if (pnHeight) *pnHeight = 224;
    return 0;
}

// Get visible size
// REMOVED: Duplicate implementation of BurnDrvGetVisibleSize
// Using the implementation from driver_functions.c
INT32 BurnDrvGetVisibleSize_UNUSED(INT32* pnWidth, INT32* pnHeight) {
    // This function has been removed to avoid duplicates
    return 0;
}
// Debug and logging
// ----------------------------------------------------------------------------

// Debug message output
int bprintf(int nStatus, const char* szFormat, ...) {
    return 0;
}

// ----------------------------------------------------------------------------
// Memory management
// ----------------------------------------------------------------------------

// Memory allocation functions
void* BurnLocalloc(UINT32 size) {
    return malloc(size);
}

void BurnLocalfree(void* ptr) {
    free(ptr);
}

// ----------------------------------------------------------------------------
// Sound functions
// ----------------------------------------------------------------------------

INT32 BurnSoundInit() { 
    return 0; 
}

INT32 BurnSoundExit() { 
    return 0; 
}

INT32 BurnSoundStop() { 
    return 0; 
}

INT32 BurnSoundPlay() { 
    return 0; 
}

INT32 BurnSoundGetPosition() { 
    return 0; 
}

INT32 BurnSoundSetVolume(INT32 nVol) { 
    return 0; 
}

void BurnSoundCommand(INT32 nCommand) {}

void BurnSoundUpdate() {}

// Now using unified implementation in sound_globals.c
// ----------------------------------------------------------------------------
// Rendering
// ----------------------------------------------------------------------------

void BurnDrvSetPBurnDraw(UINT8* pImage, INT32 nPitch, INT32 nBpp) {
    // Connect the frame buffer to FBNeo
}

// ----------------------------------------------------------------------------
// Miscellaneous core functions
// ----------------------------------------------------------------------------

// Initialize driver
// REMOVED: Duplicate implementation of BurnDrvInit
// Using the implementation from driver_functions.c
INT32 BurnDrvInit_UNUSED(void) {
    // This function has been removed to avoid duplicates
    return 0;
}
// Exit driver
// REMOVED: Duplicate implementation of BurnDrvExit
// Using the implementation from driver_functions.c
INT32 BurnDrvExit_UNUSED(void) {
    // This function has been removed to avoid duplicates
    return 0;
}
// Run frame
INT32 RunFrame(int bDraw) {
    return 0;
}

// High color conversion
// REMOVED: Duplicate implementation of BurnHighCol
// Using the implementation from driver_functions.c
UINT32 BurnHighCol_UNUSED(INT32 r, INT32 g, INT32 b, INT32 i) {
    // This function has been removed to avoid duplicates
    return 0;
}
INT32 BurnTimerSetRetrig(INT32 c, double period) {
    return 0;
}

INT32 BurnTimerReset() {
    return 0;
}

INT32 BurnTimerEndFrame(INT32 nCycles) {
    return 0;
}

// Zip handling
INT32 BurnExtLoadRom(UINT8* pDest, INT32* pnWrote, INT32 i) {
    return 0;
}

// Cheats
void CheatApply(void) {
}

// Reset
INT32 BurnDrvReset() {
    return 0;
}

// Now using unified implementation in sound_globals.c
// REMOVED: Duplicate implementation of BurnSoundRender
// Now using unified implementation in sound_globals.c