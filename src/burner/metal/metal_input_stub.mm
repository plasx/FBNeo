#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>
#import <GameController/GameController.h>
#include <stdio.h>
#include <stdlib.h>
#include "metal_input_stubs.h"
#include "metal_input_defs.h"

// Forward declarations for functions called in this file
extern "C" INT32 Metal_SaveState(INT32 nSlot);
extern "C" INT32 Metal_LoadState(INT32 nSlot);
extern "C" INT32 BurnDrvReset();

// Modified functions to avoid duplicates
// Instead of implementing Metal_RequestQuit directly, create a new function that calls it
static void RequestQuitFromInput() {
    extern void Metal_RequestQuit();
    Metal_RequestQuit();
}

// Same for debug overlay
static void ToggleDebugOverlayFromInput() {
    extern void Metal_ToggleDebugOverlay();
    Metal_ToggleDebugOverlay();
}

// Input configuration for CPS2 Marvel vs Capcom
typedef struct {
    int keyCode;   // macOS key code
    int inputId;   // FBNeo input ID
    const char* description; // Description for debugging
} InputMapping;

// Input mapping for Marvel vs Capcom controls
static InputMapping g_inputMappings[] = {
    // Player 1 controls - Arrow keys + Z,X,C,V,B,N keys
    { kVK_UpArrow,    P1_UP,          "P1 Up" },
    { kVK_DownArrow,  P1_DOWN,        "P1 Down" },
    { kVK_LeftArrow,  P1_LEFT,        "P1 Left" },
    { kVK_RightArrow, P1_RIGHT,       "P1 Right" },
    { kVK_ANSI_Z,     P1_WEAK_PUNCH,  "P1 Weak Punch" },
    { kVK_ANSI_X,     P1_MED_PUNCH,   "P1 Medium Punch" },
    { kVK_ANSI_C,     P1_STRONG_PUNCH,"P1 Strong Punch" },
    { kVK_ANSI_V,     P1_WEAK_KICK,   "P1 Weak Kick" },
    { kVK_ANSI_B,     P1_MED_KICK,    "P1 Medium Kick" },
    { kVK_ANSI_N,     P1_STRONG_KICK, "P1 Strong Kick" },
    { kVK_Return,     P1_START,       "P1 Start" },
    { kVK_Space,      P1_COIN,        "P1 Coin" },
    
    // Player 2 controls - WASD + Q,W,E,R,T,Y keys
    { kVK_ANSI_W,     P2_UP,          "P2 Up" },
    { kVK_ANSI_S,     P2_DOWN,        "P2 Down" },
    { kVK_ANSI_A,     P2_LEFT,        "P2 Left" },
    { kVK_ANSI_D,     P2_RIGHT,       "P2 Right" },
    { kVK_ANSI_Q,     P2_WEAK_PUNCH,  "P2 Weak Punch" },
    { kVK_ANSI_W,     P2_MED_PUNCH,   "P2 Medium Punch" },
    { kVK_ANSI_E,     P2_STRONG_PUNCH,"P2 Strong Punch" },
    { kVK_ANSI_R,     P2_WEAK_KICK,   "P2 Weak Kick" },
    { kVK_ANSI_T,     P2_MED_KICK,    "P2 Medium Kick" },
    { kVK_ANSI_Y,     P2_STRONG_KICK, "P2 Strong Kick" },
    { kVK_Tab,        P2_START,       "P2 Start" },
    { kVK_CapsLock,   P2_COIN,        "P2 Coin" },
    
    // System functions
    { kVK_F3,         RESET,          "Reset Game" },
    { kVK_F1,         TOGGLE_OVERLAY, "Toggle Debug Overlay" },
    { kVK_F5,         SAVE_STATE,     "Save State" },
    { kVK_F8,         LOAD_STATE,     "Load State" },
    { kVK_Escape,     QUIT,           "Quit" },
    
    // End marker
    { -1, -1, NULL }
};

// Define some globals to track input state
static bool g_inputInitialized = false;
static bool g_keyboardEnabled = true;
static bool g_gamepadEnabled = false;
static int g_keyStates[256] = {0}; // Simple keyboard state array
static NSArray<GCController *> *g_controllers = nil;

