#include "gamepad_support.h"
#include "input_mapping.h"
#include "rom_loading_debug.h"
#include "memory_tracking.h"
#include <string.h>
#include <stdlib.h>

// Maximum number of gamepads supported
#define MAX_GAMEPADS 8

// Gamepad states
static GamepadState g_gamepads[MAX_GAMEPADS];
static int g_gamepadCount = 0;
static int g_initialized = 0;

// Deadzone for analog sticks
#define ANALOG_DEADZONE 0.25f

// Initialize gamepad support
int Gamepad_Init(void) {
    if (g_initialized) {
        return 0; // Already initialized
    }
    
    // Initialize gamepad states
    memset(g_gamepads, 0, sizeof(g_gamepads));
    g_gamepadCount = 0;
    
    // In a real implementation, this would initialize the GameController framework
    // For now, we'll just simulate a single gamepad
    g_gamepads[0].connected = 1;
    strcpy(g_gamepads[0].name, "Simulated Gamepad");
    g_gamepads[0].playerIndex = 0;
    g_gamepadCount = 1;
    
    g_initialized = 1;
    
    ROMLoader_TrackLoadStep("INPUT INIT", "Gamepad support initialized, %d controller(s) detected", g_gamepadCount);
    
    return 0;
}

// Shutdown gamepad support
void Gamepad_Shutdown(void) {
    if (!g_initialized) {
        return;
    }
    
    // In a real implementation, this would clean up the GameController framework
    g_initialized = 0;
    
    ROMLoader_DebugLog(LOG_INFO, "Gamepad support shutdown");
}

// Apply deadzone to analog input
static float ApplyDeadzone(float value) {
    if (value < -ANALOG_DEADZONE) {
        // Scale the value from -1.0 to -ANALOG_DEADZONE back to -1.0 to 0.0
        return (value + ANALOG_DEADZONE) / (1.0f - ANALOG_DEADZONE);
    } else if (value > ANALOG_DEADZONE) {
        // Scale the value from ANALOG_DEADZONE to 1.0 back to 0.0 to 1.0
        return (value - ANALOG_DEADZONE) / (1.0f - ANALOG_DEADZONE);
    } else {
        // Within deadzone, return 0
        return 0.0f;
    }
}

// Update all gamepads (poll for new state)
void Gamepad_Update(void) {
    if (!g_initialized) {
        return;
    }
    
    // In a real implementation, this would poll the GameController framework
    // For now, we'll just simulate no changes
}

// Get the number of connected gamepads
int Gamepad_GetCount(void) {
    return g_gamepadCount;
}

// Get the state of a specific gamepad
GamepadState* Gamepad_GetState(int gamepadIndex) {
    if (gamepadIndex < 0 || gamepadIndex >= MAX_GAMEPADS || !g_gamepads[gamepadIndex].connected) {
        return NULL;
    }
    
    return &g_gamepads[gamepadIndex];
}

// Check if a button is pressed
int Gamepad_IsButtonPressed(int gamepadIndex, GamepadButton button) {
    if (gamepadIndex < 0 || gamepadIndex >= MAX_GAMEPADS || 
        !g_gamepads[gamepadIndex].connected || 
        button < 0 || button >= GAMEPAD_BUTTON_COUNT) {
        return 0;
    }
    
    return g_gamepads[gamepadIndex].buttonState[button];
}

// Get an axis value (-1.0 to 1.0)
float Gamepad_GetAxisValue(int gamepadIndex, GamepadAxis axis) {
    if (gamepadIndex < 0 || gamepadIndex >= MAX_GAMEPADS || 
        !g_gamepads[gamepadIndex].connected || 
        axis < 0 || axis >= GAMEPAD_AXIS_COUNT) {
        return 0.0f;
    }
    
    // Apply deadzone to analog sticks
    if (axis >= GAMEPAD_AXIS_LEFT_X && axis <= GAMEPAD_AXIS_RIGHT_Y) {
        return ApplyDeadzone(g_gamepads[gamepadIndex].axisState[axis]);
    }
    
    // Triggers don't need deadzone
    return g_gamepads[gamepadIndex].axisState[axis];
}

// Apply vibration/rumble (0.0 to 1.0 intensity)
void Gamepad_SetVibration(int gamepadIndex, float leftMotor, float rightMotor) {
    if (gamepadIndex < 0 || gamepadIndex >= MAX_GAMEPADS || !g_gamepads[gamepadIndex].connected) {
        return;
    }
    
    // In a real implementation, this would use the GameController haptics API
    ROMLoader_DebugLog(LOG_DETAIL, "Gamepad %d vibration: L=%.2f R=%.2f", 
                     gamepadIndex, leftMotor, rightMotor);
}

// Map a gamepad input to a player action
int Gamepad_MapInput(int gamepadIndex, int playerId, GamepadButton button, const char* actionName) {
    if (gamepadIndex < 0 || gamepadIndex >= MAX_GAMEPADS || 
        !g_gamepads[gamepadIndex].connected || 
        button < 0 || button >= GAMEPAD_BUTTON_COUNT ||
        !actionName) {
        return -1;
    }
    
    // Configure the input mapping
    return InputMapper_ConfigureMapping(playerId, actionName, INPUT_DEVICE_GAMEPAD, gamepadIndex, button);
}

