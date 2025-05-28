#import <Cocoa/Cocoa.h>
#import <Carbon/Carbon.h> // For key codes
#import <GameController/GameController.h>

#include "metal_input.h"
#include "../../../burner/burner.h"
#include "inp_keys.h"

// Global input state
static InputState g_inputState;
static InputState g_prevInputState;  // Previous frame state for detecting transitions

// Key mapping profiles
static KeyBindingProfile g_keyProfiles[8]; // Support up to 8 profiles
static int g_profileCount = 0;
static int g_activeProfile = 0;

// Standard key mappings
static const KeyMapping kDefaultKeyMappings[] = {
    // Player 1 controls (WASD + JKL)
    { kVK_ANSI_W, FBK_UP },     // Up
    { kVK_ANSI_S, FBK_DOWN },   // Down
    { kVK_ANSI_A, FBK_LEFT },   // Left
    { kVK_ANSI_D, FBK_RIGHT },  // Right
    { kVK_ANSI_J, FBK_A },      // Button 1
    { kVK_ANSI_K, FBK_B },      // Button 2
    { kVK_ANSI_L, FBK_C },      // Button 3
    { kVK_ANSI_I, FBK_D },      // Button 4
    { kVK_ANSI_O, FBK_E },      // Button 5
    { kVK_ANSI_P, FBK_F },      // Button 6
    
    // Player 2 controls (Arrow keys + numeric pad)
    { kVK_UpArrow, FBK_4UP },       // Up
    { kVK_DownArrow, FBK_4DOWN },   // Down
    { kVK_LeftArrow, FBK_4LEFT },   // Left
    { kVK_RightArrow, FBK_4RIGHT }, // Right
    { kVK_ANSI_1, FBK_4A },         // Button 1
    { kVK_ANSI_2, FBK_4B },         // Button 2
    { kVK_ANSI_3, FBK_4C },         // Button 3
    { kVK_ANSI_4, FBK_4D },         // Button 4
    { kVK_ANSI_5, FBK_4E },         // Button 5
    { kVK_ANSI_6, FBK_4F },         // Button 6
    
    // Special keys
    { kVK_Return, FBK_START },     // Start
    { kVK_Space, FBK_COIN },       // Coin
    { kVK_Escape, FBK_ESCAPE },    // Exit
    { kVK_F1, FBK_F1 },            // F1
    { kVK_F2, FBK_F2 },            // F2
    { kVK_F3, FBK_F3 },            // F3
    { kVK_Tab, FBK_TAB }           // Tab
};

// Number of default key mappings
static const int kDefaultKeyMappingCount = sizeof(kDefaultKeyMappings) / sizeof(KeyMapping);

// Internal helper functions
static void InitializeDefaultKeyBindings();
static int TranslateMacKeyToFBNeoKey(int macKeyCode);
static bool IsModifierKey(int macKeyCode);
static int GetModifierIndex(int macKeyCode);

// Initialize the input system
int Metal_InitInput() {
    // Clear input state
    memset(&g_inputState, 0, sizeof(InputState));
    memset(&g_prevInputState, 0, sizeof(InputState));
    
    // Initialize key profiles
    g_profileCount = 0;
    g_activeProfile = 0;
    
    // Create default key binding profile
    InitializeDefaultKeyBindings();
    
    // Load any saved key bindings
    Metal_LoadKeyBindings();
    
    // Initialize gamepad detection
    if (@available(macOS 10.9, *)) {
        [GCController startWirelessControllerDiscovery:^{}];
        
        // Add observer for gamepad connection/disconnection
        [[NSNotificationCenter defaultCenter] addObserverForName:GCControllerDidConnectNotification
                                                          object:nil
                                                           queue:[NSOperationQueue mainQueue]
                                                      usingBlock:^(NSNotification *note) {
            NSLog(@"Gamepad connected");
            // Handle gamepad connection
        }];
        
        [[NSNotificationCenter defaultCenter] addObserverForName:GCControllerDidDisconnectNotification
                                                          object:nil
                                                           queue:[NSOperationQueue mainQueue]
                                                      usingBlock:^(NSNotification *note) {
            NSLog(@"Gamepad disconnected");
            // Handle gamepad disconnection
        }];
        
        // Get currently connected controllers
        NSArray *controllers = [GCController controllers];
        NSLog(@"Found %lu connected gamepads", (unsigned long)controllers.count);
        
        // Set up each controller
        for (GCController *controller in controllers) {
            // Assign controller to a player slot (0-3)
            int playerIndex = MIN((int)[controllers indexOfObject:controller], 3);
            controller.playerIndex = playerIndex;
            
            // Set controller handler
            if (controller.extendedGamepad) {
                controller.extendedGamepad.valueChangedHandler = ^(GCExtendedGamepad *gamepad, GCControllerElement *element) {
                    // Handle button/axis changes
                    // This would map to Metal_ProcessGamepadEvent calls
                };
            }
        }
    }
    
    NSLog(@"Metal input system initialized");
    return 0;
}

