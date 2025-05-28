#pragma once

// CPS Input System - simplified header for Metal port

#ifdef __cplusplus
extern "C" {
#endif

// Define CPS input arrays
extern unsigned char CpsInp000[0x10]; // Player 1
extern unsigned char CpsInp001[0x10]; // Player 2
extern unsigned char CpsInp011[0x10]; // Service switches
extern unsigned char CpsInp177[0x10]; // Player 3
extern unsigned char CpsInp179[0x10]; // Player 4
extern unsigned char CpsInpMisc[0x10]; // Misc switches

// Initialize the CPS input system
void CpsInputInit(void);

// Set a specific CPS input value
void CpsInputSetValue(int player, int input, int value);

// Map keyboard input to CPS input
void CpsInputMapKey(int keyCode, int player, int input);

// Map gamepad input to CPS input
void CpsInputMapGamepad(int gamepadIndex, int buttonIndex, int player, int input);

// Reset all CPS inputs to default state
void CpsInputReset(void);

// Process CPS inputs
void CpsInputUpdate(void);

// Check if a specific CPS input is active
int CpsInputIsActive(int player, int input);

#ifdef __cplusplus
}
#endif 