// External declaration for BurnDrvSetInput function
extern "C" INT32 BurnDrvSetInput(INT32 i, INT32 nState);

// Initialize input system
INT32 Metal_InitInput() {
    // Clear key states
    memset(g_keyStates, 0, sizeof(g_keyStates));
    
    // Initialize with keyboard enabled
    g_keyboardEnabled = true;
    g_gamepadEnabled = false;
    
    // Set up GameController framework
    NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
    
    // Register for controller connection notifications
    [center addObserverForName:GCControllerDidConnectNotification
                        object:nil
                         queue:[NSOperationQueue mainQueue]
                    usingBlock:^(NSNotification *note) {
        GCController *controller = note.object;
        NSLog(@"Controller connected: %@", controller.vendorName ?: @"Unknown");
        g_gamepadEnabled = true;
        g_controllers = [GCController controllers];
    }];
    
    // Register for controller disconnection notifications
    [center addObserverForName:GCControllerDidDisconnectNotification
                        object:nil
                         queue:[NSOperationQueue mainQueue]
                    usingBlock:^(NSNotification *note) {
        NSLog(@"Controller disconnected");
        g_controllers = [GCController controllers];
        g_gamepadEnabled = (g_controllers.count > 0);
    }];
    
    // Start wireless controller discovery
    [GCController startWirelessControllerDiscoveryWithCompletionHandler:^{
        NSLog(@"Wireless controller discovery completed");
    }];
    
    // Check for already-connected controllers
    g_controllers = [GCController controllers];
    g_gamepadEnabled = (g_controllers.count > 0);
    
    if (g_gamepadEnabled) {
        NSLog(@"Found %lu connected controller(s)", (unsigned long)g_controllers.count);
        for (GCController *controller in g_controllers) {
            NSLog(@"- %@", controller.vendorName ?: @"Unknown");
        }
    }
    
    g_inputInitialized = true;
    
    NSLog(@"Input system initialized:");
    NSLog(@"  - Keyboard: %@", g_keyboardEnabled ? @"Enabled" : @"Disabled");
    NSLog(@"  - Gamepad: %@ (%@)", 
          g_gamepadEnabled ? @"Enabled" : @"Disabled",
          g_gamepadEnabled ? @"Connected" : @"Not connected");
    
    return 0; // Success
}

// Shutdown input system
INT32 Metal_ExitInput() {
    // Clean up GameController framework resources
    [[NSNotificationCenter defaultCenter] removeObserver:nil 
                                                   name:GCControllerDidConnectNotification 
                                                 object:nil];
    
    [[NSNotificationCenter defaultCenter] removeObserver:nil 
                                                   name:GCControllerDidDisconnectNotification 
                                                 object:nil];
    
    g_controllers = nil;
    g_inputInitialized = false;
    
    return 0; // Success
}