// Shutdown the input system
void Metal_ExitInput() {
    // Save key bindings
    Metal_SaveKeyBindings();
    
    // Stop gamepad discovery
    if (@available(macOS 10.9, *)) {
        [GCController stopWirelessControllerDiscovery];
        
        // Remove notification observers
        [[NSNotificationCenter defaultCenter] removeObserver:nil
                                                        name:GCControllerDidConnectNotification
                                                      object:nil];
        
        [[NSNotificationCenter defaultCenter] removeObserver:nil
                                                        name:GCControllerDidDisconnectNotification
                                                      object:nil];
    }
    
    NSLog(@"Metal input system shutdown");
}

// Process a key event
void Metal_ProcessKeyEvent(int keyCode, bool keyDown) {
    // Store keyboard state
    if (keyCode >= 0 && keyCode < 256) {
        g_inputState.keyboard.keyState[keyCode] = keyDown ? 1 : 0;
    }
    
    // Handle modifier keys separately
    if (IsModifierKey(keyCode)) {
        int modifierIndex = GetModifierIndex(keyCode);
        if (modifierIndex >= 0 && modifierIndex < 8) {
            g_inputState.keyboard.modifierState[modifierIndex] = keyDown;
        }
    }
    
    // Translate to FBNeo key
    int fbKey = TranslateMacKeyToFBNeoKey(keyCode);
    if (fbKey != 0) {
        // Forward to FBNeo's key processing function
        // This typically updates the global key state array or performs a callback
        NSLog(@"Key %d mapped to FBNeo key %d (down: %d)", keyCode, fbKey, keyDown);
        
        // Call into FBNeo input system
        if (keyDown) {
            InputSetState(fbKey, 0x01);
        } else {
            InputSetState(fbKey, 0x00);
        }
    }
}

// Process a mouse event
void Metal_ProcessMouseEvent(int button, int x, int y, bool buttonDown) {
    // Update mouse position
    g_inputState.mouse.x = x;
    g_inputState.mouse.y = y;
    
    // Calculate delta (could be smoother if called each frame instead)
    g_inputState.mouse.deltaX = x - g_prevInputState.mouse.x;
    g_inputState.mouse.deltaY = y - g_prevInputState.mouse.y;
    
    // Update button state (0 = left, 1 = right, 2 = middle, 3 = back, 4 = forward)
    if (button >= 0 && button < 5) {
        g_inputState.mouse.buttonState[button] = buttonDown;
    }
    
    // Forward to FBNeo mouse handling if needed
    if (button >= 0 && button < 3) {
        // Map button to FBNeo mouse button codes
        int fbMouseButton = 0;
        switch (button) {
            case 0: fbMouseButton = 0; break; // Left
            case 1: fbMouseButton = 1; break; // Right
            case 2: fbMouseButton = 2; break; // Middle
        }
        
        // Call into the FBNeo input system for mouse events
        // InputProcessMouseCallback would be defined in the FBNeo input system
        // InputProcessMouseCallback(fbMouseButton, x, y, buttonDown);
    }
}