// Auto-assign gamepads to players
void Gamepad_AutoAssignPlayers(void) {
    if (!g_initialized) {
        return;
    }
    
    // Assign each connected gamepad to a player
    int playerCount = 0;
    for (int i = 0; i < MAX_GAMEPADS && playerCount < 4; i++) {
        if (g_gamepads[i].connected) {
            g_gamepads[i].playerIndex = playerCount++;
            
            ROMLoader_DebugLog(LOG_INFO, "Assigned gamepad %d (%s) to Player %d", 
                             i, g_gamepads[i].name, g_gamepads[i].playerIndex + 1);
            
            ROMLoader_TrackLoadStep("INPUT INIT", "Assigned %s to Player %d", 
                                 g_gamepads[i].name, g_gamepads[i].playerIndex + 1);
        }
    }
}

// Simulate a button press (for testing)
void Gamepad_SimulateButtonPress(int gamepadIndex, GamepadButton button, int isPressed) {
    if (gamepadIndex < 0 || gamepadIndex >= MAX_GAMEPADS || 
        !g_gamepads[gamepadIndex].connected || 
        button < 0 || button >= GAMEPAD_BUTTON_COUNT) {
        return;
    }
    
    g_gamepads[gamepadIndex].buttonState[button] = isPressed;
    
    // Automatically translate to input actions
    InputMapper_ProcessInput(INPUT_DEVICE_GAMEPAD, gamepadIndex, button, isPressed);
}

// Simulate an axis change (for testing)
void Gamepad_SimulateAxisChange(int gamepadIndex, GamepadAxis axis, float value) {
    if (gamepadIndex < 0 || gamepadIndex >= MAX_GAMEPADS || 
        !g_gamepads[gamepadIndex].connected || 
        axis < 0 || axis >= GAMEPAD_AXIS_COUNT) {
        return;
    }
    
    g_gamepads[gamepadIndex].axisState[axis] = value;
    
    // Convert to DPAD for certain thresholds
    if (axis == GAMEPAD_AXIS_LEFT_X && fabs(value) > ANALOG_DEADZONE) {
        if (value < -ANALOG_DEADZONE) {
            // Left
            Gamepad_SimulateButtonPress(gamepadIndex, GAMEPAD_BUTTON_DPAD_LEFT, 1);
            Gamepad_SimulateButtonPress(gamepadIndex, GAMEPAD_BUTTON_DPAD_RIGHT, 0);
        } else if (value > ANALOG_DEADZONE) {
            // Right
            Gamepad_SimulateButtonPress(gamepadIndex, GAMEPAD_BUTTON_DPAD_LEFT, 0);
            Gamepad_SimulateButtonPress(gamepadIndex, GAMEPAD_BUTTON_DPAD_RIGHT, 1);
        }
    } else if (axis == GAMEPAD_AXIS_LEFT_Y && fabs(value) > ANALOG_DEADZONE) {
        if (value < -ANALOG_DEADZONE) {
            // Up
            Gamepad_SimulateButtonPress(gamepadIndex, GAMEPAD_BUTTON_DPAD_UP, 1);
            Gamepad_SimulateButtonPress(gamepadIndex, GAMEPAD_BUTTON_DPAD_DOWN, 0);
        } else if (value > ANALOG_DEADZONE) {
            // Down
            Gamepad_SimulateButtonPress(gamepadIndex, GAMEPAD_BUTTON_DPAD_UP, 0);
            Gamepad_SimulateButtonPress(gamepadIndex, GAMEPAD_BUTTON_DPAD_DOWN, 1);
        }
    }
}

// Handle gamepad connection/disconnection event
void Gamepad_HandleConnectionEvent(int gamepadIndex, int connected) {
    if (gamepadIndex < 0 || gamepadIndex >= MAX_GAMEPADS) {
        return;
    }
    
    // Update connection state
    g_gamepads[gamepadIndex].connected = connected;
    
    if (connected) {
        // New connection
        ROMLoader_DebugLog(LOG_INFO, "Gamepad %d connected: %s", gamepadIndex, g_gamepads[gamepadIndex].name);
        ROMLoader_TrackLoadStep("INPUT INIT", "Controller connected: %s", g_gamepads[gamepadIndex].name);
        
        // Auto-assign to player
        g_gamepads[gamepadIndex].playerIndex = gamepadIndex;
        if (gamepadIndex < 4) {
            ROMLoader_TrackLoadStep("INPUT INIT", "Assigned controller to Player %d", gamepadIndex + 1);
        }
    } else {
        // Disconnection
        ROMLoader_DebugLog(LOG_INFO, "Gamepad %d disconnected: %s", 
                         gamepadIndex, g_gamepads[gamepadIndex].name);
        ROMLoader_TrackLoadStep("INPUT INIT", "Controller disconnected: %s", g_gamepads[gamepadIndex].name);
        
        // Clear player assignment
        g_gamepads[gamepadIndex].playerIndex = -1;
    }
    
    // Update count of connected gamepads
    g_gamepadCount = 0;
    for (int i = 0; i < MAX_GAMEPADS; i++) {
        if (g_gamepads[i].connected) {
            g_gamepadCount++;
        }
    }
} 