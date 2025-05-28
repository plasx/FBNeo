#ifndef _METAL_INPUT_H_
#define _METAL_INPUT_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Apple Virtual Key Codes
enum {
    kVK_ANSI_A                    = 0x00,
    kVK_ANSI_S                    = 0x01,
    kVK_ANSI_D                    = 0x02,
    kVK_ANSI_F                    = 0x03,
    kVK_ANSI_H                    = 0x04,
    kVK_ANSI_G                    = 0x05,
    kVK_ANSI_Z                    = 0x06,
    kVK_ANSI_X                    = 0x07,
    kVK_ANSI_C                    = 0x08,
    kVK_ANSI_V                    = 0x09,
    kVK_ANSI_B                    = 0x0B,
    kVK_ANSI_Q                    = 0x0C,
    kVK_ANSI_W                    = 0x0D,
    kVK_ANSI_E                    = 0x0E,
    kVK_ANSI_R                    = 0x0F,
    kVK_ANSI_Y                    = 0x10,
    kVK_ANSI_T                    = 0x11,
    kVK_ANSI_1                    = 0x12,
    kVK_ANSI_2                    = 0x13,
    kVK_ANSI_3                    = 0x14,
    kVK_ANSI_4                    = 0x15,
    kVK_ANSI_6                    = 0x16,
    kVK_ANSI_5                    = 0x17,
    kVK_ANSI_Equal                = 0x18,
    kVK_ANSI_9                    = 0x19,
    kVK_ANSI_7                    = 0x1A,
    kVK_ANSI_Minus                = 0x1B,
    kVK_ANSI_8                    = 0x1C,
    kVK_ANSI_0                    = 0x1D,
    kVK_ANSI_RightBracket         = 0x1E,
    kVK_ANSI_O                    = 0x1F,
    kVK_ANSI_U                    = 0x20,
    kVK_ANSI_LeftBracket          = 0x21,
    kVK_ANSI_I                    = 0x22,
    kVK_ANSI_P                    = 0x23,
    kVK_ANSI_L                    = 0x25,
    kVK_ANSI_J                    = 0x26,
    kVK_ANSI_Quote                = 0x27,
    kVK_ANSI_K                    = 0x28,
    kVK_ANSI_Semicolon            = 0x29,
    kVK_ANSI_Backslash            = 0x2A,
    kVK_ANSI_Comma                = 0x2B,
    kVK_ANSI_Slash                = 0x2C,
    kVK_ANSI_N                    = 0x2D,
    kVK_ANSI_M                    = 0x2E,
    kVK_ANSI_Period               = 0x2F,
    kVK_ANSI_Grave                = 0x32,
    kVK_ANSI_KeypadDecimal        = 0x41,
    kVK_ANSI_KeypadMultiply       = 0x43,
    kVK_ANSI_KeypadPlus           = 0x45,
    kVK_ANSI_KeypadClear          = 0x47,
    kVK_ANSI_KeypadDivide         = 0x4B,
    kVK_ANSI_KeypadEnter          = 0x4C,
    kVK_ANSI_KeypadMinus          = 0x4E,
    kVK_ANSI_KeypadEquals         = 0x51,
    kVK_ANSI_Keypad0              = 0x52,
    kVK_ANSI_Keypad1              = 0x53,
    kVK_ANSI_Keypad2              = 0x54,
    kVK_ANSI_Keypad3              = 0x55,
    kVK_ANSI_Keypad4              = 0x56,
    kVK_ANSI_Keypad5              = 0x57,
    kVK_ANSI_Keypad6              = 0x58,
    kVK_ANSI_Keypad7              = 0x59,
    kVK_ANSI_Keypad8              = 0x5B,
    kVK_ANSI_Keypad9              = 0x5C,
    
    /* Keycodes for keys that are independent of keyboard layout */
    kVK_Return                    = 0x24,
    kVK_Tab                       = 0x30,
    kVK_Space                     = 0x31,
    kVK_Delete                    = 0x33,
    kVK_Escape                    = 0x35,
    kVK_Command                   = 0x37,
    kVK_Shift                     = 0x38,
    kVK_CapsLock                  = 0x39,
    kVK_Option                    = 0x3A,
    kVK_Control                   = 0x3B,
    kVK_RightShift                = 0x3C,
    kVK_RightOption               = 0x3D,
    kVK_RightControl              = 0x3E,
    kVK_Function                  = 0x3F,
    kVK_F17                       = 0x40,
    kVK_VolumeUp                  = 0x48,
    kVK_VolumeDown                = 0x49,
    kVK_Mute                      = 0x4A,
    kVK_F18                       = 0x4F,
    kVK_F19                       = 0x50,
    kVK_F20                       = 0x5A,
    kVK_F5                        = 0x60,
    kVK_F6                        = 0x61,
    kVK_F7                        = 0x62,
    kVK_F3                        = 0x63,
    kVK_F8                        = 0x64,
    kVK_F9                        = 0x65,
    kVK_F11                       = 0x67,
    kVK_F13                       = 0x69,
    kVK_F16                       = 0x6A,
    kVK_F14                       = 0x6B,
    kVK_F10                       = 0x6D,
    kVK_F12                       = 0x6F,
    kVK_F15                       = 0x71,
    kVK_Help                      = 0x72,
    kVK_Home                      = 0x73,
    kVK_PageUp                    = 0x74,
    kVK_ForwardDelete             = 0x75,
    kVK_F4                        = 0x76,
    kVK_End                       = 0x77,
    kVK_F2                        = 0x78,
    kVK_PageDown                  = 0x79,
    kVK_F1                        = 0x7A,
    kVK_LeftArrow                 = 0x7B,
    kVK_RightArrow                = 0x7C,
    kVK_DownArrow                 = 0x7D,
    kVK_UpArrow                   = 0x7E
};

