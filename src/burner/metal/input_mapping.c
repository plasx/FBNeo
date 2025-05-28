#include "input_mapping.h"
#include "rom_loading_debug.h"
#include "memory_tracking.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Maximum number of players supported
#define MAX_PLAYERS 4

// Maximum number of mappings per player
#define MAX_MAPPINGS_PER_PLAYER 32

// Maximum number of profiles
#define MAX_PROFILES 16

// Global input configuration for each player
static PlayerInputConfig g_playerConfigs[MAX_PLAYERS];

// Current game type
static GameType g_currentGameType = GAME_TYPE_FIGHTING;

// Loaded profile names
static char g_profileNames[MAX_PROFILES][64];
static int g_numProfiles = 0;

// Default mappings for different game types
typedef struct {
    GameType gameType;
    const char* actionName;
    InputActionType actionType;
    InputDeviceType defaultDeviceType;
    int defaultInputCode;
} DefaultMapping;

// Default fighting game mappings
static DefaultMapping g_fightingDefaults[] = {
    { GAME_TYPE_FIGHTING, "Up", INPUT_ACTION_DPAD, INPUT_DEVICE_KEYBOARD, 'w' },
    { GAME_TYPE_FIGHTING, "Down", INPUT_ACTION_DPAD, INPUT_DEVICE_KEYBOARD, 's' },
    { GAME_TYPE_FIGHTING, "Left", INPUT_ACTION_DPAD, INPUT_DEVICE_KEYBOARD, 'a' },
    { GAME_TYPE_FIGHTING, "Right", INPUT_ACTION_DPAD, INPUT_DEVICE_KEYBOARD, 'd' },
    { GAME_TYPE_FIGHTING, "Punch1", INPUT_ACTION_BUTTON, INPUT_DEVICE_KEYBOARD, 'j' },
    { GAME_TYPE_FIGHTING, "Punch2", INPUT_ACTION_BUTTON, INPUT_DEVICE_KEYBOARD, 'k' },
    { GAME_TYPE_FIGHTING, "Punch3", INPUT_ACTION_BUTTON, INPUT_DEVICE_KEYBOARD, 'l' },
    { GAME_TYPE_FIGHTING, "Kick1", INPUT_ACTION_BUTTON, INPUT_DEVICE_KEYBOARD, 'u' },
    { GAME_TYPE_FIGHTING, "Kick2", INPUT_ACTION_BUTTON, INPUT_DEVICE_KEYBOARD, 'i' },
    { GAME_TYPE_FIGHTING, "Kick3", INPUT_ACTION_BUTTON, INPUT_DEVICE_KEYBOARD, 'o' },
    { GAME_TYPE_FIGHTING, "Start", INPUT_ACTION_BUTTON, INPUT_DEVICE_KEYBOARD, '1' },
    { GAME_TYPE_FIGHTING, "Coin", INPUT_ACTION_BUTTON, INPUT_DEVICE_KEYBOARD, '5' }
};

// Default shmup mappings
static DefaultMapping g_shmupDefaults[] = {
    { GAME_TYPE_SHMUP, "Up", INPUT_ACTION_DPAD, INPUT_DEVICE_KEYBOARD, 'w' },
    { GAME_TYPE_SHMUP, "Down", INPUT_ACTION_DPAD, INPUT_DEVICE_KEYBOARD, 's' },
    { GAME_TYPE_SHMUP, "Left", INPUT_ACTION_DPAD, INPUT_DEVICE_KEYBOARD, 'a' },
    { GAME_TYPE_SHMUP, "Right", INPUT_ACTION_DPAD, INPUT_DEVICE_KEYBOARD, 'd' },
    { GAME_TYPE_SHMUP, "Fire", INPUT_ACTION_BUTTON, INPUT_DEVICE_KEYBOARD, 'j' },
    { GAME_TYPE_SHMUP, "Bomb", INPUT_ACTION_BUTTON, INPUT_DEVICE_KEYBOARD, 'k' },
    { GAME_TYPE_SHMUP, "Special", INPUT_ACTION_BUTTON, INPUT_DEVICE_KEYBOARD, 'l' },
    { GAME_TYPE_SHMUP, "Start", INPUT_ACTION_BUTTON, INPUT_DEVICE_KEYBOARD, '1' },
    { GAME_TYPE_SHMUP, "Coin", INPUT_ACTION_BUTTON, INPUT_DEVICE_KEYBOARD, '5' }
};

// Get the number of default mappings for a game type
static int GetDefaultMappingCount(GameType gameType) {
    switch (gameType) {
        case GAME_TYPE_FIGHTING:
            return sizeof(g_fightingDefaults) / sizeof(DefaultMapping);
        case GAME_TYPE_SHMUP:
            return sizeof(g_shmupDefaults) / sizeof(DefaultMapping);
        default:
            // Default to fighting game mappings for now
            return sizeof(g_fightingDefaults) / sizeof(DefaultMapping);
    }
}

