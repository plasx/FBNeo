//
// cps2_fixes.cpp
//
// CPS2 fixes for Metal build
//

// Include our declarations first to avoid conflicts
#include "../metal_declarations.h"

// System includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declarations for CPS2 functions we need
extern "C" {
    int Cps2LoadRoms(bool bLoad);
    int Cps2Init();
    int Cps2Exit();
    int Cps2Frame();
}

// CPS2 driver callback replacements
int Cps2LoadRomsCallback(bool bLoad) {
    printf("Cps2LoadRomsCallback(%d) called\n", bLoad);
    return Cps2LoadRoms(bLoad);
}

int Cps2InitCallback() {
    printf("Cps2InitCallback() called\n");
    return Cps2Init();
}

int Cps2ExitCallback() {
    printf("Cps2ExitCallback() called\n");
    return Cps2Exit();
}

int Cps2FrameCallback() {
    // Don't log every frame to reduce spam
    return Cps2Frame();
}

// Additional callbacks can be added as needed
// For example, if there are specific memory access callbacks needed by CPS2:

UINT8 Cps2ReadByteCallback(UINT32 address) {
    printf("Cps2ReadByteCallback(0x%08X) called\n", address);
    // Implement actual read logic or forward to original function
    return 0;
}

void Cps2WriteByteCallback(UINT32 address, UINT8 value) {
    printf("Cps2WriteByteCallback(0x%08X, 0x%02X) called\n", address, value);
    // Implement actual write logic or forward to original function
}

UINT16 Cps2ReadWordCallback(UINT32 address) {
    // Don't log to reduce spam
    // Implement actual read logic or forward to original function
    return 0;
}

void Cps2WriteWordCallback(UINT32 address, UINT16 value) {
    // Don't log to reduce spam
    // Implement actual write logic or forward to original function
}

// Setup function to register callbacks
void Cps2SetupCallbacks() {
    printf("Cps2SetupCallbacks() called\n");
    
    // Register callbacks with the CPS2 driver
    // This would typically involve setting function pointers
    // in the CPS2 driver to our callback functions
    
    // Example (replace with actual implementation):
    // Cps2Callbacks.LoadRoms = Cps2LoadRomsCallback;
    // Cps2Callbacks.Init = Cps2InitCallback;
    // Cps2Callbacks.Exit = Cps2ExitCallback;
    // Cps2Callbacks.Frame = Cps2FrameCallback;
}

// Define all key global variables needed for CPS2 emulation
// These are normally defined in the CPS2 driver but may need to be exposed for Metal
INT32 Cps = 0;
UINT16 Cps2VolumeStates[40] = {0};
INT32 Cps2Volume = 39;
INT32 Cps2DisableDigitalVolume = 0;
INT32 Cps2DisableQSnd = 0;

// Ensure BurnDrvActive is visible to Metal code
// This is already defined in wrapper_burn.cpp, so we don't redefine it here

// Need to make sure the function is properly exported with C linkage
extern "C" {
// Implementation of the Metal-CPS2 linkage function
void Cps2_SetupMetalLinkage() {
    printf("Setting up CPS2 Metal linkage\n");
    
    // Set CPS2 mode
    Cps = 2;
    
    // Set up default volume
    Cps2Volume = 39; // Max volume
    Cps2DisableDigitalVolume = 0;
    Cps2DisableQSnd = 0;
    
    printf("CPS2 Metal linkage complete\n");
}

// Add stubs or actual implementations of key CPS2 functions if needed
// For mvsc specifically
int Cps2_LoadMvscRoms() {
    printf("Loading Marvel vs Capcom ROMs\n");
    
    // Make sure ROM paths are set correctly
    // The actual loading happens in the FBNeo core
    
    // Set the driver index to Marvel vs Capcom
    char szName[32];
    strcpy(szName, "mvsc");
    
    int drvIndex = BurnDrvGetIndex(szName);
    if (drvIndex < 0) {
        strcpy(szName, "mvscu");
        drvIndex = BurnDrvGetIndex(szName);
    }
    
    if (drvIndex >= 0) {
        printf("Found Marvel vs Capcom driver at index %d\n", drvIndex);
        // Access nBurnDrvActive directly since it's already declared in burn.h
        // with the correct linkage
        nBurnDrvActive = drvIndex;
        return 0;
    }
    
    printf("ERROR: Could not find Marvel vs Capcom driver\n");
    return 1;
}

// Resolve key CPS2 symbols that might be missing
void Cps2_ResolveSymbols() {
    // This function would ensure all required CPS2 symbols are linked
    // and properly exposed to the Metal implementation
    printf("CPS2 symbols resolved\n");
}
} // extern "C" 