// Process a gamepad event
void Metal_ProcessGamepadEvent(int gamepadIndex, int buttonIndex, float value) {
    if (gamepadIndex < 0 || gamepadIndex >= 4) {
        return; // Invalid gamepad index
    }
    
    // Make this the active gamepad if it's not already
    if (!g_inputState.isGamepadActive || g_inputState.activeGamepad != gamepadIndex) {
        g_inputState.isGamepadActive = true;
        g_inputState.activeGamepad = gamepadIndex;
    }
    
    // Digital button (value is 0.0 or 1.0)
    if (buttonIndex >= 0 && buttonIndex < 16) {
        g_inputState.gamepad[gamepadIndex].buttonState[buttonIndex] = (value > 0.5f);
        
        // Map to FBNeo key codes based on player
        int fbKey = 0;
        
        // First player
        if (gamepadIndex == 0) {
            switch (buttonIndex) {
                case 0: fbKey = FBK_A; break;     // A
                case 1: fbKey = FBK_B; break;     // B
                case 2: fbKey = FBK_C; break;     // X
                case 3: fbKey = FBK_D; break;     // Y
                case 4: fbKey = FBK_E; break;     // L
                case 5: fbKey = FBK_F; break;     // R
                case 6: fbKey = FBK_COIN; break;  // Back/Select
                case 7: fbKey = FBK_START; break; // Start
                // D-pad handled separately below
            }
        }
        // Second player
        else if (gamepadIndex == 1) {
            switch (buttonIndex) {
                case 0: fbKey = FBK_4A; break;     // A
                case 1: fbKey = FBK_4B; break;     // B
                case 2: fbKey = FBK_4C; break;     // X
                case 3: fbKey = FBK_4D; break;     // Y
                case 4: fbKey = FBK_4E; break;     // L
                case 5: fbKey = FBK_4F; break;     // R
                case 6: fbKey = FBK_4COIN; break;  // Back/Select
                case 7: fbKey = FBK_4START; break; // Start
                // D-pad handled separately below
            }
        }
        
        if (fbKey != 0) {
            // Forward to FBNeo input system
            InputSetState(fbKey, value > 0.5f ? 0x01 : 0x00);
        }
    }
    // Analog stick or trigger
    else if (buttonIndex >= 100) {
        // Axis values are between -1.0 and 1.0 (or 0.0 and 1.0 for triggers)
        switch (buttonIndex) {
            case 100: // Left stick X
                g_inputState.gamepad[gamepadIndex].leftStickX = value;
                // Map to D-pad if past threshold
                if (gamepadIndex == 0) {
                    if (value < -0.5f) InputSetState(FBK_LEFT, 0x01);
                    else if (value > 0.5f) InputSetState(FBK_RIGHT, 0x01);
                    else {
                        InputSetState(FBK_LEFT, 0x00);
                        InputSetState(FBK_RIGHT, 0x00);
                    }
                } else if (gamepadIndex == 1) {
                    if (value < -0.5f) InputSetState(FBK_4LEFT, 0x01);
                    else if (value > 0.5f) InputSetState(FBK_4RIGHT, 0x01);
                    else {
                        InputSetState(FBK_4LEFT, 0x00);
                        InputSetState(FBK_4RIGHT, 0x00);
                    }
                }
                break;
                
            case 101: // Left stick Y
                g_inputState.gamepad[gamepadIndex].leftStickY = value;
                // Map to D-pad if past threshold
                if (gamepadIndex == 0) {
                    if (value < -0.5f) InputSetState(FBK_DOWN, 0x01);
                    else if (value > 0.5f) InputSetState(FBK_UP, 0x01);
                    else {
                        InputSetState(FBK_DOWN, 0x00);
                        InputSetState(FBK_UP, 0x00);
                    }
                } else if (gamepadIndex == 1) {
                    if (value < -0.5f) InputSetState(FBK_4DOWN, 0x01);
                    else if (value > 0.5f) InputSetState(FBK_4UP, 0x01);
                    else {
                        InputSetState(FBK_4DOWN, 0x00);
                        InputSetState(FBK_4UP, 0x00);
                    }
                }
                break;
                
            case 102: // Right stick X
                g_inputState.gamepad[gamepadIndex].rightStickX = value;
                break;
                
            case 103: // Right stick Y
                g_inputState.gamepad[gamepadIndex].rightStickY = value;
                break;
                
            case 104: // Left trigger
                g_inputState.gamepad[gamepadIndex].leftTrigger = value;
                break;
                
            case 105: // Right trigger
                g_inputState.gamepad[gamepadIndex].rightTrigger = value;
                break;
        }
    }
}

