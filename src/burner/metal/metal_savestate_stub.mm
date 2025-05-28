#import <Foundation/Foundation.h>
#include <stdio.h>
#include <string.h>
#include "metal_savestate_stubs.h"

// Basic type definitions
typedef int INT32;

// Stub implementation of the savestate system for the Metal port
// This avoids using the BurnStateInfo struct which is causing build errors

// Forward declarations with proper C linkage
extern "C" INT32 Metal_SaveState(INT32 nSlot);
extern "C" INT32 Metal_LoadState(INT32 nSlot);
extern "C" INT32 Metal_InitSaveState();
extern "C" INT32 Metal_ExitSaveState();
extern "C" INT32 Metal_QuickSave();
extern "C" INT32 Metal_QuickLoad();
extern "C" const char* Metal_GetSaveStateStatus();
extern "C" int Metal_GetCurrentSaveSlot();

// Implementations
INT32 Metal_SaveState(INT32 nSlot) {
    printf("[Metal_SaveState_Stub] Saving state to slot %d (stub implementation)\n", nSlot);
    return 0; // Success
}

INT32 Metal_LoadState(INT32 nSlot) {
    printf("[Metal_LoadState_Stub] Loading state from slot %d (stub implementation)\n", nSlot);
    return 0; // Success
}

INT32 Metal_InitSaveState() {
    printf("[Metal_InitSaveState_Stub] Initializing savestate system (stub implementation)\n");
    return 0; // Success
}

INT32 Metal_ExitSaveState() {
    printf("[Metal_ExitSaveState_Stub] Shutting down savestate system (stub implementation)\n");
    return 0; // Success
}

INT32 Metal_QuickSave() {
    printf("[Metal_QuickSave_Stub] Quick save (stub implementation)\n");
    return Metal_SaveState(0);
}

INT32 Metal_QuickLoad() {
    printf("[Metal_QuickLoad_Stub] Quick load (stub implementation)\n");
    return Metal_LoadState(0);
}

const char* Metal_GetSaveStateStatus() {
    return "Stub Implementation";
}

int Metal_GetCurrentSaveSlot() {
    return 0;
} 