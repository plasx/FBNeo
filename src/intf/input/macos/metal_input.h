#ifndef _METAL_INPUT_H_
#define _METAL_INPUT_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Input device types
typedef enum {
    InputDeviceKeyboard = 0,
    InputDeviceGamepad,
    InputDeviceMouse,
    InputDeviceTouch,
    InputDeviceCount
} InputDeviceType;

// Input device state
typedef struct {
    bool connected;
    int deviceID;
    InputDeviceType type;
    void* deviceRef; // Platform-specific device reference
} InputDevice;

// Keyboard specific state
typedef struct {
    uint8_t keyState[256]; // State for up to 256 keys
    bool modifierState[8]; // Shift, Ctrl, Alt, Command, etc.
} KeyboardState;

// Mouse specific state
typedef struct {
    int x, y;
    int deltaX, deltaY;
    int wheel;
    bool buttonState[5]; // Left, right, middle, back, forward
} MouseState;

// Gamepad specific state
typedef struct {
    float leftStickX, leftStickY;
    float rightStickX, rightStickY;
    float leftTrigger, rightTrigger;
    bool buttonState[16]; // Up to 16 buttons
} GamepadState;

// Combined input state
typedef struct {
    KeyboardState keyboard;
    MouseState mouse;
    GamepadState gamepad[4]; // Support for up to 4 gamepads
    bool isGamepadActive;
    int activeGamepad; // Index of the active gamepad (0-3)
} InputState;

// Mapping from hardware key codes to FBNeo key codes
typedef struct {
    int hardwareKeyCode;
    int fbKeyCode;
} KeyMapping;

// Key binding profile
typedef struct {
    const char* profileName;
    KeyMapping mappings[64]; // Up to 64 mappings per profile
    int mappingCount;
} KeyBindingProfile;

// Function prototypes

// Initialize the input system
int Metal_InitInput();

// Shutdown the input system
void Metal_ExitInput();

// Process a key event
void Metal_ProcessKeyEvent(int keyCode, bool keyDown);

// Process a mouse event
void Metal_ProcessMouseEvent(int button, int x, int y, bool buttonDown);

// Process a gamepad event
void Metal_ProcessGamepadEvent(int gamepadIndex, int buttonIndex, float value);

// Update input state (called once per frame)
void Metal_UpdateInput();

// Check if a key is down
bool Metal_IsKeyDown(int keyCode);

// Check if a key was just pressed this frame
bool Metal_IsKeyPressed(int keyCode);

// Check if a key was just released this frame
bool Metal_IsKeyReleased(int keyCode);

// Get the current mouse position
void Metal_GetMousePosition(int* x, int* y);

// Set the mouse position
void Metal_SetMousePosition(int x, int y);

// Load key binding profiles
void Metal_LoadKeyBindings();

// Save key binding profiles
void Metal_SaveKeyBindings();

// Set active key binding profile
void Metal_SetKeyBindingProfile(const char* profileName);

// Define a new key binding
void Metal_DefineKeyBinding(int hardwareKeyCode, int fbKeyCode);

// Remove a key binding
void Metal_RemoveKeyBinding(int hardwareKeyCode);

// Get the active input state
InputState* Metal_GetInputState();

#ifdef __cplusplus
}
#endif

#endif // _METAL_INPUT_H_ 