// Get the default mapping array for a game type
static DefaultMapping* GetDefaultMappings(GameType gameType) {
    switch (gameType) {
        case GAME_TYPE_FIGHTING:
            return g_fightingDefaults;
        case GAME_TYPE_SHMUP:
            return g_shmupDefaults;
        default:
            // Default to fighting game mappings for now
            return g_fightingDefaults;
    }
}

// Initialize input mapping system
void InputMapper_Init(void) {
    // Initialize player configurations
    for (int i = 0; i < MAX_PLAYERS; i++) {
        g_playerConfigs[i].playerId = i;
        g_playerConfigs[i].gameType = GAME_TYPE_FIGHTING; // Default to fighting game
        g_playerConfigs[i].mappings = NULL;
        g_playerConfigs[i].mappingCount = 0;
        g_playerConfigs[i].profileName = "Default";
    }
    
    // Reset profiles
    g_numProfiles = 0;
    strcpy(g_profileNames[g_numProfiles++], "Default");
    
    // Load default mappings for player 1
    InputMapper_ApplyPreset(0, GAME_TYPE_FIGHTING);
    
    ROMLoader_TrackLoadStep("INPUT INIT", "Input mapping system initialized with default profiles");
}

// Configure a mapping for a player
int InputMapper_ConfigureMapping(int playerId, const char* actionName, InputDeviceType deviceType, int deviceId, int inputCode) {
    if (playerId < 0 || playerId >= MAX_PLAYERS || !actionName) {
        return -1;
    }
    
    PlayerInputConfig* config = &g_playerConfigs[playerId];
    
    // Find existing mapping
    for (int i = 0; i < config->mappingCount; i++) {
        if (strcmp(config->mappings[i].name, actionName) == 0) {
            // Update existing mapping
            config->mappings[i].deviceType = deviceType;
            config->mappings[i].deviceId = deviceId;
            config->mappings[i].inputCode = inputCode;
            config->mappings[i].isConfigured = 1;
            
            ROMLoader_DebugLog(LOG_INFO, "Updated mapping for Player %d: %s -> %d (device %d, type %d)",
                             playerId + 1, actionName, inputCode, deviceId, deviceType);
            return i;
        }
    }
    
    // If we reach here, this is a new mapping
    if (config->mappingCount >= MAX_MAPPINGS_PER_PLAYER) {
        ROMLoader_DebugLog(LOG_WARNING, "Too many mappings for Player %d, can't add %s",
                         playerId + 1, actionName);
        return -1;
    }
    
    // Allocate mappings array if needed
    if (!config->mappings) {
        config->mappings = (InputMapping*)MemoryTracker_Allocate(
            MAX_MAPPINGS_PER_PLAYER * sizeof(InputMapping),
            "Input Mappings for Player");
        if (!config->mappings) {
            ROMLoader_DebugLog(LOG_ERROR, "Failed to allocate memory for input mappings");
            return -1;
        }
    }
    
    // Add new mapping
    int index = config->mappingCount++;
    config->mappings[index].name = strdup(actionName);
    config->mappings[index].actionType = INPUT_ACTION_BUTTON; // Default to button
    config->mappings[index].deviceType = deviceType;
    config->mappings[index].deviceId = deviceId;
    config->mappings[index].inputCode = inputCode;
    config->mappings[index].defaultCode = inputCode;
    config->mappings[index].isConfigured = 1;
    
    ROMLoader_DebugLog(LOG_INFO, "Added new mapping for Player %d: %s -> %d (device %d, type %d)",
                     playerId + 1, actionName, inputCode, deviceId, deviceType);
    
    return index;
}

// Apply preset configurations based on game type
int InputMapper_ApplyPreset(int playerId, GameType gameType) {
    if (playerId < 0 || playerId >= MAX_PLAYERS) {
        return -1;
    }
    
    PlayerInputConfig* config = &g_playerConfigs[playerId];
    config->gameType = gameType;
    
    // Free existing mappings
    if (config->mappings) {
        for (int i = 0; i < config->mappingCount; i++) {
            if (config->mappings[i].name) {
                free((void*)config->mappings[i].name);
            }
        }
        MemoryTracker_Free(config->mappings);
        config->mappings = NULL;
        config->mappingCount = 0;
    }
    
    // Allocate new mappings
    int mappingCount = GetDefaultMappingCount(gameType);
    config->mappings = (InputMapping*)MemoryTracker_Allocate(
        MAX_MAPPINGS_PER_PLAYER * sizeof(InputMapping),
        "Input Mappings for Player");
    if (!config->mappings) {
        ROMLoader_DebugLog(LOG_ERROR, "Failed to allocate memory for input mappings");
        return -1;
    }
    
    // Apply default mappings
    DefaultMapping* defaults = GetDefaultMappings(gameType);
    for (int i = 0; i < mappingCount; i++) {
        config->mappings[i].name = strdup(defaults[i].actionName);
        config->mappings[i].actionType = defaults[i].actionType;
        config->mappings[i].deviceType = defaults[i].defaultDeviceType;
        config->mappings[i].deviceId = 0; // First device of this type
        config->mappings[i].inputCode = defaults[i].defaultInputCode;
        config->mappings[i].defaultCode = defaults[i].defaultInputCode;
        config->mappings[i].isConfigured = 1;
        
        config->mappingCount++;
    }
    
    // Set profile name
    switch (gameType) {
        case GAME_TYPE_FIGHTING:
            config->profileName = "Fighting";
            break;
        case GAME_TYPE_SHMUP:
            config->profileName = "Shooter";
            break;
        default:
            config->profileName = "Default";
            break;
    }
    
    ROMLoader_DebugLog(LOG_INFO, "Applied %s preset for Player %d (%d mappings)",
                     config->profileName, playerId + 1, config->mappingCount);
    
    ROMLoader_TrackLoadStep("INPUT INIT", "Applied %s control scheme for Player %d",
                         config->profileName, playerId + 1);
    
    return 0;
}