// Update input state (called once per frame)
void Metal_UpdateInput() {
    // Store previous state for detecting transitions
    memcpy(&g_prevInputState, &g_inputState, sizeof(InputState));
    
    // Update gamepad state from GCController objects if available
    if (@available(macOS 10.9, *)) {
        NSArray<GCController *> *controllers = [GCController controllers];
        for (GCController *controller in controllers) {
            int playerIndex = (int)controller.playerIndex;
            if (playerIndex < 0 || playerIndex >= 4) continue;
            
            if (controller.extendedGamepad) {
                GCExtendedGamepad *gamepad = controller.extendedGamepad;
                
                // Process D-pad
                if (gamepad.dpad.up.isPressed) Metal_ProcessGamepadEvent(playerIndex, 8, 1.0f);
                else Metal_ProcessGamepadEvent(playerIndex, 8, 0.0f);
                
                if (gamepad.dpad.down.isPressed) Metal_ProcessGamepadEvent(playerIndex, 9, 1.0f);
                else Metal_ProcessGamepadEvent(playerIndex, 9, 0.0f);
                
                if (gamepad.dpad.left.isPressed) Metal_ProcessGamepadEvent(playerIndex, 10, 1.0f);
                else Metal_ProcessGamepadEvent(playerIndex, 10, 0.0f);
                
                if (gamepad.dpad.right.isPressed) Metal_ProcessGamepadEvent(playerIndex, 11, 1.0f);
                else Metal_ProcessGamepadEvent(playerIndex, 11, 0.0f);
                
                // Process main buttons
                Metal_ProcessGamepadEvent(playerIndex, 0, gamepad.buttonA.isPressed ? 1.0f : 0.0f);
                Metal_ProcessGamepadEvent(playerIndex, 1, gamepad.buttonB.isPressed ? 1.0f : 0.0f);
                Metal_ProcessGamepadEvent(playerIndex, 2, gamepad.buttonX.isPressed ? 1.0f : 0.0f);
                Metal_ProcessGamepadEvent(playerIndex, 3, gamepad.buttonY.isPressed ? 1.0f : 0.0f);
                
                // Process shoulder buttons
                Metal_ProcessGamepadEvent(playerIndex, 4, gamepad.leftShoulder.isPressed ? 1.0f : 0.0f);
                Metal_ProcessGamepadEvent(playerIndex, 5, gamepad.rightShoulder.isPressed ? 1.0f : 0.0f);
                
                // Process analog sticks
                Metal_ProcessGamepadEvent(playerIndex, 100, gamepad.leftThumbstick.xAxis.value);
                Metal_ProcessGamepadEvent(playerIndex, 101, gamepad.leftThumbstick.yAxis.value);
                Metal_ProcessGamepadEvent(playerIndex, 102, gamepad.rightThumbstick.xAxis.value);
                Metal_ProcessGamepadEvent(playerIndex, 103, gamepad.rightThumbstick.yAxis.value);
                
                // Process triggers
                Metal_ProcessGamepadEvent(playerIndex, 104, gamepad.leftTrigger.value);
                Metal_ProcessGamepadEvent(playerIndex, 105, gamepad.rightTrigger.value);
            }
        }
    }
}

// Check if a key is down
bool Metal_IsKeyDown(int keyCode) {
    if (keyCode >= 0 && keyCode < 256) {
        return g_inputState.keyboard.keyState[keyCode] != 0;
    }
    return false;
}

// Check if a key was just pressed this frame
bool Metal_IsKeyPressed(int keyCode) {
    if (keyCode >= 0 && keyCode < 256) {
        return g_inputState.keyboard.keyState[keyCode] != 0 && g_prevInputState.keyboard.keyState[keyCode] == 0;
    }
    return false;
}

// Check if a key was just released this frame
bool Metal_IsKeyReleased(int keyCode) {
    if (keyCode >= 0 && keyCode < 256) {
        return g_inputState.keyboard.keyState[keyCode] == 0 && g_prevInputState.keyboard.keyState[keyCode] != 0;
    }
    return false;
}

// Get the current mouse position
void Metal_GetMousePosition(int* x, int* y) {
    if (x) *x = g_inputState.mouse.x;
    if (y) *y = g_inputState.mouse.y;
}

