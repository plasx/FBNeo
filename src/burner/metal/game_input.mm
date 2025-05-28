#import <Cocoa/Cocoa.h>
#import <GameController/GameController.h>
#include "burnint.h"
#include "metal_bridge.h"
#include "game_input.h"

// Input system for FBNeo Metal implementation

// Define missing input constants if needed
#ifndef BIT_DIGITAL
#define BIT_DIGITAL (1)
#endif

#ifndef BIT_UNUSED
#define BIT_UNUSED (0)
#endif

// Forward declare GameInp structure
struct GameInp {
    UINT8 nInput;
    char* szName;
    UINT8 nType;
    union {
        UINT8* pVal;
        UINT16* pShortVal;
    } Input;
};

// Input state
typedef struct {
    bool initialized;
    InputState state;
    id<NSObject> gamepadConnectObserver;
    id<NSObject> gamepadDisconnectObserver;
    NSMutableArray* gamepads;
    GCController* activeGamepad;
} InputSystemState;

static InputSystemState inputSystem = {
    .initialized = false,
    .state = {{{0}}},  // Properly initialize nested struct with braces
    .gamepadConnectObserver = nil,
    .gamepadDisconnectObserver = nil,
    .gamepads = nil,
    .activeGamepad = nil
};

// Map key codes to input state
static void MapKeyToInput(int keyCode, bool pressed) {
    // Map key codes to input state
    switch (keyCode) {
        // Player 1 directional inputs
        case 0x7B: // Left Arrow
            inputSystem.state.players[0].left = pressed;
            break;
        case 0x7C: // Right Arrow
            inputSystem.state.players[0].right = pressed;
            break;
        case 0x7D: // Down Arrow
            inputSystem.state.players[0].down = pressed;
            break;
        case 0x7E: // Up Arrow
            inputSystem.state.players[0].up = pressed;
            break;
            
        // Player 1 buttons - QWERTY keyboard mapping
        case 0x00: // A key - Punch
            inputSystem.state.players[0].buttons[0] = pressed;
            break;
        case 0x01: // S key - Kick
            inputSystem.state.players[0].buttons[1] = pressed;
            break;
        case 0x02: // D key - Special
            inputSystem.state.players[0].buttons[2] = pressed;
            break;
        case 0x03: // F key - Tag
            inputSystem.state.players[0].buttons[3] = pressed;
            break;
        case 0x05: // G key - Button 5
            inputSystem.state.players[0].buttons[4] = pressed;
            break;
        case 0x04: // H key - Button 6
            inputSystem.state.players[0].buttons[5] = pressed;
            break;
            
        // Player 2 directional inputs - use different keys
        case 0x0D: // W key - P2 Up
            inputSystem.state.players[1].up = pressed;
            break;
        case 0x0A: // A key - P2 Left
            inputSystem.state.players[1].left = pressed;
            break;
        case 0x0F: // S key - P2 Down
            inputSystem.state.players[1].down = pressed;
            break;
        case 0x07: // D key - P2 Right
            inputSystem.state.players[1].right = pressed;
            break;
            
        // Player 2 buttons - QWERTY keyboard mapping
        case 0x0C: // Q key - P2 Button 1
            inputSystem.state.players[1].buttons[0] = pressed;
            break;
        case 0x0E: // E key - P2 Button 2
            inputSystem.state.players[1].buttons[1] = pressed;
            break;
        case 0x0B: // R key - P2 Button 3
            inputSystem.state.players[1].buttons[2] = pressed;
            break;
        case 0x11: // T key - P2 Button 4
            inputSystem.state.players[1].buttons[3] = pressed;
            break;
        case 0x10: // Y key - P2 Button 5
            inputSystem.state.players[1].buttons[4] = pressed;
            break;
        case 0x12: // U key - P2 Button 6
            inputSystem.state.players[1].buttons[5] = pressed;
            break;
            
        // System controls
        case 0x31: // Space - Start
            if (pressed) {
                inputSystem.state.players[0].start = !inputSystem.state.players[0].start;
            }
            break;
        case 0x30: // Tab - Start P2
            if (pressed) {
                inputSystem.state.players[1].start = !inputSystem.state.players[1].start;
            }
            break;
        case 0x08: // C key - Coin
            if (pressed) {
                inputSystem.state.players[0].coin = !inputSystem.state.players[0].coin;
            }
            break;
        case 0x09: // V key - Coin P2
            if (pressed) {
                inputSystem.state.players[1].coin = !inputSystem.state.players[1].coin;
            }
            break;
        case 0x35: // Escape - Exit
            inputSystem.state.exit = pressed;
            break;
        case 0x24: // Return - Menu
            inputSystem.state.menu = pressed;
            break;
    }
}

