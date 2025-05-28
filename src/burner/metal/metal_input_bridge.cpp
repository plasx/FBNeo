#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "metal_compat_layer.h"
#include "metal_input_defs.h"

// CPS2 input variables
extern "C" {
    extern UINT8 CpsReset;
    extern UINT8 CpsInp000[8];  // Player 2 controls
    extern UINT8 CpsInp001[8];  // Player 1 controls
    extern UINT8 CpsInp010[8];  // Extra controls
    extern UINT8 CpsInp011[8];  // More player controls
    extern UINT8 CpsInp018[8];  // More system controls
    extern UINT8 CpsInp020[8];  // Coin/Start
    extern UINT8 CpsInp021[8];  // Service/Diagnostic
    extern UINT8 CpsInp119[8];  // Player 3 controls for some games
}

// Input mapping for CPS2
// This maps the generic input IDs to the specific CPS2 input bits
void Metal_MapInputToCPS2(INT32 inputId, INT32 state) {
    // State should be 0 (released) or 1 (pressed)
    UINT8 value = state ? 1 : 0;
    
    switch (inputId) {
        // Player 1 controls
        case P1_UP:          CpsInp001[3] = value; break;
        case P1_DOWN:        CpsInp001[2] = value; break;
        case P1_LEFT:        CpsInp001[1] = value; break;
        case P1_RIGHT:       CpsInp001[0] = value; break;
        case P1_WEAK_PUNCH:  CpsInp001[4] = value; break;
        case P1_MED_PUNCH:   CpsInp001[5] = value; break;
        case P1_STRONG_PUNCH:CpsInp001[6] = value; break;
        case P1_WEAK_KICK:   CpsInp011[0] = value; break;
        case P1_MED_KICK:    CpsInp011[1] = value; break;
        case P1_STRONG_KICK: CpsInp011[2] = value; break;
        case P1_START:       CpsInp020[0] = value; break;
        case P1_COIN:        CpsInp020[4] = value; break;
        
        // Player 2 controls
        case P2_UP:          CpsInp000[3] = value; break;
        case P2_DOWN:        CpsInp000[2] = value; break;
        case P2_LEFT:        CpsInp000[1] = value; break;
        case P2_RIGHT:       CpsInp000[0] = value; break;
        case P2_WEAK_PUNCH:  CpsInp000[4] = value; break;
        case P2_MED_PUNCH:   CpsInp000[5] = value; break;
        case P2_STRONG_PUNCH:CpsInp000[6] = value; break;
        case P2_WEAK_KICK:   CpsInp011[4] = value; break;
        case P2_MED_KICK:    CpsInp011[5] = value; break;
        case P2_STRONG_KICK: CpsInp011[6] = value; break;
        case P2_START:       CpsInp020[1] = value; break;
        case P2_COIN:        CpsInp020[5] = value; break;
        
        // System controls
        case RESET:          CpsReset = value; break;
        case DIAGNOSTIC:     CpsInp021[1] = value; break;
        case SERVICE:        CpsInp021[2] = value; break;
        
        default:
            // Unknown input ID
            break;
    }
}

// Clear all CPS2 inputs
void Metal_ClearCPS2Inputs() {
    CpsReset = 0;
    memset(CpsInp000, 0, sizeof(CpsInp000));
    memset(CpsInp001, 0, sizeof(CpsInp001));
    memset(CpsInp010, 0, sizeof(CpsInp010));
    memset(CpsInp011, 0, sizeof(CpsInp011));
    memset(CpsInp018, 0, sizeof(CpsInp018));
    memset(CpsInp020, 0, sizeof(CpsInp020));
    memset(CpsInp021, 0, sizeof(CpsInp021));
    memset(CpsInp119, 0, sizeof(CpsInp119));
}

// BurnDrvSetInput implementation that maps to CPS2
extern "C" INT32 BurnDrvSetInput(INT32 i, INT32 nState) {
    // Map the input to CPS2
    Metal_MapInputToCPS2(i, nState);
    return 0;
}

// Initialize input system
extern "C" INT32 BurnInputInit() {
    printf("[BurnInputInit] Initializing input system\n");
    Metal_ClearCPS2Inputs();
    return 0;
}

// Exit input system
extern "C" INT32 BurnInputExit() {
    printf("[BurnInputExit] Exiting input system\n");
    Metal_ClearCPS2Inputs();
    return 0;
}

// Print current input state (for debugging)
void Metal_PrintCPS2InputState() {
    printf("=== CPS2 Input State ===\n");
    printf("CpsReset: %d\n", CpsReset);
    
    printf("CpsInp000 (P2): ");
    for (int i = 0; i < 8; i++) printf("%d ", CpsInp000[i]);
    printf("\n");
    
    printf("CpsInp001 (P1): ");
    for (int i = 0; i < 8; i++) printf("%d ", CpsInp001[i]);
    printf("\n");
    
    printf("CpsInp011 (Kicks): ");
    for (int i = 0; i < 8; i++) printf("%d ", CpsInp011[i]);
    printf("\n");
    
    printf("CpsInp020 (System): ");
    for (int i = 0; i < 8; i++) printf("%d ", CpsInp020[i]);
    printf("\n");
    
    printf("CpsInp021 (Service): ");
    for (int i = 0; i < 8; i++) printf("%d ", CpsInp021[i]);
    printf("\n");
    
    printf("=======================\n");
} 