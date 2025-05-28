#ifndef GAMEPAD_SUPPORT_H
#define GAMEPAD_SUPPORT_H

#include "metal_declarations.h"
#include "input_mapping.h"

#ifdef __cplusplus
extern "C" {
#endif

// Gamepad button definitions (aligned with GameController framework)
typedef enum {
    GAMEPAD_BUTTON_A,
    GAMEPAD_BUTTON_B,
    GAMEPAD_BUTTON_X,
    GAMEPAD_BUTTON_Y,
    GAMEPAD_BUTTON_LEFT_SHOULDER,
    GAMEPAD_BUTTON_RIGHT_SHOULDER,
    GAMEPAD_BUTTON_LEFT_TRIGGER,
    GAMEPAD_BUTTON_RIGHT_TRIGGER,
    GAMEPAD_BUTTON_DPAD_UP,
    GAMEPAD_BUTTON_DPAD_DOWN,
    GAMEPAD_BUTTON_DPAD_LEFT,
    GAMEPAD_BUTTON_DPAD_RIGHT,
    GAMEPAD_BUTTON_MENU,
    GAMEPAD_BUTTON_OPTIONS,
    GAMEPAD_BUTTON_LEFT_THUMBSTICK,
    GAMEPAD_BUTTON_RIGHT_THUMBSTICK,
    GAMEPAD_BUTTON_HOME,
    GAMEPAD_BUTTON_COUNT
} GamepadButton;

// Gamepad axis definitions
typedef enum {
    GAMEPAD_AXIS_LEFT_X,
    GAMEPAD_AXIS_LEFT_Y,
    GAMEPAD_AXIS_RIGHT_X,
    GAMEPAD_AXIS_RIGHT_Y,
    GAMEPAD_AXIS_LEFT_TRIGGER,
    GAMEPAD_AXIS_RIGHT_TRIGGER,
    GAMEPAD_AXIS_COUNT
} GamepadAxis;

// Gamepad state
typedef struct {
    int connected;
    int buttonState[GAMEPAD_BUTTON_COUNT];
    float axisState[GAMEPAD_AXIS_COUNT];
    char name[64];
    int playerIndex;
} GamepadState;

// Initialize gamepad support
int Gamepad_Init(void);

// Shutdown gamepad support
void Gamepad_Shutdown(void);

// Update all gamepads (poll for new state)
void Gamepad_Update(void);

// Get the number of connected gamepads
int Gamepad_GetCount(void);

// Get the state of a specific gamepad
GamepadState* Gamepad_GetState(int gamepadIndex);

// Check if a button is pressed
int Gamepad_IsButtonPressed(int gamepadIndex, GamepadButton button);

// Get an axis value (-1.0 to 1.0)
float Gamepad_GetAxisValue(int gamepadIndex, GamepadAxis axis);

// Apply vibration/rumble (0.0 to 1.0 intensity)
void Gamepad_SetVibration(int gamepadIndex, float leftMotor, float rightMotor);

// Map a gamepad input to a player action
int Gamepad_MapInput(int gamepadIndex, int playerId, GamepadButton button, const char* actionName);

// Auto-assign gamepads to players
void Gamepad_AutoAssignPlayers(void);

// Handle gamepad connection/disconnection event
void Gamepad_HandleConnectionEvent(int gamepadIndex, int connected);

#ifdef __cplusplus
}
#endif

#endif // GAMEPAD_SUPPORT_H 