// Final Burn Neo - Metal build wrapper for burn.cpp
// This file connects the Metal UI to the FBNeo core

// Standard includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

// Include FBNeo core headers
#include "burnint.h"

// Include Metal-specific headers
#include "burner/metal/metal_wrappers.h"
#include "burner/metal/burner_metal.h"

// Access external variables from the FBNeo core directly
// Don't define them with different linkage
extern UINT32 nBurnDrvCount;
// Use declaration from burnint.h
extern INT32 nInterpolation;
extern INT32 nFMInterpolation;
extern UINT8 nBurnLayer;
extern UINT8 nSpriteEnable;

// Forward declare BurnDrvReset to fix the undeclared identifier
extern "C" INT32 BurnDrvReset();

// External variables for rendering and audio
extern UINT8* pBurnDraw;
extern INT32 nBurnPitch;
extern INT32 nBurnBpp;
extern INT32 nBurnSoundRate;
extern INT32 nBurnSoundLen;
extern INT16* pBurnSoundOut;
extern INT32 nMaxPlayers;
extern bool bSaveCROs;

// Get driver index by name - implements proper driver lookup
extern "C" INT32 BurnDrvGetIndexByName(const char* szName) {
    if (!szName || !szName[0]) {
        return -1;
    }
    
    // Case-insensitive comparison
    char lowerName[256];
    strncpy(lowerName, szName, 255);
    lowerName[255] = '\0';
    
    // Convert to lowercase for case-insensitive comparison
    for (int i = 0; lowerName[i]; i++) {
        lowerName[i] = tolower(lowerName[i]);
    }
    
    // Store the current active driver
    UINT32 currentActive = nBurnDrvActive;
    
    // Search through all drivers
    for (UINT32 i = 0; i < nBurnDrvCount; i++) {
        nBurnDrvActive = i;
        
        // Get the driver name and convert to lowercase
        const char* drvName = BurnDrvGetTextA(DRV_NAME);
        if (!drvName) continue;
        
        char lowerDrvName[256];
        strncpy(lowerDrvName, drvName, 255);
        lowerDrvName[255] = '\0';
        
        for (int j = 0; lowerDrvName[j]; j++) {
            lowerDrvName[j] = tolower(lowerDrvName[j]);
        }
        
        // Check for exact match
        if (strcmp(lowerDrvName, lowerName) == 0) {
            // Restore the original active driver
            nBurnDrvActive = currentActive;
            return i;
        }
        
        // Also check parent name
        const char* parentName = BurnDrvGetTextA(DRV_PARENT);
        if (parentName && parentName[0]) {
            char lowerParentName[256];
            strncpy(lowerParentName, parentName, 255);
            lowerParentName[255] = '\0';
            
            for (int j = 0; lowerParentName[j]; j++) {
                lowerParentName[j] = tolower(lowerParentName[j]);
            }
            
            if (strcmp(lowerParentName, lowerName) == 0) {
                // Restore the original active driver
                nBurnDrvActive = currentActive;
                return i;
            }
        }
        
        // Check if the driver name contains the search name
        // Useful for ROMs like "sf2ce" matching "sf2ceea", etc.
        if (strstr(lowerDrvName, lowerName) == lowerDrvName) {
            // Restore the original active driver
            nBurnDrvActive = currentActive;
            return i;
        }
    }
    
    // Restore the original active driver
    nBurnDrvActive = currentActive;
    return -1;
}

// Connect to the real FBNeo core
extern "C" {
    // BurnLib initialization and shutdown
    INT32 BurnLibInit_Metal() {
        printf("BurnLibInit_Metal: Initializing FBNeo core...\n");
        
        // Call the real BurnLibInit to initialize the emulation core
        INT32 result = BurnLibInit();
        
        if (result == 0) {
            printf("FBNeo core initialized successfully with %d drivers\n", nBurnDrvCount);
        } else {
            printf("Failed to initialize FBNeo core: %d\n", result);
        }
        
        return result;
    }
    
    INT32 BurnLibExit_Metal() {
        printf("BurnLibExit_Metal: Shutting down FBNeo core...\n");
        
        // Call the real BurnLibExit to shut down the emulation core
        return BurnLibExit();
    }
    
    // Driver initialization/exit/reset
    INT32 BurnDrvInit_Metal(INT32 nDrvNum) {
        printf("BurnDrvInit_Metal: Initializing driver #%d\n", nDrvNum);
        
        // Set the active driver index
        nBurnDrvActive = nDrvNum;
        
        // Call the real BurnDrvInit to initialize the driver
        INT32 result = BurnDrvInit();
        
        if (result == 0) {
            printf("Driver initialized successfully: %s\n", BurnDrvGetTextA(DRV_FULLNAME));
        } else {
            printf("Failed to initialize driver: %d\n", result);
        }
        
        return result;
    }
    
    INT32 BurnDrvExit_Metal() {
        printf("BurnDrvExit_Metal: Exiting driver\n");
        
        // Call the real BurnDrvExit to shut down the driver
        return BurnDrvExit();
    }
    
    INT32 BurnDrvFrame_Metal(INT32 bDraw) {
        // Call the real BurnDrvFrame to run one frame of emulation
        return BurnDrvFrame();
    }
    
    INT32 BurnDrvReset_Metal() {
        printf("BurnDrvReset_Metal: Resetting driver\n");
        
        // Call the real BurnDrvReset to reset the current driver
        return BurnDrvReset();
    }
    
    // Driver information retrieval
    char* BurnDrvGetTextA_Metal(UINT32 i) {
        // Get text from the current driver
        return BurnDrvGetTextA(i);
    }
    
    UINT32 BurnDrvGetFlags_Metal() {
        // Get flags from the current driver
        return BurnDrvGetFlags();
    }
    
    INT32 BurnDrvGetMaxPlayers_Metal() {
        // Get maximum number of players for the current driver
        return BurnDrvGetMaxPlayers();
    }
    
    INT32 BurnDrvGetGenreFlags_Metal() {
        // Get genre flags for the current driver
        return BurnDrvGetGenreFlags();
    }
    
    INT32 BurnDrvGetFamilyFlags_Metal() {
        // Get family flags for the current driver
        return BurnDrvGetFamilyFlags();
    }
    
    // ROM loading functions
    INT32 BurnLoadRom_Metal(UINT8* Dest, INT32 i, INT32 nGap) {
        // Use the real BurnLoadRom to load ROM data
        return BurnLoadRom(Dest, i, nGap);
    }
    
    INT32 BurnLoadRomExt_Metal(UINT8* Dest, INT32 i, INT32 nGap, INT32 nType) {
        // Forward to the main ROM loading function
        extern int BurnLoadRomExt(UINT8* Dest, INT32 i, INT32 nGap, INT32 nType);
        return BurnLoadRomExt(Dest, i, nGap, nType);
    }
    
    // Utility functions
    INT32 BurnAreaScan_Metal(INT32 nAction, INT32* pnMin) {
        // Metal stub for area scanning
        return 0;
    }
    
    INT32 BurnSpeedAdjust_Metal(INT32 cyc) {
        return cyc;
    }
    
    INT32 BurnSynchroniseStream_Metal(INT32 nSoundRate) {
        return 0;
    }
    
    void BurnSoundDCFilterReset_Metal() {
        // Forward to the main DC filter reset
        extern void BurnSoundDCFilterReset();
        BurnSoundDCFilterReset();
    }
} 