// Handler for gamepad connect events
static void GamepadConnected(GCController* controller) {
    printf("Gamepad connected\n");
    
    if (!inputSystem.gamepads) {
        return;
    }
    
    // Add to gamepads array if not already there
    if (![inputSystem.gamepads containsObject:controller]) {
        [inputSystem.gamepads addObject:controller];
        printf("Added new gamepad. Total gamepads: %lu\n", (unsigned long)[inputSystem.gamepads count]);
    }
    
    // If no active gamepad, set this one as active
    if (!inputSystem.activeGamepad && [inputSystem.gamepads count] > 0) {
        inputSystem.activeGamepad = controller;
        printf("Set active gamepad: %s\n", [controller.vendorName UTF8String]);
        
        // Register input handlers for this controller
        if (controller.extendedGamepad) {
            GCExtendedGamepad* gamepad = controller.extendedGamepad;
            
            // Set value changed handler for all inputs
            gamepad.valueChangedHandler = ^(GCExtendedGamepad* _Nonnull gamepad, GCControllerElement* _Nonnull element) {
                // Handle directional inputs
                inputSystem.state.players[0].up = gamepad.dpad.up.pressed;
                inputSystem.state.players[0].down = gamepad.dpad.down.pressed;
                inputSystem.state.players[0].left = gamepad.dpad.left.pressed;
                inputSystem.state.players[0].right = gamepad.dpad.right.pressed;
                
                // Handle buttons
                inputSystem.state.players[0].buttons[0] = gamepad.buttonA.pressed;
                inputSystem.state.players[0].buttons[1] = gamepad.buttonB.pressed;
                inputSystem.state.players[0].buttons[2] = gamepad.buttonX.pressed;
                inputSystem.state.players[0].buttons[3] = gamepad.buttonY.pressed;
                inputSystem.state.players[0].buttons[4] = gamepad.leftShoulder.pressed;
                inputSystem.state.players[0].buttons[5] = gamepad.rightShoulder.pressed;
                
                // Handle start/coin
                if (gamepad.buttonOptions && gamepad.buttonOptions.pressed) {
                    inputSystem.state.players[0].coin = true;
                } else {
                    inputSystem.state.players[0].coin = false;
                }
                
                if (gamepad.buttonMenu && gamepad.buttonMenu.pressed) {
                    inputSystem.state.players[0].start = true;
                } else {
                    inputSystem.state.players[0].start = false;
                }
            };
        }
    }
}

// Handler for gamepad disconnect events
static void GamepadDisconnected(GCController* controller) {
    printf("Gamepad disconnected\n");
    
    if (!inputSystem.gamepads) {
        return;
    }
    
    // Remove from gamepads array
    if ([inputSystem.gamepads containsObject:controller]) {
        [inputSystem.gamepads removeObject:controller];
        printf("Removed gamepad. Total gamepads: %lu\n", (unsigned long)[inputSystem.gamepads count]);
    }
    
    // If this was the active gamepad, pick a new one
    if (inputSystem.activeGamepad == controller) {
        if ([inputSystem.gamepads count] > 0) {
            inputSystem.activeGamepad = [inputSystem.gamepads objectAtIndex:0];
            printf("Set new active gamepad: %s\n", [inputSystem.activeGamepad.vendorName UTF8String]);
        } else {
            inputSystem.activeGamepad = nil;
            printf("No active gamepad now\n");
        }
    }
}

// Initialize the input system
int GameInput_Init() {
    printf("GameInput_Init()\n");
    
    if (inputSystem.initialized) {
        printf("Input system already initialized\n");
        return 0;
    }
    
    // Initialize state
    memset(&inputSystem.state, 0, sizeof(inputSystem.state));
    
    // Create gamepads array
    inputSystem.gamepads = [[NSMutableArray alloc] init];
    
    // Register for gamepad connect/disconnect notifications
    inputSystem.gamepadConnectObserver = 
        [[NSNotificationCenter defaultCenter] addObserverForName:GCControllerDidConnectNotification
                                                         object:nil
                                                          queue:[NSOperationQueue mainQueue]
                                                     usingBlock:^(NSNotification * _Nonnull note) {
                                                         GamepadConnected(note.object);
                                                     }];
    
    inputSystem.gamepadDisconnectObserver = 
        [[NSNotificationCenter defaultCenter] addObserverForName:GCControllerDidDisconnectNotification
                                                         object:nil
                                                          queue:[NSOperationQueue mainQueue]
                                                     usingBlock:^(NSNotification * _Nonnull note) {
                                                         GamepadDisconnected(note.object);
                                                     }];
    
    // Start the discovery process for wireless gamepads
    [GCController startWirelessControllerDiscoveryWithCompletionHandler:^{
        // Called when discovery is complete
        printf("Gamepad discovery complete\n");
    }];
    
    // Add any already-connected gamepads
    NSArray* controllers = [GCController controllers];
    for (GCController* controller in controllers) {
        GamepadConnected(controller);
    }
    
    // Set flag
    inputSystem.initialized = true;
    
    printf("Input system initialized\n");
    
    return 0;
}