// Process gamepad input
void ProcessGamepadInput() {
    if (!g_gamepadEnabled || g_controllers.count == 0) {
        return;
    }
    
    // Process each connected controller
    for (GCController *controller in g_controllers) {
        // Modern way to check for different controller profiles
        if (controller.extendedGamepad) {
            GCExtendedGamepad *gamepad = controller.extendedGamepad;
            
            // Player 1 D-pad
            BurnDrvSetInput(P1_UP, gamepad.dpad.up.isPressed ? 0x01 : 0x00);
            BurnDrvSetInput(P1_DOWN, gamepad.dpad.down.isPressed ? 0x01 : 0x00);
            BurnDrvSetInput(P1_LEFT, gamepad.dpad.left.isPressed ? 0x01 : 0x00);
            BurnDrvSetInput(P1_RIGHT, gamepad.dpad.right.isPressed ? 0x01 : 0x00);
            
            // Player 1 buttons
            BurnDrvSetInput(P1_WEAK_PUNCH, gamepad.buttonA.isPressed ? 0x01 : 0x00);
            BurnDrvSetInput(P1_MED_PUNCH, gamepad.buttonB.isPressed ? 0x01 : 0x00);
            BurnDrvSetInput(P1_STRONG_PUNCH, gamepad.buttonX.isPressed ? 0x01 : 0x00);
            BurnDrvSetInput(P1_WEAK_KICK, gamepad.buttonY.isPressed ? 0x01 : 0x00);
            BurnDrvSetInput(P1_MED_KICK, gamepad.leftShoulder.isPressed ? 0x01 : 0x00);
            BurnDrvSetInput(P1_STRONG_KICK, gamepad.rightShoulder.isPressed ? 0x01 : 0x00);
            
            // Start/Coin
            BurnDrvSetInput(P1_START, gamepad.buttonOptions ? gamepad.buttonOptions.isPressed : 0x00);
            BurnDrvSetInput(P1_COIN, gamepad.buttonMenu ? gamepad.buttonMenu.isPressed : 0x00);
            
            // Use left thumbstick as alternative D-pad
            float leftX = gamepad.leftThumbstick.xAxis.value;
            float leftY = gamepad.leftThumbstick.yAxis.value;
            
            if (leftX < -0.5f) BurnDrvSetInput(P1_LEFT, 0x01);
            if (leftX > 0.5f) BurnDrvSetInput(P1_RIGHT, 0x01);
            if (leftY < -0.5f) BurnDrvSetInput(P1_UP, 0x01);
            if (leftY > 0.5f) BurnDrvSetInput(P1_DOWN, 0x01);
        }
    }
}

// Process input on each frame
void Metal_ProcessInput() {
    if (!g_inputInitialized) {
        return;
    }
    
    // Process keyboard input - set all mapped inputs based on current key states
    if (g_keyboardEnabled) {
        for (int i = 0; g_inputMappings[i].description != NULL; i++) {
            int keyCode = g_inputMappings[i].keyCode;
            int inputId = g_inputMappings[i].inputId;
            
            if (keyCode >= 0 && keyCode < 256) {
                bool keyPressed = (g_keyStates[keyCode] != 0);
                
                // Handle special system inputs
                if (keyPressed) {
                    switch (inputId) {
                        case RESET:
                            BurnDrvReset();
                            break;
                            
                        case TOGGLE_OVERLAY:
                            ToggleDebugOverlayFromInput();
                            break;
                            
                        case SAVE_STATE:
                            Metal_SaveState(1); // Save to slot 1
                            break;
                            
                        case LOAD_STATE:
                            Metal_LoadState(1); // Load from slot 1
                            break;
                            
                        case QUIT:
                            RequestQuitFromInput();
                            break;
                            
                        default:
                            // For regular inputs, pass to the emulation core
                            BurnDrvSetInput(inputId, keyPressed ? 0x01 : 0x00);
                            break;
                    }
                } else {
                    // For regular button releases, pass to the emulation core
                    if (inputId < RESET) {
                        BurnDrvSetInput(inputId, 0x00);
                    }
                }
            }
        }
    }
    
    // Process gamepad input (if enabled and controllers are connected)
    if (g_gamepadEnabled) {
        ProcessGamepadInput();
    }
}

// Handle key down event from AppKit
INT32 Metal_HandleKeyDown(int keyCode) {
    if (keyCode >= 0 && keyCode < 256) {
        g_keyStates[keyCode] = 1;
    }
    return 0;
}

// Handle key up event from AppKit
INT32 Metal_HandleKeyUp(int keyCode) {
    if (keyCode >= 0 && keyCode < 256) {
        g_keyStates[keyCode] = 0;
    }
    return 0;
}

// Handle command key combinations
INT32 Metal_HandleCommandKey(int keyCode, bool isDown) {
    if (!isDown) return 0;
    
    switch (keyCode) {
        case kVK_ANSI_S:  // Command+S = Save State
            return Metal_QuickSave();
            
        case kVK_ANSI_L:  // Command+L = Load State
            return Metal_QuickLoad();
            
        case kVK_ANSI_R:  // Command+R = Reset
            return BurnDrvReset();
            
        default:
            return 0;
    }
}

// Get active inputs (for debug overlay)
int Metal_GetActiveInputs() {
    int count = 0;
    
    // Count active keyboard inputs
    for (int i = 0; i < 256; i++) {
        if (g_keyStates[i]) count++;
    }
    
    return count;
} 