#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>
#import <GameController/GameController.h>
#import "FBInput.h"
#import "FBInputMap.h"
#import "FBInputConstants.h"

// C-compatible interface
#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
int Metal_InputInit_UNUSED();
int Metal_InputFrame();
int Metal_InputExit_UNUSED();
bool Metal_InputState(unsigned int key);
int Metal_InputSetCooperativeLevel(void* hWnd, int nInput);
void Metal_InputKeyboardEmulation(bool bEnable);

// External variables from FBNeo core
extern unsigned char* FBNeoKeyState;
extern int FBNeoInputFlags;
extern unsigned int FBNeoInputBitmasks[5][26];

#ifdef __cplusplus
}
#endif

// ============================================================================
// Metal Input System Implementation
// ============================================================================

// Input system state
static struct {
    bool initialized;
    NSMutableArray* controllers;
    NSMutableDictionary* keyboardMap;
    NSMutableDictionary* gamepadMap;
    int cooperativeLevel;
    bool keyboardEmulation;
    unsigned char keyState[512];
} g_inputState = {
    .initialized = false,
    .controllers = nil,
    .keyboardMap = nil,
    .gamepadMap = nil,
    .cooperativeLevel = 0,
    .keyboardEmulation = false
};

// Controller connection/disconnection notification handlers
static id controllerConnectObserver = nil;
static id controllerDisconnectObserver = nil;

// Key constants for Metal input
typedef NS_ENUM(NSInteger, MetalInputType) {
    kMetalInputKeyboard = 0,
    kMetalInputGamepad = 1,
    kMetalInputMouse = 2
};

// Initialize the input system
int Metal_InputInit_UNUSED() {
    // If already initialized, return success
    if (g_inputState.initialized) {
        return 0;
    }
    
    NSLog(@"Metal_InputInit: Initializing Metal input system");
    
    // Initialize state
    memset(g_inputState.keyState, 0, sizeof(g_inputState.keyState));
    g_inputState.cooperativeLevel = 0;
    g_inputState.keyboardEmulation = false;
    
    // Create collections
    g_inputState.controllers = [[NSMutableArray alloc] init];
    g_inputState.keyboardMap = [[NSMutableDictionary alloc] init];
    g_inputState.gamepadMap = [[NSMutableDictionary alloc] init];
    
    // Set up keyboard mapping
    [FBInputMap setupDefaultKeyboardMap:g_inputState.keyboardMap];
    
    // Set up game controller mapping
    [FBInputMap setupDefaultGamepadMap:g_inputState.gamepadMap];
    
    // Register for controller connection notifications
    NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
    
    controllerConnectObserver = [center addObserverForName:GCControllerDidConnectNotification 
                                                    object:nil 
                                                     queue:[NSOperationQueue mainQueue] 
                                                usingBlock:^(NSNotification* note) {
        GCController* controller = note.object;
        if (controller && ![g_inputState.controllers containsObject:controller]) {
            NSLog(@"Controller connected: %@", controller.vendorName);
            [g_inputState.controllers addObject:controller];
            
            // Configure controller input handlers
            [FBInput configureController:controller];
        }
    }];
    
    controllerDisconnectObserver = [center addObserverForName:GCControllerDidDisconnectNotification 
                                                       object:nil 
                                                        queue:[NSOperationQueue mainQueue] 
                                                   usingBlock:^(NSNotification* note) {
        GCController* controller = note.object;
        if (controller) {
            NSLog(@"Controller disconnected: %@", controller.vendorName);
            [g_inputState.controllers removeObject:controller];
        }
    }];
    
    // Start controller discovery
    [GCController startWirelessControllerDiscoveryWithCompletionHandler:^{
        NSLog(@"Controller discovery completed");
    }];
    
    // Get already-connected controllers
    NSArray* controllers = [GCController controllers];
    for (GCController* controller in controllers) {
        NSLog(@"Found connected controller: %@", controller.vendorName);
        [g_inputState.controllers addObject:controller];
        
        // Configure controller input handlers
        [FBInput configureController:controller];
    }
    
    // Register for key events from the main application
    [[NSNotificationCenter defaultCenter] addObserver:[FBInput sharedInstance]
                                             selector:@selector(handleKeyEvent:)
                                                 name:@"FBNeoKeyEvent"
                                               object:nil];
    
    // Mark as initialized
    g_inputState.initialized = true;
    
    NSLog(@"Metal_InputInit: Input system initialized successfully");
    return 0;
}

