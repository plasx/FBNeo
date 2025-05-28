#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "metal_input_defs.h"

// Define the types used by FBNeo
typedef unsigned char UINT8;
typedef int INT32;

// Input buffers for CPS2 games
UINT8 CpsInp000[8] = {0}; // Player 2 controls
UINT8 CpsInp001[8] = {0}; // Player 1 controls
UINT8 CpsInp010[8] = {0}; // Extra controls
UINT8 CpsInp011[8] = {0}; // More player controls
UINT8 CpsInp018[8] = {0}; // More system controls
UINT8 CpsInp020[8] = {0}; // Coin/Start
UINT8 CpsInp021[8] = {0}; // Service/Diagnostic
UINT8 CpsInp119[8] = {0}; // Player 3 controls for some games
UINT8 CpsReset = 0;      // Reset signal

// Debug buffer to track which inputs are active
static UINT8 g_inputDebug[32] = {0};

// Set input for the FBNeo emulation core
extern "C" INT32 BurnDrvSetInput(INT32 i, INT32 nState) {
    // Special case for debugging - track which inputs are being used
    if (i >= 0 && i < 32) {
        g_inputDebug[i] = (UINT8)nState;
    }
    
    // Map the input to the correct CPS2 input buffer
    switch (i) {
        // Player 1 Controls
        case P1_UP:          CpsInp001[3] = (UINT8)nState; break;
        case P1_DOWN:        CpsInp001[2] = (UINT8)nState; break;
        case P1_LEFT:        CpsInp001[1] = (UINT8)nState; break;
        case P1_RIGHT:       CpsInp001[0] = (UINT8)nState; break;
        case P1_WEAK_PUNCH:  CpsInp001[4] = (UINT8)nState; break;
        case P1_MED_PUNCH:   CpsInp001[5] = (UINT8)nState; break;
        case P1_STRONG_PUNCH:CpsInp001[6] = (UINT8)nState; break;
        case P1_WEAK_KICK:   CpsInp011[0] = (UINT8)nState; break;
        case P1_MED_KICK:    CpsInp011[1] = (UINT8)nState; break;
        case P1_STRONG_KICK: CpsInp011[2] = (UINT8)nState; break;
        case P1_START:       CpsInp020[0] = (UINT8)nState; break;
        case P1_COIN:        CpsInp020[4] = (UINT8)nState; break;
        
        // Player 2 Controls
        case P2_UP:          CpsInp000[3] = (UINT8)nState; break;
        case P2_DOWN:        CpsInp000[2] = (UINT8)nState; break;
        case P2_LEFT:        CpsInp000[1] = (UINT8)nState; break;
        case P2_RIGHT:       CpsInp000[0] = (UINT8)nState; break;
        case P2_WEAK_PUNCH:  CpsInp000[4] = (UINT8)nState; break;
        case P2_MED_PUNCH:   CpsInp000[5] = (UINT8)nState; break;
        case P2_STRONG_PUNCH:CpsInp000[6] = (UINT8)nState; break;
        case P2_WEAK_KICK:   CpsInp011[4] = (UINT8)nState; break;
        case P2_MED_KICK:    CpsInp011[5] = (UINT8)nState; break;
        case P2_STRONG_KICK: CpsInp020[6] = (UINT8)nState; break;
        case P2_START:       CpsInp020[1] = (UINT8)nState; break;
        case P2_COIN:        CpsInp020[5] = (UINT8)nState; break;
        
        // System Controls
        case RESET:          CpsReset      = (UINT8)nState; break;
        case DIAGNOSTIC:     CpsInp021[1]  = (UINT8)nState; break;
        case SERVICE:        CpsInp021[2]  = (UINT8)nState; break;
        
        default:
            // Unhandled input
            return 1;
    }
    
    // Occasionally print input status for debugging
    static int debugCounter = 0;
    if (++debugCounter >= 600) {  // Once every 10 seconds at 60 FPS
        debugCounter = 0;
        
        // Count active inputs
        int activeInputs = 0;
        for (int j = 0; j < 32; j++) {
            if (g_inputDebug[j]) {
                activeInputs++;
            }
        }
        
        if (activeInputs > 0) {
            printf("[BurnDrvSetInput] Active inputs (%d):", activeInputs);
            for (int j = 0; j < 32; j++) {
                if (g_inputDebug[j]) {
                    printf(" %d", j);
                }
            }
            printf("\n");
        }
    }
    
    return 0; // Success
}

// Get the count of active inputs for debug display
extern "C" int Metal_GetActiveInputs() {
    int count = 0;
    for (int i = 0; i < 32; i++) {
        if (g_inputDebug[i]) {
            count++;
        }
    }
    return count;
}

// Reset the emulation
extern "C" INT32 BurnDrvReset() {
    printf("[BurnDrvReset] Resetting emulation\n");
    
    // Clear all input buffers
    memset(CpsInp000, 0, sizeof(CpsInp000));
    memset(CpsInp001, 0, sizeof(CpsInp001));
    memset(CpsInp010, 0, sizeof(CpsInp010));
    memset(CpsInp011, 0, sizeof(CpsInp011));
    memset(CpsInp018, 0, sizeof(CpsInp018));
    memset(CpsInp020, 0, sizeof(CpsInp020));
    memset(CpsInp021, 0, sizeof(CpsInp021));
    memset(CpsInp119, 0, sizeof(CpsInp119));
    CpsReset = 0;
    
    return 0; // Success
} 