#import <Cocoa/Cocoa.h>
#import <GameController/GameController.h>
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>

#include "metal_compat_layer.h"
#include "metal_input_defs.h"
#include <stdio.h>
#include <string.h>

// External declarations
extern "C" {
    INT32 BurnDrvSetInput(INT32 i, INT32 nState);
    INT32 BurnDrvReset();
    INT32 Metal_SaveState(INT32 nSlot);
    INT32 Metal_LoadState(INT32 nSlot);
    INT32 Metal_QuickSave();
    INT32 Metal_QuickLoad();
    void Metal_ToggleDebugOverlay();
    void Metal_RequestQuit();
}

// Maximum number of supported players
#define MAX_PLAYERS 4

// Maximum number of buttons per player
#define MAX_BUTTONS 16

// Input state tracking
static bool g_keyStates[256] = {false};
static bool g_inputInitialized = false;
static NSMutableArray<GCController*>* g_controllers = nil;
static bool g_gamepadEnabled = false;
static bool g_keyboardEnabled = true;
static id g_controllerConnectObserver = nil;
static id g_controllerDisconnectObserver = nil;

// Process controller input
static void processControllerInput() {
    if (g_controllers.count == 0 || !g_gamepadEnabled) return;
    
    for (GCController* controller in g_controllers) {
        if (controller.extendedGamepad) {
            GCExtendedGamepad* gamepad = controller.extendedGamepad;
            
            // D-pad
            BurnDrvSetInput(P1_UP, gamepad.dpad.up.pressed ? 0x01 : 0x00);
            BurnDrvSetInput(P1_DOWN, gamepad.dpad.down.pressed ? 0x01 : 0x00);
            BurnDrvSetInput(P1_LEFT, gamepad.dpad.left.pressed ? 0x01 : 0x00);
            BurnDrvSetInput(P1_RIGHT, gamepad.dpad.right.pressed ? 0x01 : 0x00);
            
            // Face buttons
            BurnDrvSetInput(P1_WEAK_PUNCH, gamepad.buttonA.pressed ? 0x01 : 0x00);
            BurnDrvSetInput(P1_MED_PUNCH, gamepad.buttonB.pressed ? 0x01 : 0x00);
            BurnDrvSetInput(P1_STRONG_PUNCH, gamepad.buttonX.pressed ? 0x01 : 0x00);
            BurnDrvSetInput(P1_WEAK_KICK, gamepad.buttonY.pressed ? 0x01 : 0x00);
            
            // Shoulder buttons
            BurnDrvSetInput(P1_MED_KICK, gamepad.leftShoulder.pressed ? 0x01 : 0x00);
            BurnDrvSetInput(P1_STRONG_KICK, gamepad.rightShoulder.pressed ? 0x01 : 0x00);
            
            // Triggers (for additional functions)
            if (gamepad.leftTrigger.pressed) {
                BurnDrvSetInput(P1_START, 0x01);
            } else {
                BurnDrvSetInput(P1_START, 0x00);
            }
            
            if (gamepad.rightTrigger.pressed) {
                BurnDrvSetInput(P1_COIN, 0x01);
            } else {
                BurnDrvSetInput(P1_COIN, 0x00);
            }
            
            // Analog stick - convert to D-pad input if pushed far enough
            float leftStickX = gamepad.leftThumbstick.xAxis.value;
            float leftStickY = gamepad.leftThumbstick.yAxis.value;
            
            // Use left stick as alternative D-pad with dead zone
            if (leftStickY < -0.5f) BurnDrvSetInput(P1_UP, 0x01);
            if (leftStickY > 0.5f) BurnDrvSetInput(P1_DOWN, 0x01);
            if (leftStickX < -0.5f) BurnDrvSetInput(P1_LEFT, 0x01);
            if (leftStickX > 0.5f) BurnDrvSetInput(P1_RIGHT, 0x01);
        }
    }
}