// Shutdown the input system
int GameInput_Exit() {
    printf("GameInput_Exit()\n");
    
    if (!inputSystem.initialized) {
        printf("Input system not initialized\n");
        return 0;
    }
    
    // Stop gamepad discovery
    [GCController stopWirelessControllerDiscovery];
    
    // Remove notification observers
    if (inputSystem.gamepadConnectObserver) {
        [[NSNotificationCenter defaultCenter] removeObserver:inputSystem.gamepadConnectObserver];
        inputSystem.gamepadConnectObserver = nil;
    }
    
    if (inputSystem.gamepadDisconnectObserver) {
        [[NSNotificationCenter defaultCenter] removeObserver:inputSystem.gamepadDisconnectObserver];
        inputSystem.gamepadDisconnectObserver = nil;
    }
    
    // Release gamepads array
    inputSystem.gamepads = nil;
    inputSystem.activeGamepad = nil;
    
    // Reset state
    memset(&inputSystem.state, 0, sizeof(inputSystem.state));
    inputSystem.initialized = false;
    
    printf("Input system shutdown\n");
    
    return 0;
}

// Handle key down event
int GameInput_HandleKeyDown(int keyCode) {
    //printf("GameInput_HandleKeyDown(%d)\n", keyCode);
    
    if (!inputSystem.initialized) {
        return 1;
    }
    
    MapKeyToInput(keyCode, true);
    
    return 0;
}

// Handle key up event
int GameInput_HandleKeyUp(int keyCode) {
    //printf("GameInput_HandleKeyUp(%d)\n", keyCode);
    
    if (!inputSystem.initialized) {
        return 1;
    }
    
    MapKeyToInput(keyCode, false);
    
    return 0;
}

// Get the current input state
const InputState* GameInput_GetState() {
    return &inputSystem.state;
}

// Check if a specific button is pressed for a player
bool GameInput_IsButtonPressed(int player, int button) {
    if (player < 0 || player >= MAX_PLAYERS || button < 0 || button >= MAX_BUTTONS) {
        return false;
    }
    
    return inputSystem.state.players[player].buttons[button];
}

// Configure input mappings - this would show a UI for input configuration
int GameInput_ConfigureControls() {
    printf("GameInput_ConfigureControls()\n");
    
    if (!inputSystem.initialized) {
        printf("Input system not initialized\n");
        return 1;
    }
    
    // In a real implementation, this would show a UI for remapping controls
    printf("Input configuration would be shown here\n");
    
    return 0;
}

// Is exit requested
bool GameInput_IsExitRequested() {
    return inputSystem.state.exit;
}

// Is menu requested
bool GameInput_IsMenuRequested() {
    return inputSystem.state.menu;
}

// Get the number of connected gamepads
int GameInput_GetGamepadCount() {
    if (!inputSystem.initialized || !inputSystem.gamepads) {
        return 0;
    }
    
    return (int)[inputSystem.gamepads count];
}

// Check if a gamepad is active
bool GameInput_HasActiveGamepad() {
    return inputSystem.activeGamepad != nil;
}

// Get active gamepad name
const char* GameInput_GetActiveGamepadName() {
    if (!inputSystem.activeGamepad) {
        return "None";
    }
    
    return [inputSystem.activeGamepad.vendorName UTF8String];
}

// For now, implement a simplified version that doesn't depend on GameInp
void GameInput_Update() {
    if (!inputSystem.initialized) {
        return;
    }
    
    // Simplified version that doesn't use GameInp for now
    printf("Input state updated\n");
}

// Reset all inputs
void GameInput_Reset() {
    if (!inputSystem.initialized) {
        return;
    }
    
    // Clear all input state
    memset(&inputSystem.state, 0, sizeof(inputSystem.state));
    
    printf("Input state reset\n");
} 