// Set the mouse position
void Metal_SetMousePosition(int x, int y) {
    // Get the main screen
    NSScreen *mainScreen = [NSScreen mainScreen];
    if (!mainScreen) {
        NSLog(@"Could not get main screen");
        return;
    }
    
    // Convert coordinates to screen coordinates
    NSRect screenFrame = [mainScreen frame];
    
    // macOS origin is bottom-left, but we expect top-left origin
    CGPoint point = CGPointMake(x, screenFrame.size.height - y);
    
    // Move the mouse cursor
    CGWarpMouseCursorPosition(point);
    
    // Update our internal state
    g_inputState.mouse.x = x;
    g_inputState.mouse.y = y;
}

// Load key binding profiles
void Metal_LoadKeyBindings() {
    // In a real implementation, we would load from a plist or other preference file
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    
    // Load active profile index
    g_activeProfile = [defaults integerForKey:@"FBNeoActiveKeyProfile"];
    
    // Try to load saved profiles
    NSDictionary *savedProfiles = [defaults objectForKey:@"FBNeoKeyProfiles"];
    if (savedProfiles) {
        g_profileCount = 0;
        
        // Iterate through saved profiles
        for (NSString *profileName in savedProfiles) {
            NSArray *mappings = savedProfiles[profileName];
            
            if (g_profileCount < 8) {
                KeyBindingProfile *profile = &g_keyProfiles[g_profileCount++];
                profile->profileName = strdup([profileName UTF8String]);
                profile->mappingCount = 0;
                
                // Process each key mapping
                for (NSDictionary *mapping in mappings) {
                    int hwKey = [mapping[@"hw"] intValue];
                    int fbKey = [mapping[@"fb"] intValue];
                    
                    if (profile->mappingCount < 64) {
                        profile->mappings[profile->mappingCount].hardwareKeyCode = hwKey;
                        profile->mappings[profile->mappingCount].fbKeyCode = fbKey;
                        profile->mappingCount++;
                    }
                }
            }
        }
        
        NSLog(@"Loaded %d key binding profiles", g_profileCount);
    }
    
    // If no profiles were loaded, ensure we have at least the default
    if (g_profileCount == 0) {
        InitializeDefaultKeyBindings();
    }
}

// Save key binding profiles
void Metal_SaveKeyBindings() {
    NSMutableDictionary *profilesDict = [NSMutableDictionary dictionary];
    
    // Save each profile
    for (int i = 0; i < g_profileCount; i++) {
        KeyBindingProfile *profile = &g_keyProfiles[i];
        
        NSMutableArray *mappingsArray = [NSMutableArray array];
        
        // Save each mapping within the profile
        for (int j = 0; j < profile->mappingCount; j++) {
            KeyMapping *mapping = &profile->mappings[j];
            
            [mappingsArray addObject:@{
                @"hw": @(mapping->hardwareKeyCode),
                @"fb": @(mapping->fbKeyCode)
            }];
        }
        
        // Add to profiles dictionary
        NSString *profileName = [NSString stringWithUTF8String:profile->profileName];
        profilesDict[profileName] = mappingsArray;
    }
    
    // Save to user defaults
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    [defaults setObject:profilesDict forKey:@"FBNeoKeyProfiles"];
    [defaults setInteger:g_activeProfile forKey:@"FBNeoActiveKeyProfile"];
    [defaults synchronize];
    
    NSLog(@"Saved %d key binding profiles", g_profileCount);
}

// Set active key binding profile
void Metal_SetKeyBindingProfile(const char* profileName) {
    for (int i = 0; i < g_profileCount; i++) {
        if (strcmp(g_keyProfiles[i].profileName, profileName) == 0) {
            g_activeProfile = i;
            
            // Save the active profile to preferences
            NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
            [defaults setInteger:g_activeProfile forKey:@"FBNeoActiveKeyProfile"];
            [defaults synchronize];
            
            NSLog(@"Activated key profile: %s", profileName);
            return;
        }
    }
    
    NSLog(@"Profile not found: %s", profileName);
}