// Initialize input system
INT32 Metal_InitInput() {
    printf("[Metal_InitInput] Initializing Metal input system\n");
    
    // Initialize FBNeo input system
    BurnInputInit();
    
    // Initialize controller handling
    g_controllers = [NSMutableArray array];
    
    // Add controllers
    for (GCController* controller in [GCController controllers]) {
        [g_controllers addObject:controller];
    }
    
    printf("[Metal_InitInput] Found %lu controllers\n", (unsigned long)g_controllers.count);
    
    // Set up notifications for controller connections/disconnections
    g_controllerConnectObserver = [[NSNotificationCenter defaultCenter] 
                                   addObserverForName:GCControllerDidConnectNotification
                                   object:nil
                                   queue:[NSOperationQueue mainQueue]
                                   usingBlock:^(NSNotification *note) {
        GCController* controller = note.object;
        printf("[Metal_InitInput] Controller connected: %s\n", controller.vendorName.UTF8String);
        if (![g_controllers containsObject:controller]) {
            [g_controllers addObject:controller];
        }
    }];
    
    g_controllerDisconnectObserver = [[NSNotificationCenter defaultCenter] 
                                      addObserverForName:GCControllerDidDisconnectNotification
                                      object:nil
                                      queue:[NSOperationQueue mainQueue]
                                      usingBlock:^(NSNotification *note) {
        GCController* controller = note.object;
        printf("[Metal_InitInput] Controller disconnected: %s\n", controller.vendorName.UTF8String);
        [g_controllers removeObject:controller];
    }];
    
    // Start wireless controller discovery
    [GCController startWirelessControllerDiscoveryWithCompletionHandler:^{
        printf("[Metal_InitInput] Wireless controller discovery completed\n");
    }];
    
    g_gamepadEnabled = (g_controllers.count > 0);
    g_keyboardEnabled = true;
    g_inputInitialized = true;
    
    printf("[Metal_InitInput] Input system initialized successfully\n");
    printf("[Metal_InitInput] Keyboard: %s\n", g_keyboardEnabled ? "Enabled" : "Disabled");
    printf("[Metal_InitInput] Controllers: %s (%lu connected)\n", 
          g_gamepadEnabled ? "Enabled" : "Disabled",
          (unsigned long)g_controllers.count);
          
    return 0;
}

// Exit input system
INT32 Metal_ExitInput() {
    printf("[Metal_ExitInput] Shutting down input system\n");
    
    // Remove controller observers
    if (g_controllerConnectObserver) {
        [[NSNotificationCenter defaultCenter] removeObserver:g_controllerConnectObserver];
        g_controllerConnectObserver = nil;
    }
    
    if (g_controllerDisconnectObserver) {
        [[NSNotificationCenter defaultCenter] removeObserver:g_controllerDisconnectObserver];
        g_controllerDisconnectObserver = nil;
    }
    
    // Clean up resources
    g_controllers = nil;
    g_inputInitialized = false;
    
    // Exit FBNeo input system
    BurnInputExit();
    
    printf("[Metal_ExitInput] Input system shutdown complete\n");
    return 0;
}

// Process all input and send to FBNeo core
void Metal_ProcessInput() {
    if (!g_inputInitialized) {
        // Initialize the input system if not already done
        printf("[Metal_ProcessInput] Input system not initialized, initializing now\n");
        Metal_InitInput();
        
        if (!g_inputInitialized) {
            printf("[Metal_ProcessInput] ERROR: Failed to initialize input system\n");
            return;
        }
    }
    
    // Process controller input if available
    if (g_gamepadEnabled) {
        processControllerInput();
    }
    
    // Process keyboard input
    if (g_keyboardEnabled) {
        // Player 1 controls
        if (g_keyStates[kVK_UpArrow]) BurnDrvSetInput(P1_UP, 0x01);
        else BurnDrvSetInput(P1_UP, 0x00);
        
        if (g_keyStates[kVK_DownArrow]) BurnDrvSetInput(P1_DOWN, 0x01);
        else BurnDrvSetInput(P1_DOWN, 0x00);
        
        if (g_keyStates[kVK_LeftArrow]) BurnDrvSetInput(P1_LEFT, 0x01);
        else BurnDrvSetInput(P1_LEFT, 0x00);
        
        if (g_keyStates[kVK_RightArrow]) BurnDrvSetInput(P1_RIGHT, 0x01);
        else BurnDrvSetInput(P1_RIGHT, 0x00);
        
        if (g_keyStates[kVK_ANSI_Z]) BurnDrvSetInput(P1_WEAK_PUNCH, 0x01);
        else BurnDrvSetInput(P1_WEAK_PUNCH, 0x00);
        
        if (g_keyStates[kVK_ANSI_X]) BurnDrvSetInput(P1_MED_PUNCH, 0x01);
        else BurnDrvSetInput(P1_MED_PUNCH, 0x00);
        
        if (g_keyStates[kVK_ANSI_C]) BurnDrvSetInput(P1_STRONG_PUNCH, 0x01);
        else BurnDrvSetInput(P1_STRONG_PUNCH, 0x00);
        
        if (g_keyStates[kVK_ANSI_V]) BurnDrvSetInput(P1_WEAK_KICK, 0x01);
        else BurnDrvSetInput(P1_WEAK_KICK, 0x00);
        
        if (g_keyStates[kVK_ANSI_B]) BurnDrvSetInput(P1_MED_KICK, 0x01);
        else BurnDrvSetInput(P1_MED_KICK, 0x00);
        
        if (g_keyStates[kVK_ANSI_N]) BurnDrvSetInput(P1_STRONG_KICK, 0x01);
        else BurnDrvSetInput(P1_STRONG_KICK, 0x00);
        
        // Player 1 system buttons
        if (g_keyStates[kVK_ANSI_1]) BurnDrvSetInput(P1_START, 0x01);
        else BurnDrvSetInput(P1_START, 0x00);
        
        if (g_keyStates[kVK_ANSI_5]) BurnDrvSetInput(P1_COIN, 0x01);
        else BurnDrvSetInput(P1_COIN, 0x00);
        
        // Player 2 controls
        if (g_keyStates[kVK_ANSI_W]) BurnDrvSetInput(P2_UP, 0x01);
        else BurnDrvSetInput(P2_UP, 0x00);
        
        if (g_keyStates[kVK_ANSI_S]) BurnDrvSetInput(P2_DOWN, 0x01);
        else BurnDrvSetInput(P2_DOWN, 0x00);
        
        if (g_keyStates[kVK_ANSI_A]) BurnDrvSetInput(P2_LEFT, 0x01);
        else BurnDrvSetInput(P2_LEFT, 0x00);
        
        if (g_keyStates[kVK_ANSI_D]) BurnDrvSetInput(P2_RIGHT, 0x01);
        else BurnDrvSetInput(P2_RIGHT, 0x00);
        
        if (g_keyStates[kVK_ANSI_Q]) BurnDrvSetInput(P2_WEAK_PUNCH, 0x01);
        else BurnDrvSetInput(P2_WEAK_PUNCH, 0x00);
        
        if (g_keyStates[kVK_ANSI_E]) BurnDrvSetInput(P2_MED_PUNCH, 0x01);
        else BurnDrvSetInput(P2_MED_PUNCH, 0x00);
        
        if (g_keyStates[kVK_ANSI_R]) BurnDrvSetInput(P2_STRONG_PUNCH, 0x01);
        else BurnDrvSetInput(P2_STRONG_PUNCH, 0x00);
        
        if (g_keyStates[kVK_ANSI_T]) BurnDrvSetInput(P2_WEAK_KICK, 0x01);
        else BurnDrvSetInput(P2_WEAK_KICK, 0x00);
        
        if (g_keyStates[kVK_ANSI_Y]) BurnDrvSetInput(P2_MED_KICK, 0x01);
        else BurnDrvSetInput(P2_MED_KICK, 0x00);
        
        if (g_keyStates[kVK_ANSI_H]) BurnDrvSetInput(P2_STRONG_KICK, 0x01);
        else BurnDrvSetInput(P2_STRONG_KICK, 0x00);
        
        // Player 2 system buttons
        if (g_keyStates[kVK_ANSI_2]) BurnDrvSetInput(P2_START, 0x01);
        else BurnDrvSetInput(P2_START, 0x00);
        
        if (g_keyStates[kVK_ANSI_6]) BurnDrvSetInput(P2_COIN, 0x01);
        else BurnDrvSetInput(P2_COIN, 0x00);
        
        // Special keys
        if (g_keyStates[kVK_F3]) {
            // Reset
            extern INT32 BurnDrvReset();
            BurnDrvReset();
        }
    }
}

