#ifndef INPUT_MAPPING_H
#define INPUT_MAPPING_H

#include "metal_declarations.h"

#ifdef __cplusplus
extern "C" {
#endif

// Input device types
typedef enum {
    INPUT_DEVICE_KEYBOARD,
    INPUT_DEVICE_GAMEPAD,
    INPUT_DEVICE_MOUSE,
    INPUT_DEVICE_LIGHTGUN
} InputDeviceType;

// Input action types
typedef enum {
    INPUT_ACTION_BUTTON,    // Regular button press
    INPUT_ACTION_AXIS,      // Analog axis (joystick, trigger)
    INPUT_ACTION_DPAD,      // Digital direction pad
    INPUT_ACTION_SPECIAL    // Special functions (pause, screenshot, etc.)
} InputActionType;

// Game types for preset configurations
typedef enum {
    GAME_TYPE_FIGHTING,     // Street Fighter, KOF, etc.
    GAME_TYPE_SHMUP,        // Shoot-em-ups
    GAME_TYPE_PLATFORMER,   // Platform games
    GAME_TYPE_PUZZLE,       // Puzzle games
    GAME_TYPE_RACING,       // Racing games
    GAME_TYPE_SPORTS,       // Sports games
    GAME_TYPE_LIGHTGUN,     // Light gun games
    GAME_TYPE_MAHJONG,      // Mahjong games
    GAME_TYPE_CUSTOM        // Custom mapping
} GameType;

// Input mapping structure
typedef struct {
    const char* name;           // Mapping name (e.g. "Jump", "Punch", etc.)
    InputActionType actionType; // Type of action
    int deviceId;               // Device ID (keyboard=0, gamepad1=1, etc.)
    InputDeviceType deviceType; // Type of device
    int inputCode;              // Key code, button ID, etc.
    int defaultCode;            // Default mapping code
    int isConfigured;           // Whether mapping has been configured
} InputMapping;

// Player input configuration
typedef struct {
    int playerId;                  // Player ID (0-based)
    GameType gameType;             // Type of game for default mappings
    InputMapping* mappings;        // Array of mappings
    int mappingCount;              // Number of mappings
    const char* profileName;       // Name of current profile
} PlayerInputConfig;

// Initialize input mapping system
void InputMapper_Init(void);

// Load input mapping profiles
int InputMapper_LoadProfiles(const char* profileDir);

// Configure a mapping for a player
int InputMapper_ConfigureMapping(int playerId, const char* actionName, InputDeviceType deviceType, int deviceId, int inputCode);

// Apply preset configurations based on game type
int InputMapper_ApplyPreset(int playerId, GameType gameType);

// Process input event and translate to game actions
int InputMapper_ProcessInput(int deviceType, int deviceId, int inputCode, int value);

// Save current mappings to a profile
int InputMapper_SaveProfile(const char* profileName);

// Load a specific profile
int InputMapper_LoadProfile(const char* profileName);

// Get current mapping for a player and action
InputMapping* InputMapper_GetMapping(int playerId, const char* actionName);

// Reset all mappings to defaults
void InputMapper_ResetToDefaults(void);

// Set the game type for auto-configuration
void InputMapper_SetGameType(GameType gameType);

// Get the current game type
GameType InputMapper_GetGameType(void);

#ifdef __cplusplus
}
#endif

#endif // INPUT_MAPPING_H 