// FBNeo Key Codes (aligned with the main FBNeo codebase)
enum {
    FBK_ESCAPE        = 1,
    FBK_1             = 2,
    FBK_2             = 3,
    FBK_3             = 4,
    FBK_4             = 5,
    FBK_5             = 6,
    FBK_6             = 7,
    FBK_7             = 8,
    FBK_8             = 9,
    FBK_9             = 10,
    FBK_0             = 11,
    FBK_MINUS         = 12,
    FBK_EQUALS        = 13,
    FBK_BACK          = 14,
    FBK_TAB           = 15,
    FBK_Q             = 16,
    FBK_W             = 17,
    FBK_E             = 18,
    FBK_R             = 19,
    FBK_T             = 20,
    FBK_Y             = 21,
    FBK_U             = 22,
    FBK_I             = 23,
    FBK_O             = 24,
    FBK_P             = 25,
    FBK_LBRACKET      = 26,
    FBK_RBRACKET      = 27,
    FBK_ENTER         = 28,
    FBK_LCONTROL      = 29,
    FBK_A             = 30,
    FBK_S             = 31,
    FBK_D             = 32,
    FBK_F             = 33,
    FBK_G             = 34,
    FBK_H             = 35,
    FBK_J             = 36,
    FBK_K             = 37,
    FBK_L             = 38,
    FBK_SEMICOLON     = 39,
    FBK_APOSTROPHE    = 40,
    FBK_GRAVE         = 41,
    FBK_LSHIFT        = 42,
    FBK_BACKSLASH     = 43,
    FBK_Z             = 44,
    FBK_X             = 45,
    FBK_C             = 46,
    FBK_V             = 47,
    FBK_B             = 48,
    FBK_N             = 49,
    FBK_M             = 50,
    FBK_COMMA         = 51,
    FBK_PERIOD        = 52,
    FBK_SLASH         = 53,
    FBK_RSHIFT        = 54,
    FBK_MULTIPLY      = 55,
    FBK_LALT          = 56,
    FBK_SPACE         = 57,
    FBK_CAPITAL       = 58,
    FBK_F1            = 59,
    FBK_F2            = 60,
    FBK_F3            = 61,
    FBK_F4            = 62,
    FBK_F5            = 63,
    FBK_F6            = 64,
    FBK_F7            = 65,
    FBK_F8            = 66,
    FBK_F9            = 67,
    FBK_F10           = 68,
    FBK_NUMLOCK       = 69,
    FBK_SCROLL        = 70,
    FBK_NUMPAD7       = 71,
    FBK_NUMPAD8       = 72,
    FBK_NUMPAD9       = 73,
    FBK_SUBTRACT      = 74,
    FBK_NUMPAD4       = 75,
    FBK_NUMPAD5       = 76,
    FBK_NUMPAD6       = 77,
    FBK_ADD           = 78,
    FBK_NUMPAD1       = 79,
    FBK_NUMPAD2       = 80,
    FBK_NUMPAD3       = 81,
    FBK_NUMPAD0       = 82,
    FBK_DECIMAL       = 83,
    FBK_OEM_102       = 86,
    FBK_F11           = 87,
    FBK_F12           = 88,
    FBK_HOME          = 97,
    FBK_UP            = 98,
    FBK_PRIOR         = 99,
    FBK_LEFT          = 100,
    FBK_RIGHT         = 101,
    FBK_END           = 102,
    FBK_DOWN          = 103,
    FBK_NEXT          = 104,
    FBK_INSERT        = 105,
    FBK_DELETE        = 106,
    FBK_LWIN          = 107,
    FBK_RWIN          = 108,
    FBK_APPS          = 109,
    FBK_RCONTROL      = 128,
    FBK_RALT          = 129,
    FBK_PAUSE         = 130,
    FBK_PGUP          = 131,
    FBK_PGDN          = 132,
    FBK_RETURN        = 156,
    FBK_NUMPADENTER   = 156,
    FBK_RCONTROL_EX   = 157
};

// Function declarations
void* Metal_CreateKeyboardView(NSRect frame);
void Metal_ReleaseKeyboardView(void* viewPtr);
int Metal_IsKeyDown(unsigned short keyCode);
int Metal_HandleKeyDown(int keyCode);
int Metal_HandleKeyUp(int keyCode);
int Metal_InitKeyboardInput();
int Metal_ExitKeyboardInput();

// Input handling functions
int Metal_InputKeyDown(int keyCode);
int Metal_InputKeyUp(int keyCode);
int Metal_InitInput();
int Metal_ExitInput();
int Metal_InitControllerSupport();
int Metal_ExitControllerSupport();
int Metal_GetControllerCount();

// Initialize input system
int MetalInput_Init();

// Shutdown input system
int MetalInput_Exit();

// Process input (call every frame)
int MetalInput_Make(bool bCopy);

// Set key state
void MetalInput_SetKeyState(int key, bool isPressed);

// Set joypad state
void MetalInput_SetJoypadState(int player, int button, bool isPressed);

// Set axis state
void MetalInput_SetAxisState(int player, int axis, int value);

// Key map management
void MetalInput_SetDefaultKeymap();
void MetalInput_LoadKeymap(const char* filename);
void MetalInput_SaveKeymap(const char* filename);

// Input device management
int MetalInput_GetDeviceCount();
const char* MetalInput_GetDeviceName(int index);
void MetalInput_SetActiveDevice(int index);

#ifdef __cplusplus
}
#endif

#endif // _METAL_INPUT_H_ 