// Handle key down event
INT32 Metal_HandleKeyDown(int keyCode) {
    if (keyCode >= 0 && keyCode < 256) {
        g_keyStates[keyCode] = true;
        
        // Special keys that need immediate processing
        if (keyCode == kVK_F1) {
            // Toggle debug overlay
            extern void Metal_ToggleDebugOverlay();
            Metal_ToggleDebugOverlay();
        } else if (keyCode == kVK_F5) {
            // Quick save
            extern INT32 Metal_SaveState(INT32 nSlot);
            Metal_SaveState(1);
        } else if (keyCode == kVK_F8) {
            // Quick load
            extern INT32 Metal_LoadState(INT32 nSlot);
            Metal_LoadState(1);
        } else if (keyCode == kVK_Escape) {
            // Quit
            extern void Metal_RequestQuit();
            Metal_RequestQuit();
        }
    }
    return 0;
}

// Handle key up event
INT32 Metal_HandleKeyUp(int keyCode) {
    if (keyCode >= 0 && keyCode < 256) {
        g_keyStates[keyCode] = false;
    }
    return 0;
}

// Handle Command key combinations (⌘S, ⌘L)
INT32 Metal_HandleCommandKey(int keyCode, bool isDown) {
    if (!isDown) return 0;
    
    switch (keyCode) {
        case kVK_ANSI_S:  // Command+S = Save State
            extern INT32 Metal_QuickSave();
            return Metal_QuickSave();
            
        case kVK_ANSI_L:  // Command+L = Load State
            extern INT32 Metal_QuickLoad();
            return Metal_QuickLoad();
            
        case kVK_ANSI_R:  // Command+R = Reset
            extern INT32 BurnDrvReset();
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

// C interface functions
extern "C" {
    INT32 Metal_InitInput_C() {
        return Metal_InitInput();
    }
    
    INT32 Metal_ExitInput_C() {
        return Metal_ExitInput();
    }
    
    INT32 Metal_HandleKeyDown_C(int keyCode) {
        return Metal_HandleKeyDown(keyCode);
    }
    
    INT32 Metal_HandleKeyUp_C(int keyCode) {
        return Metal_HandleKeyUp(keyCode);
    }
    
    void Metal_ProcessInput_C() {
        Metal_ProcessInput();
    }
} 