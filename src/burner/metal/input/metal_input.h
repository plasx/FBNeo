#ifndef _METAL_INPUT_H_
#define _METAL_INPUT_H_

// =============================================================================
// FBNeo Metal - Input System Interface
// =============================================================================
// This header defines the interface for the Metal-specific input system
// =============================================================================

#include "../fixes/metal_interop.h"
#include "../fixes/cps_input_bridge.h"

METAL_BEGIN_C_DECL

// -----------------------------------------------------------------------------
// Metal Input System
// -----------------------------------------------------------------------------

// Key definitions for macOS
#define KEY_UP          126
#define KEY_DOWN        125
#define KEY_LEFT        123
#define KEY_RIGHT       124
#define KEY_ESCAPE      53
#define KEY_RETURN      36
#define KEY_SPACE       49

// Define FBNeo key codes to map to 
#define FBK_UP          0x4001
#define FBK_DOWN        0x4002
#define FBK_LEFT        0x4003
#define FBK_RIGHT       0x4004
#define FBK_ESCAPE      0x4005
#define FBK_RETURN      0x4006
#define FBK_SPACE       0x4007
#define FBK_A           0x4008
#define FBK_B           0x4009
#define FBK_C           0x400A
#define FBK_D           0x400B
#define FBK_E           0x400C
#define FBK_F           0x400D
#define FBK_G           0x400E
#define FBK_H           0x400F
#define FBK_I           0x4010
#define FBK_J           0x4011
#define FBK_K           0x4012
#define FBK_L           0x4013
#define FBK_M           0x4014
#define FBK_N           0x4015
#define FBK_O           0x4016
#define FBK_P           0x4017
#define FBK_Q           0x4018
#define FBK_R           0x4019
#define FBK_S           0x401A
#define FBK_T           0x401B
#define FBK_U           0x401C
#define FBK_V           0x401D
#define FBK_W           0x401E
#define FBK_X           0x401F
#define FBK_Y           0x4020
#define FBK_Z           0x4021
#define FBK_0           0x4022
#define FBK_1           0x4023
#define FBK_2           0x4024
#define FBK_3           0x4025
#define FBK_4           0x4026
#define FBK_5           0x4027
#define FBK_6           0x4028
#define FBK_7           0x4029
#define FBK_8           0x402A
#define FBK_9           0x402B
#define FBK_F1          0x402C
#define FBK_F2          0x402D
#define FBK_F3          0x402E
#define FBK_F4          0x402F
#define FBK_F5          0x4030
#define FBK_F6          0x4031
#define FBK_F7          0x4032
#define FBK_F8          0x4033
#define FBK_F9          0x4034
#define FBK_F10         0x4035
#define FBK_F11         0x4036
#define FBK_F12         0x4037
#define FBK_NUMPAD0     0x4038
#define FBK_NUMPAD1     0x4039
#define FBK_NUMPAD2     0x403A
#define FBK_NUMPAD3     0x403B
#define FBK_NUMPAD4     0x403C
#define FBK_NUMPAD5     0x403D
#define FBK_NUMPAD6     0x403E
#define FBK_NUMPAD7     0x403F
#define FBK_NUMPAD8     0x4040
#define FBK_NUMPAD9     0x4041

// -----------------------------------------------------------------------------
// Core Input Functions - C Interface
// -----------------------------------------------------------------------------

// Initialize the input system
INT32 Metal_InputInit();

// Update input state
INT32 Metal_InputUpdate();

// Shut down the input system
INT32 Metal_InputExit();

// Process a keyboard key press
void Metal_KeyDown(INT32 keyCode);

// Process a keyboard key release
void Metal_KeyUp(INT32 keyCode);

// Set a gamepad button state
void Metal_GamepadButtonDown(INT32 playerIndex, INT32 buttonIndex);

// Release a gamepad button
void Metal_GamepadButtonUp(INT32 playerIndex, INT32 buttonIndex);

// Set a gamepad axis value
void Metal_GamepadAxisMove(INT32 playerIndex, INT32 axisIndex, float value);

// Register a new gamepad
INT32 Metal_GamepadConnected(INT32 playerIndex);

// Handle gamepad disconnect
void Metal_GamepadDisconnected(INT32 playerIndex);

// Get the current state of a particular input
INT32 Metal_GetInputState(INT32 inputCode);

// Map inputs to CPS input arrays
void Metal_MapInputsToCPS();

METAL_END_C_DECL

#endif // _METAL_INPUT_H_ 