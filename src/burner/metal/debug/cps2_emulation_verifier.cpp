// cps2_emulation_verifier.cpp
// Minimal emulation verification for FBNeo Metal build

// Use minimal includes to avoid conflicts
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// Use forward declarations instead of including core headers
// This avoids the conflicts between Metal and FBNeo core

// Basic type definitions if not already defined
#ifndef UINT8_DEFINED
typedef unsigned char UINT8;
#define UINT8_DEFINED
#endif

#ifndef UINT32_DEFINED
typedef unsigned int UINT32;
#define UINT32_DEFINED
#endif

#ifndef INT32_DEFINED
typedef int INT32;
#define INT32_DEFINED
#endif

// Forward declarations for CPS2 functions we need
#ifdef __cplusplus
extern "C" {
#endif

    // FBNeo globals from burn.h needed by this file
    extern int nBurnSoundRate;
    extern int nBurnDrvActive;
    
    // External function declarations
    const char* BurnDrvGetTextA(UINT32 i);
    INT32 BurnDrvGetHardwareCode();
    
    // CPS2 specific functions (defined in cps.h)
    extern INT32 Cps;
    extern INT32 Cps2DisableQSnd;
    
    // Simplified stubs for verification
    INT32 VerifyCPS2System();

#ifdef __cplusplus
}
#endif

// CPS2 emulation verification functions
INT32 VerifyCPS2System() {
    printf("=== CPS2 Emulation Verification ===\n");
    
    // Check if we're running a CPS2 game
    INT32 hwCode = BurnDrvGetHardwareCode();
    printf("Hardware Code: 0x%08X\n", hwCode);
    
    // Print game name
    printf("Game: %s\n", BurnDrvGetTextA(0));
    
    // Check CPS version
    printf("CPS Version: %d\n", Cps);
    
    // More verification can be added here
    
    return 0;
}

// Main verification function that can be called from Metal code
extern "C" INT32 Metal_VerifyCPS2Emulation() {
    return VerifyCPS2System();
} 