// Process input for the current frame
int Metal_InputFrame() {
    // If not initialized, return failure
    if (!g_inputState.initialized) {
        return 1;
    }
    
    // Get current input state from FBInput
    [[FBInput sharedInstance] updateInputState:g_inputState.keyState];
    
    // Update FBNeo core key state
    if (FBNeoKeyState) {
        // Copy our internal key state to FBNeo's key state
        memcpy(FBNeoKeyState, g_inputState.keyState, 512);
    }
    
    // Process keyboard emulation if enabled
    if (g_inputState.keyboardEmulation) {
        // This would map gamepad inputs to keyboard inputs
        // Implementation would depend on specific emulation needs
    }
    
    return 0;
}

// Shutdown the input system
int Metal_InputExit_UNUSED() {
    // If not initialized, do nothing
    if (!g_inputState.initialized) {
        return 0;
    }
    
    NSLog(@"Metal_InputExit: Shutting down Metal input system");
    
    // Stop controller discovery
    [GCController stopWirelessControllerDiscovery];
    
    // Remove notification observers
    NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
    if (controllerConnectObserver) {
        [center removeObserver:controllerConnectObserver];
        controllerConnectObserver = nil;
    }
    
    if (controllerDisconnectObserver) {
        [center removeObserver:controllerDisconnectObserver];
        controllerDisconnectObserver = nil;
    }
    
    // Remove key event observer
    [center removeObserver:[FBInput sharedInstance] name:@"FBNeoKeyEvent" object:nil];
    
    // Release controller array and maps
    g_inputState.controllers = nil;
    g_inputState.keyboardMap = nil;
    g_inputState.gamepadMap = nil;
    
    // Reset state
    memset(g_inputState.keyState, 0, sizeof(g_inputState.keyState));
    g_inputState.initialized = false;
    
    NSLog(@"Metal_InputExit: Input system shutdown complete");
    return 0;
}

// Check the state of a specific input key
bool Metal_InputState(unsigned int key) {
    // If not initialized, return false
    if (!g_inputState.initialized) {
        return false;
    }
    
    // Check key state (ensure key is within bounds)
    if (key < 512) {
        return (g_inputState.keyState[key] != 0);
    }
    
    return false;
}

// Set the cooperative level for input handling
int Metal_InputSetCooperativeLevel(void* hWnd, int nInput) {
    g_inputState.cooperativeLevel = nInput;
    return 0;
}

// Enable or disable keyboard emulation for gamepads
void Metal_InputKeyboardEmulation(bool bEnable) {
    g_inputState.keyboardEmulation = bEnable;
}

// Additional helper functions

// Map a game controller button to a FBNeo input
void Metal_MapControllerButton(int controllerId, int buttonId, int fbButtonId) {
    if (!g_inputState.initialized) {
        return;
    }
    
    NSString* key = [NSString stringWithFormat:@"%d_%d", controllerId, buttonId];
    NSNumber* value = [NSNumber numberWithInt:fbButtonId];
    
    [g_inputState.gamepadMap setObject:value forKey:key];
}

// Map a keyboard key to a FBNeo input
void Metal_MapKeyboardKey(int keyCode, int fbButtonId) {
    if (!g_inputState.initialized) {
        return;
    }
    
    NSString* key = [NSString stringWithFormat:@"%d", keyCode];
    NSNumber* value = [NSNumber numberWithInt:fbButtonId];
    
    [g_inputState.keyboardMap setObject:value forKey:key];
}

// Reset all input mappings to defaults
void Metal_ResetInputMappings() {
    if (!g_inputState.initialized) {
        return;
    }
    
    // Clear existing maps
    [g_inputState.keyboardMap removeAllObjects];
    [g_inputState.gamepadMap removeAllObjects];
    
    // Restore defaults
    [FBInputMap setupDefaultKeyboardMap:g_inputState.keyboardMap];
    [FBInputMap setupDefaultGamepadMap:g_inputState.gamepadMap];
} 