// Define a new key binding
void Metal_DefineKeyBinding(int hardwareKeyCode, int fbKeyCode) {
    if (g_activeProfile < 0 || g_activeProfile >= g_profileCount) {
        NSLog(@"No active profile to add key binding to");
        return;
    }
    
    KeyBindingProfile *profile = &g_keyProfiles[g_activeProfile];
    
    // Check if this key is already mapped
    for (int i = 0; i < profile->mappingCount; i++) {
        if (profile->mappings[i].hardwareKeyCode == hardwareKeyCode) {
            // Update existing mapping
            profile->mappings[i].fbKeyCode = fbKeyCode;
            NSLog(@"Updated key mapping: HW %d -> FB %d", hardwareKeyCode, fbKeyCode);
            return;
        }
    }
    
    // Add new mapping if there's room
    if (profile->mappingCount < 64) {
        profile->mappings[profile->mappingCount].hardwareKeyCode = hardwareKeyCode;
        profile->mappings[profile->mappingCount].fbKeyCode = fbKeyCode;
        profile->mappingCount++;
        
        NSLog(@"Added key mapping: HW %d -> FB %d", hardwareKeyCode, fbKeyCode);
    } else {
        NSLog(@"Profile has too many key mappings");
    }
}

// Remove a key binding
void Metal_RemoveKeyBinding(int hardwareKeyCode) {
    if (g_activeProfile < 0 || g_activeProfile >= g_profileCount) {
        NSLog(@"No active profile to remove key binding from");
        return;
    }
    
    KeyBindingProfile *profile = &g_keyProfiles[g_activeProfile];
    
    // Find and remove the mapping
    for (int i = 0; i < profile->mappingCount; i++) {
        if (profile->mappings[i].hardwareKeyCode == hardwareKeyCode) {
            // Move all subsequent mappings up
            for (int j = i; j < profile->mappingCount - 1; j++) {
                profile->mappings[j] = profile->mappings[j + 1];
            }
            
            profile->mappingCount--;
            NSLog(@"Removed key mapping for hardware key %d", hardwareKeyCode);
            return;
        }
    }
    
    NSLog(@"No mapping found for hardware key %d", hardwareKeyCode);
}

// Get the active input state
InputState* Metal_GetInputState() {
    return &g_inputState;
}

// Private helper functions

// Initialize default key bindings
static void InitializeDefaultKeyBindings() {
    if (g_profileCount >= 8) {
        return; // Already have max profiles
    }
    
    KeyBindingProfile *profile = &g_keyProfiles[g_profileCount++];
    profile->profileName = "Default";
    profile->mappingCount = kDefaultKeyMappingCount;
    
    // Copy the default mappings
    for (int i = 0; i < kDefaultKeyMappingCount; i++) {
        profile->mappings[i] = kDefaultKeyMappings[i];
    }
    
    // Make this the active profile
    g_activeProfile = g_profileCount - 1;
    
    NSLog(@"Initialized default key bindings with %d mappings", kDefaultKeyMappingCount);
}

// Translate Mac key code to FBNeo key code based on active profile
static int TranslateMacKeyToFBNeoKey(int macKeyCode) {
    if (g_activeProfile < 0 || g_activeProfile >= g_profileCount) {
        return 0; // No active profile
    }
    
    KeyBindingProfile *profile = &g_keyProfiles[g_activeProfile];
    
    // Look for a mapping
    for (int i = 0; i < profile->mappingCount; i++) {
        if (profile->mappings[i].hardwareKeyCode == macKeyCode) {
            return profile->mappings[i].fbKeyCode;
        }
    }
    
    return 0; // No mapping found
}

// Check if a key is a modifier key
static bool IsModifierKey(int macKeyCode) {
    switch (macKeyCode) {
        case kVK_Shift:
        case kVK_RightShift:
        case kVK_Control:
        case kVK_RightControl:
        case kVK_Option:
        case kVK_RightOption:
        case kVK_Command:
        case kVK_RightCommand:
            return true;
        default:
            return false;
    }
}

// Get the index for a modifier key
static int GetModifierIndex(int macKeyCode) {
    switch (macKeyCode) {
        case kVK_Shift: return 0;
        case kVK_RightShift: return 1;
        case kVK_Control: return 2;
        case kVK_RightControl: return 3;
        case kVK_Option: return 4;
        case kVK_RightOption: return 5;
        case kVK_Command: return 6;
        case kVK_RightCommand: return 7;
        default: return -1;
    }
} 