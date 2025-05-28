#include "cps_input_full.h"
#include <string.h>
#include <stdio.h>

// Define CPS input arrays
unsigned char CpsInp000[0x10] = {0}; // Player 1
unsigned char CpsInp001[0x10] = {0}; // Player 2
unsigned char CpsInp011[0x10] = {0}; // Service switches
unsigned char CpsInp177[0x10] = {0}; // Player 3
unsigned char CpsInp179[0x10] = {0}; // Player 4
unsigned char CpsInpMisc[0x10] = {0}; // Misc switches

// Initialize the CPS input system
void CpsInputInit(void) {
    memset(CpsInp000, 0, sizeof(CpsInp000));
    memset(CpsInp001, 0, sizeof(CpsInp001));
    memset(CpsInp011, 0, sizeof(CpsInp011));
    memset(CpsInp177, 0, sizeof(CpsInp177));
    memset(CpsInp179, 0, sizeof(CpsInp179));
    memset(CpsInpMisc, 0, sizeof(CpsInpMisc));
    
    printf("CPS Input System initialized\n");
}

// Set a specific CPS input value
void CpsInputSetValue(int player, int input, int value) {
    unsigned char* inputArray = NULL;
    
    // Select the appropriate input array based on player
    switch (player) {
        case 0: inputArray = CpsInp000; break; // Player 1
        case 1: inputArray = CpsInp001; break; // Player 2
        case 2: inputArray = CpsInp177; break; // Player 3
        case 3: inputArray = CpsInp179; break; // Player 4
        case -1: inputArray = CpsInp011; break; // System inputs
        default: return; // Invalid player
    }
    
    // Set the input value
    if (input >= 0 && input < 0x10) {
        if (value) {
            inputArray[input / 8] |= (1 << (input % 8));
        } else {
            inputArray[input / 8] &= ~(1 << (input % 8));
        }
    }
}

// Map keyboard input to CPS input
void CpsInputMapKey(int keyCode, int player, int input) {
    // This would normally map a key to a CPS input
    // For our minimal implementation, we just log it
    printf("CPS Input: Mapping key %d to player %d, input %d\n", keyCode, player, input);
}

// Map gamepad input to CPS input
void CpsInputMapGamepad(int gamepadIndex, int buttonIndex, int player, int input) {
    // This would normally map a gamepad button to a CPS input
    // For our minimal implementation, we just log it
    printf("CPS Input: Mapping gamepad %d button %d to player %d, input %d\n", 
           gamepadIndex, buttonIndex, player, input);
}

// Reset all CPS inputs to default state
void CpsInputReset(void) {
    CpsInputInit();
}

// Process CPS inputs
void CpsInputUpdate(void) {
    // This would normally read the hardware inputs and update CPS arrays
    // For our minimal implementation, we do nothing
}

// Check if a specific CPS input is active
int CpsInputIsActive(int player, int input) {
    unsigned char* inputArray = NULL;
    
    // Select the appropriate input array based on player
    switch (player) {
        case 0: inputArray = CpsInp000; break; // Player 1
        case 1: inputArray = CpsInp001; break; // Player 2
        case 2: inputArray = CpsInp177; break; // Player 3
        case 3: inputArray = CpsInp179; break; // Player 4
        case -1: inputArray = CpsInp011; break; // System inputs
        default: return 0; // Invalid player
    }
    
    // Check the input value
    if (input >= 0 && input < 0x10) {
        return (inputArray[input / 8] & (1 << (input % 8))) ? 1 : 0;
    }
    
    return 0;
} 