// Process input event and translate to game actions
int InputMapper_ProcessInput(int deviceType, int deviceId, int inputCode, int value) {
    // Track if input was handled
    int handled = 0;
    
    // Check mappings for all players
    for (int playerId = 0; playerId < MAX_PLAYERS; playerId++) {
        PlayerInputConfig* config = &g_playerConfigs[playerId];
        
        // Skip players without mappings
        if (!config->mappings || config->mappingCount == 0) {
            continue;
        }
        
        // Check each mapping
        for (int i = 0; i < config->mappingCount; i++) {
            InputMapping* mapping = &config->mappings[i];
            
            // If this mapping matches the input
            if (mapping->deviceType == deviceType && 
                mapping->deviceId == deviceId && 
                mapping->inputCode == inputCode) {
                
                // This is a match, translate to game action
                ROMLoader_DebugLog(LOG_DETAIL, "Input match for Player %d: %s (value: %d)",
                                 playerId + 1, mapping->name, value);
                
                // Forward to the game input system
                // In a real implementation, this would call into the FBNeo input system
                
                handled = 1;
            }
        }
    }
    
    return handled;
}

// Save current mappings to a profile
int InputMapper_SaveProfile(const char* profileName) {
    if (!profileName) {
        return -1;
    }
    
    // In a real implementation, this would save to a file
    // For now, just add to our list of profiles
    if (g_numProfiles < MAX_PROFILES) {
        strncpy(g_profileNames[g_numProfiles], profileName, sizeof(g_profileNames[0]) - 1);
        g_profileNames[g_numProfiles][sizeof(g_profileNames[0]) - 1] = '\0';
        g_numProfiles++;
        
        ROMLoader_DebugLog(LOG_INFO, "Saved input profile: %s", profileName);
        return 0;
    }
    
    return -1;
}

// Load a specific profile
int InputMapper_LoadProfile(const char* profileName) {
    if (!profileName) {
        return -1;
    }
    
    // In a real implementation, this would load from a file
    // For now, just check if it's one of our standard types
    if (strcmp(profileName, "Fighting") == 0) {
        for (int i = 0; i < MAX_PLAYERS; i++) {
            InputMapper_ApplyPreset(i, GAME_TYPE_FIGHTING);
        }
        return 0;
    } else if (strcmp(profileName, "Shooter") == 0) {
        for (int i = 0; i < MAX_PLAYERS; i++) {
            InputMapper_ApplyPreset(i, GAME_TYPE_SHMUP);
        }
        return 0;
    }
    
    return -1;
}

// Get current mapping for a player and action
InputMapping* InputMapper_GetMapping(int playerId, const char* actionName) {
    if (playerId < 0 || playerId >= MAX_PLAYERS || !actionName) {
        return NULL;
    }
    
    PlayerInputConfig* config = &g_playerConfigs[playerId];
    
    // Find mapping by name
    for (int i = 0; i < config->mappingCount; i++) {
        if (strcmp(config->mappings[i].name, actionName) == 0) {
            return &config->mappings[i];
        }
    }
    
    return NULL;
}

// Reset all mappings to defaults
void InputMapper_ResetToDefaults(void) {
    for (int i = 0; i < MAX_PLAYERS; i++) {
        InputMapper_ApplyPreset(i, g_playerConfigs[i].gameType);
    }
    
    ROMLoader_TrackLoadStep("INPUT INIT", "Input mappings reset to defaults");
}

// Set the game type for auto-configuration
void InputMapper_SetGameType(GameType gameType) {
    g_currentGameType = gameType;
    
    // Apply presets for all players
    for (int i = 0; i < MAX_PLAYERS; i++) {
        InputMapper_ApplyPreset(i, gameType);
    }
    
    ROMLoader_TrackLoadStep("INPUT INIT", "Game type set to %s, applied appropriate control schemes",
                         gameType == GAME_TYPE_FIGHTING ? "Fighting" :
                         gameType == GAME_TYPE_SHMUP ? "Shooter" : "Default");
}

// Get the current game type
GameType InputMapper_GetGameType(void) {
    return g_currentGameType;
} 