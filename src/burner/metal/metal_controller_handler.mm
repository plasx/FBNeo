#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <GameController/GameController.h>
#include "metal_renderer_defines.h"

// External C functions
extern "C" {
    int Metal_HandleKeyDown(int keyCode);
    int Metal_HandleKeyUp(int keyCode);
    int Metal_GetControllerCount();
    int Metal_InitControllers();
    int Metal_ExitControllers();
    const char* Metal_GetControllerName(int controllerIndex);
    int Metal_SetControllerMapping(int controllerIndex, int* mapping, int mappingSize);
    int Metal_SaveControllerMappings();
    int Metal_LoadControllerMappings();
    int Metal_SaveControllerMappingForGame(const char* gameName, int controllerIndex);
    int Metal_LoadControllerMappingForGame(const char* gameName, int controllerIndex);
}

// Controller handler that manages game controller input
@interface FBNeoControllerHandler : NSObject

// Properties
@property (nonatomic, strong) NSMutableArray *controllers;
@property (nonatomic, strong) NSMutableDictionary *controllerMappings;
@property (nonatomic, strong) NSMutableDictionary *controllerStates;
@property (nonatomic, assign) BOOL loggingEnabled;

// Initialize
- (id)init;

// Connect/disconnect handling
- (void)handleControllerConnected:(GCController *)controller;
- (void)handleControllerDisconnected:(GCController *)controller;

// Mapping controllers to player indexes
- (void)mapController:(GCController *)controller toPlayer:(int)playerIndex;
- (int)playerIndexForController:(GCController *)controller;

// Controller input state
- (void)updateControllerStates;
- (NSDictionary *)stateForController:(GCController *)controller;
- (NSDictionary *)stateForPlayerIndex:(int)playerIndex;
- (BOOL)isButtonPressedForPlayer:(int)playerIndex button:(int)buttonCode;

// Controller information
- (int)connectedControllerCount;
- (NSString *)nameForController:(GCController *)controller;
- (NSString *)nameForControllerAtIndex:(int)index;

// Configuration management
- (BOOL)loadControllerMappingForGame:(NSString *)gameName controllerIndex:(int)controllerIndex;
- (BOOL)saveControllerMappingForGame:(NSString *)gameName controllerIndex:(int)controllerIndex;
- (int)getControllerMapping:(int)controllerIndex mapping:(int*)mapping maxMappings:(int)maxMappings;
- (BOOL)setControllerMapping:(int)controllerIndex mapping:(int*)mapping mappingSize:(int)mappingSize;

@end

// Global controller handler instance
static FBNeoControllerHandler *g_controllerHandler = nil;

// Button code definitions for various controller types
typedef enum {
    CONTROLLER_BUTTON_A = 1,
    CONTROLLER_BUTTON_B = 2,
    CONTROLLER_BUTTON_X = 3,
    CONTROLLER_BUTTON_Y = 4,
    CONTROLLER_BUTTON_L1 = 5,
    CONTROLLER_BUTTON_R1 = 6,
    CONTROLLER_BUTTON_L2 = 7,
    CONTROLLER_BUTTON_R2 = 8,
    CONTROLLER_BUTTON_SELECT = 9,
    CONTROLLER_BUTTON_START = 10,
    CONTROLLER_BUTTON_L3 = 11,
    CONTROLLER_BUTTON_R3 = 12,
    CONTROLLER_DPAD_UP = 13,
    CONTROLLER_DPAD_DOWN = 14,
    CONTROLLER_DPAD_LEFT = 15,
    CONTROLLER_DPAD_RIGHT = 16,
    CONTROLLER_BUTTON_HOME = 17,
    
    // Analog inputs (treated as buttons with pressure values)
    CONTROLLER_LSTICK_UP = 20,
    CONTROLLER_LSTICK_DOWN = 21,
    CONTROLLER_LSTICK_LEFT = 22,
    CONTROLLER_LSTICK_RIGHT = 23,
    CONTROLLER_RSTICK_UP = 24,
    CONTROLLER_RSTICK_DOWN = 25,
    CONTROLLER_RSTICK_LEFT = 26,
    CONTROLLER_RSTICK_RIGHT = 27,
} ControllerButtonCode;

// Default mappings for controller buttons to FBNeo inputs
static NSDictionary *DefaultControllerMapping() {
    return @{
        @(CONTROLLER_BUTTON_A): @(FBNEO_KEY_BUTTON1),
        @(CONTROLLER_BUTTON_B): @(FBNEO_KEY_BUTTON2),
        @(CONTROLLER_BUTTON_X): @(FBNEO_KEY_BUTTON3),
        @(CONTROLLER_BUTTON_Y): @(FBNEO_KEY_BUTTON4),
        @(CONTROLLER_BUTTON_L1): @(FBNEO_KEY_BUTTON5),
        @(CONTROLLER_BUTTON_R1): @(FBNEO_KEY_BUTTON6),
        @(CONTROLLER_DPAD_UP): @(FBNEO_KEY_UP),
        @(CONTROLLER_DPAD_DOWN): @(FBNEO_KEY_DOWN),
        @(CONTROLLER_DPAD_LEFT): @(FBNEO_KEY_LEFT),
        @(CONTROLLER_DPAD_RIGHT): @(FBNEO_KEY_RIGHT),
        @(CONTROLLER_BUTTON_SELECT): @(FBNEO_KEY_COIN),
        @(CONTROLLER_BUTTON_START): @(FBNEO_KEY_START),
        @(CONTROLLER_BUTTON_HOME): @(FBNEO_KEY_MENU)
    };
}

@implementation FBNeoControllerHandler

- (id)init {
    self = [super init];
    if (self) {
        _controllers = [NSMutableArray array];
        _controllerMappings = [NSMutableDictionary dictionary];
        _controllerStates = [NSMutableDictionary dictionary];
        _loggingEnabled = NO;
        
        // Register for controller notifications
        [self setupControllerNotifications];
        
        // Start looking for controllers
        [self discoverControllers];
    }
    return self;
}

- (void)setupControllerNotifications {
    // Register for connect/disconnect notifications
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(controllerConnected:)
                                                 name:GCControllerDidConnectNotification
                                               object:nil];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(controllerDisconnected:)
                                                 name:GCControllerDidDisconnectNotification
                                               object:nil];
}

- (void)discoverControllers {
    // Start controller discovery
    [GCController startWirelessControllerDiscoveryWithCompletionHandler:^{
        NSLog(@"Controller discovery completed");
    }];
    
    // Get already connected controllers
    NSArray *controllers = [GCController controllers];
    for (GCController *controller in controllers) {
        [self handleControllerConnected:controller];
    }
}

#pragma mark - Controller Connect/Disconnect Handlers

- (void)controllerConnected:(NSNotification *)notification {
    GCController *controller = notification.object;
    [self handleControllerConnected:controller];
}

- (void)controllerDisconnected:(NSNotification *)notification {
    GCController *controller = notification.object;
    [self handleControllerDisconnected:controller];
}

- (void)handleControllerConnected:(GCController *)controller {
    if (![_controllers containsObject:controller]) {
        [_controllers addObject:controller];
        
        int playerIndex = (int)_controllers.count - 1;
        
        if (_loggingEnabled) {
            NSLog(@"Controller connected: %@, assigned to player %d", [self nameForController:controller], playerIndex);
        }
        
        // Set up default mapping for this controller
        _controllerMappings[@(playerIndex)] = [DefaultControllerMapping() mutableCopy];
        
        // Setup input value changed handler
        [self setupInputHandlersForController:controller];
        
        // Map to a player index
        [self mapController:controller toPlayer:playerIndex];
    }
}

- (void)handleControllerDisconnected:(GCController *)controller {
    int playerIndex = [self playerIndexForController:controller];
    
    if (_loggingEnabled && playerIndex >= 0) {
        NSLog(@"Controller disconnected: %@, was player %d", [self nameForController:controller], playerIndex);
    }
    
    // Remove the controller and its state
    [_controllers removeObject:controller];
    [_controllerStates removeObjectForKey:@(playerIndex)];
    
    // Remap remaining controllers to players
    for (int i = 0; i < _controllers.count; i++) {
        GCController *remainingController = _controllers[i];
        [self mapController:remainingController toPlayer:i];
    }
}

- (void)setupInputHandlersForController:(GCController *)controller {
    // Set up input handlers based on controller type
    if (controller.extendedGamepad) {
        [self setupExtendedControllerHandlers:controller];
    } else if (controller.gamepad) {
        [self setupGamepadControllerHandlers:controller];
    } else if (controller.microGamepad) {
        [self setupMicroGamepadHandlers:controller];
    }
}

- (void)setupExtendedControllerHandlers:(GCController *)controller {
    int playerIndex = [self playerIndexForController:controller];
    __weak typeof(self) weakSelf = self;
    
    // Extended gamepad setup (most common)
    controller.extendedGamepad.valueChangedHandler = ^(GCExtendedGamepad *gamepad, GCControllerElement *element) {
        // Create or update the controller state
        NSMutableDictionary *state = [weakSelf stateForController:controller] ? [weakSelf stateForController:controller] : [NSMutableDictionary dictionary];
        
        // Face buttons
        state[@(CONTROLLER_BUTTON_A)] = @(gamepad.buttonA.isPressed);
        state[@(CONTROLLER_BUTTON_B)] = @(gamepad.buttonB.isPressed);
        state[@(CONTROLLER_BUTTON_X)] = @(gamepad.buttonX.isPressed);
        state[@(CONTROLLER_BUTTON_Y)] = @(gamepad.buttonY.isPressed);
        
        // Shoulder buttons
        state[@(CONTROLLER_BUTTON_L1)] = @(gamepad.leftShoulder.isPressed);
        state[@(CONTROLLER_BUTTON_R1)] = @(gamepad.rightShoulder.isPressed);
        
        // Triggers
        state[@(CONTROLLER_BUTTON_L2)] = @(gamepad.leftTrigger.isPressed);
        state[@(CONTROLLER_BUTTON_R2)] = @(gamepad.rightTrigger.isPressed);
        
        // D-pad
        state[@(CONTROLLER_DPAD_UP)] = @(gamepad.dpad.up.isPressed);
        state[@(CONTROLLER_DPAD_DOWN)] = @(gamepad.dpad.down.isPressed);
        state[@(CONTROLLER_DPAD_LEFT)] = @(gamepad.dpad.left.isPressed);
        state[@(CONTROLLER_DPAD_RIGHT)] = @(gamepad.dpad.right.isPressed);
        
        // Thumbstick buttons
        state[@(CONTROLLER_BUTTON_L3)] = @(gamepad.leftThumbstickButton.isPressed);
        state[@(CONTROLLER_BUTTON_R3)] = @(gamepad.rightThumbstickButton.isPressed);
        
        // Menu buttons
        state[@(CONTROLLER_BUTTON_SELECT)] = @(gamepad.buttonOptions.isPressed);
        state[@(CONTROLLER_BUTTON_START)] = @(gamepad.buttonMenu.isPressed);
        
        // Left analog stick as digital inputs (with thresholds)
        float leftThreshold = 0.5f;
        state[@(CONTROLLER_LSTICK_UP)] = @(gamepad.leftThumbstick.yAxis.value > leftThreshold);
        state[@(CONTROLLER_LSTICK_DOWN)] = @(gamepad.leftThumbstick.yAxis.value < -leftThreshold);
        state[@(CONTROLLER_LSTICK_LEFT)] = @(gamepad.leftThumbstick.xAxis.value < -leftThreshold);
        state[@(CONTROLLER_LSTICK_RIGHT)] = @(gamepad.leftThumbstick.xAxis.value > leftThreshold);
        
        // Right analog stick as digital inputs
        float rightThreshold = 0.5f;
        state[@(CONTROLLER_RSTICK_UP)] = @(gamepad.rightThumbstick.yAxis.value > rightThreshold);
        state[@(CONTROLLER_RSTICK_DOWN)] = @(gamepad.rightThumbstick.yAxis.value < -rightThreshold);
        state[@(CONTROLLER_RSTICK_LEFT)] = @(gamepad.rightThumbstick.xAxis.value < -rightThreshold);
        state[@(CONTROLLER_RSTICK_RIGHT)] = @(gamepad.rightThumbstick.xAxis.value > rightThreshold);
        
        // Store state
        weakSelf.controllerStates[@(playerIndex)] = state;
    };
}

- (void)setupGamepadControllerHandlers:(GCController *)controller {
    int playerIndex = [self playerIndexForController:controller];
    __weak typeof(self) weakSelf = self;
    
    // Standard gamepad setup (fewer buttons)
    controller.gamepad.valueChangedHandler = ^(GCGamepad *gamepad, GCControllerElement *element) {
        // Create or update the controller state
        NSMutableDictionary *state = [weakSelf stateForController:controller] ? [weakSelf stateForController:controller] : [NSMutableDictionary dictionary];
        
        // Face buttons
        state[@(CONTROLLER_BUTTON_A)] = @(gamepad.buttonA.isPressed);
        state[@(CONTROLLER_BUTTON_B)] = @(gamepad.buttonB.isPressed);
        state[@(CONTROLLER_BUTTON_X)] = @(gamepad.buttonX.isPressed);
        state[@(CONTROLLER_BUTTON_Y)] = @(gamepad.buttonY.isPressed);
        
        // Shoulder buttons
        state[@(CONTROLLER_BUTTON_L1)] = @(gamepad.leftShoulder.isPressed);
        state[@(CONTROLLER_BUTTON_R1)] = @(gamepad.rightShoulder.isPressed);
        
        // D-pad
        state[@(CONTROLLER_DPAD_UP)] = @(gamepad.dpad.up.isPressed);
        state[@(CONTROLLER_DPAD_DOWN)] = @(gamepad.dpad.down.isPressed);
        state[@(CONTROLLER_DPAD_LEFT)] = @(gamepad.dpad.left.isPressed);
        state[@(CONTROLLER_DPAD_RIGHT)] = @(gamepad.dpad.right.isPressed);
        
        // Store state
        weakSelf.controllerStates[@(playerIndex)] = state;
    };
}

- (void)setupMicroGamepadHandlers:(GCController *)controller {
    int playerIndex = [self playerIndexForController:controller];
    __weak typeof(self) weakSelf = self;
    
    // Micro gamepad (Apple TV remote)
    controller.microGamepad.valueChangedHandler = ^(GCMicroGamepad *gamepad, GCControllerElement *element) {
        // Create or update the controller state
        NSMutableDictionary *state = [weakSelf stateForController:controller] ? [weakSelf stateForController:controller] : [NSMutableDictionary dictionary];
        
        // Limited buttons
        state[@(CONTROLLER_BUTTON_A)] = @(gamepad.buttonA.isPressed);
        state[@(CONTROLLER_BUTTON_X)] = @(gamepad.buttonX.isPressed);
        
        // D-pad simulation using the touchpad
        float dpadThreshold = 0.3f;
        state[@(CONTROLLER_DPAD_UP)] = @(gamepad.dpad.yAxis.value > dpadThreshold);
        state[@(CONTROLLER_DPAD_DOWN)] = @(gamepad.dpad.yAxis.value < -dpadThreshold);
        state[@(CONTROLLER_DPAD_LEFT)] = @(gamepad.dpad.xAxis.value < -dpadThreshold);
        state[@(CONTROLLER_DPAD_RIGHT)] = @(gamepad.dpad.xAxis.value > dpadThreshold);
        
        // Store state
        weakSelf.controllerStates[@(playerIndex)] = state;
    };
}

#pragma mark - Controller Mapping

- (void)mapController:(GCController *)controller toPlayer:(int)playerIndex {
    // Set the controller's player index
    controller.playerIndex = GCControllerPlayerIndex1 + playerIndex;
    
    if (_loggingEnabled) {
        NSLog(@"Mapped controller %@ to player %d", [self nameForController:controller], playerIndex);
    }
}

- (int)playerIndexForController:(GCController *)controller {
    NSUInteger index = [_controllers indexOfObject:controller];
    return (index == NSNotFound) ? -1 : (int)index;
}

#pragma mark - Controller State

- (void)updateControllerStates {
    // For controllers that don't automatically update their state
    for (GCController *controller in _controllers) {
        int playerIndex = [self playerIndexForController:controller];
        if (![_controllerStates objectForKey:@(playerIndex)]) {
            _controllerStates[@(playerIndex)] = [NSMutableDictionary dictionary];
        }
    }
}

- (NSDictionary *)stateForController:(GCController *)controller {
    int playerIndex = [self playerIndexForController:controller];
    return [self stateForPlayerIndex:playerIndex];
}

- (NSDictionary *)stateForPlayerIndex:(int)playerIndex {
    return _controllerStates[@(playerIndex)];
}

- (BOOL)isButtonPressedForPlayer:(int)playerIndex button:(int)buttonCode {
    NSDictionary *state = [self stateForPlayerIndex:playerIndex];
    if (!state) {
        return NO;
    }
    
    NSNumber *isPressed = state[@(buttonCode)];
    return [isPressed boolValue];
}

#pragma mark - Controller Information

- (int)connectedControllerCount {
    return (int)_controllers.count;
}

- (NSString *)nameForController:(GCController *)controller {
    if (!controller) {
        return @"Unknown Controller";
    }
    
    // Use the vendor-supplied name if available
    if (controller.vendorName) {
        return controller.vendorName;
    }
    
    // Otherwise, determine type
    if (controller.extendedGamepad) {
        return @"Extended Gamepad";
    } else if (controller.gamepad) {
        return @"Standard Gamepad";
    } else if (controller.microGamepad) {
        return @"Micro Gamepad";
    }
    
    return [NSString stringWithFormat:@"Controller %d", [self playerIndexForController:controller] + 1];
}

- (NSString *)nameForControllerAtIndex:(int)index {
    if (index < 0 || index >= _controllers.count) {
        return @"No Controller";
    }
    
    GCController *controller = _controllers[index];
    return [self nameForController:controller];
}

#pragma mark - Configuration Management

- (BOOL)loadControllerMappingForGame:(NSString *)gameName controllerIndex:(int)controllerIndex {
    if (!gameName || gameName.length == 0 || controllerIndex < 0 || controllerIndex >= _controllers.count) {
        return NO;
    }
    
    // Get the game-specific key
    NSString *gameKey = [NSString stringWithFormat:@"FBNeoControllerMappings_%@_%d", gameName, controllerIndex];
    
    // Get all game mappings
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    NSDictionary *gameMappings = [defaults objectForKey:@"FBNeoGameControllerMappings"];
    
    if (!gameMappings) {
        NSLog(@"No game controller mappings found");
        return NO;
    }
    
    // Get mapping for this game and controller
    NSDictionary *serializedMappings = gameMappings[gameKey];
    
    if (!serializedMappings) {
        NSLog(@"No controller mappings found for game: %@ controller: %d", gameName, controllerIndex);
        return NO;
    }
    
    // Load the mappings
    NSMutableDictionary *mapping = [NSMutableDictionary dictionary];
    
    for (NSString *controllerKeyStr in serializedMappings) {
        NSString *fbKeyStr = serializedMappings[controllerKeyStr];
        
        int controllerKey = [controllerKeyStr intValue];
        int fbKey = [fbKeyStr intValue];
        
        mapping[@(controllerKey)] = @(fbKey);
    }
    
    // Store in our mappings
    _controllerMappings[@(controllerIndex)] = mapping;
    
    if (_loggingEnabled) {
        NSLog(@"Loaded controller mappings for game: %@ controller: %d", gameName, controllerIndex);
    }
    
    return YES;
}

- (BOOL)saveControllerMappingForGame:(NSString *)gameName controllerIndex:(int)controllerIndex {
    if (!gameName || gameName.length == 0 || controllerIndex < 0 || controllerIndex >= _controllers.count) {
        return NO;
    }
    
    // Get the mapping for this controller
    NSDictionary *mapping = _controllerMappings[@(controllerIndex)];
    if (!mapping) {
        return NO;
    }
    
    // Create a serializable version of the mappings
    NSMutableDictionary *serializedMappings = [NSMutableDictionary dictionary];
    
    for (NSNumber *controllerKey in mapping) {
        NSNumber *fbKey = mapping[controllerKey];
        serializedMappings[[controllerKey stringValue]] = [fbKey stringValue];
    }
    
    // Save to user defaults with game-specific key
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    NSString *gameKey = [NSString stringWithFormat:@"FBNeoControllerMappings_%@_%d", gameName, controllerIndex];
    
    // Get existing game mappings if any
    NSMutableDictionary *gameMappings = [[defaults objectForKey:@"FBNeoGameControllerMappings"] mutableCopy];
    if (!gameMappings) {
        gameMappings = [NSMutableDictionary dictionary];
    }
    
    // Update mapping for this game
    gameMappings[gameKey] = serializedMappings;
    
    // Save back to user defaults
    [defaults setObject:gameMappings forKey:@"FBNeoGameControllerMappings"];
    [defaults synchronize];
    
    if (_loggingEnabled) {
        NSLog(@"Saved controller mappings for game: %@ controller: %d", gameName, controllerIndex);
    }
    
    return YES;
}

- (int)getControllerMapping:(int)controllerIndex mapping:(int*)mapping maxMappings:(int)maxMappings {
    if (!mapping || maxMappings <= 0 || controllerIndex < 0 || controllerIndex >= _controllers.count) {
        return 0;
    }
    
    // Get the mapping for this controller
    NSDictionary *controllerMapping = _controllerMappings[@(controllerIndex)];
    if (!controllerMapping) {
        return 0;
    }
    
    // Count how many mappings we can provide
    int totalMappings = (int)controllerMapping.count * 2; // Each mapping is controllerKey -> fbKey
    int providedMappings = MIN(totalMappings, maxMappings);
    int pairCount = providedMappings / 2;
    
    // Convert mappings to array
    int index = 0;
    for (NSNumber *controllerKey in controllerMapping) {
        if (index >= pairCount) {
            break;
        }
        
        NSNumber *fbKey = controllerMapping[controllerKey];
        
        mapping[index*2] = [fbKey intValue];
        mapping[index*2+1] = [controllerKey intValue];
        
        index++;
    }
    
    return providedMappings;
}

- (BOOL)setControllerMapping:(int)controllerIndex mapping:(int*)mapping mappingSize:(int)mappingSize {
    if (!mapping || mappingSize <= 0 || controllerIndex < 0 || controllerIndex >= _controllers.count) {
        return NO;
    }
    
    // Create a new mapping
    NSMutableDictionary *controllerMapping = [NSMutableDictionary dictionary];
    
    // Add new mappings
    for (int i = 0; i < mappingSize; i += 2) {
        if (i + 1 < mappingSize) {
            int fbKey = mapping[i];
            int controllerKey = mapping[i+1];
            
            controllerMapping[@(controllerKey)] = @(fbKey);
        }
    }
    
    // Store the new mapping
    _controllerMappings[@(controllerIndex)] = controllerMapping;
    
    return YES;
}

- (NSDictionary *)getFBNeoInputStateForPlayer:(int)playerIndex {
    if (playerIndex < 0 || playerIndex >= _controllers.count) {
        return nil;
    }
    
    // Get controller state and mapping
    NSDictionary *state = [self stateForPlayerIndex:playerIndex];
    NSDictionary *mapping = _controllerMappings[@(playerIndex)];
    
    if (!state || !mapping) {
        return nil;
    }
    
    // Convert controller state to FBNeo input state
    NSMutableDictionary *inputState = [NSMutableDictionary dictionary];
    
    for (NSNumber *controllerKey in mapping) {
        NSNumber *fbKey = mapping[controllerKey];
        NSNumber *isPressed = state[controllerKey];
        
        if (isPressed && [isPressed boolValue]) {
            inputState[fbKey] = @YES;
        }
    }
    
    return inputState;
}

- (void)dealloc {
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    [GCController stopWirelessControllerDiscovery];
}

@end

#pragma mark - C Interface

extern "C" {
    
// Initialize controller support
int Metal_InitControllerSupport() {
    @autoreleasepool {
        if (!g_controllerHandler) {
            g_controllerHandler = [[FBNeoControllerHandler alloc] init];
        }
        return (g_controllerHandler != nil) ? 0 : 1;
    }
}

// Cleanup controller support
void Metal_ExitControllerSupport() {
    @autoreleasepool {
        g_controllerHandler = nil;
    }
}

// Get number of connected controllers
int Metal_GetControllerCount() {
    @autoreleasepool {
        if (!g_controllerHandler) {
            return 0;
        }
        return [g_controllerHandler connectedControllerCount];
    }
}

// Get controller name
const char* Metal_GetControllerName(int controllerIndex) {
    @autoreleasepool {
        if (!g_controllerHandler) {
            return "";
        }
        
        NSString *name = [g_controllerHandler nameForControllerAtIndex:controllerIndex];
        return strdup([name UTF8String]);
    }
}

// Check if a controller button is pressed
int Metal_IsControllerButtonPressed(int playerIndex, int buttonCode) {
    @autoreleasepool {
        if (!g_controllerHandler) {
            return 0;
        }
        
        BOOL isPressed = [g_controllerHandler isButtonPressedForPlayer:playerIndex button:buttonCode];
        return isPressed ? 1 : 0;
    }
}

// Get FBNeo input state for player
void Metal_GetFBNeoInputStateForPlayer(int playerIndex, unsigned char* output, int outputSize) {
    @autoreleasepool {
        if (!g_controllerHandler || !output || outputSize <= 0) {
            return;
        }
        
        // Get input state for player
        NSDictionary *inputState = [g_controllerHandler getFBNeoInputStateForPlayer:playerIndex];
        
        // Clear output buffer
        memset(output, 0, outputSize);
        
        // Fill output buffer
        for (NSNumber *fbKey in inputState) {
            int key = [fbKey intValue];
            if (key >= 0 && key < outputSize) {
                output[key] = 1;
            }
        }
    }
}

// Save controller mappings for game
int Metal_SaveControllerMappingForGame(const char* gameName, int controllerIndex) {
    @autoreleasepool {
        if (!g_controllerHandler || !gameName) {
            return 1;
        }
        
        NSString *gameNameStr = [NSString stringWithUTF8String:gameName];
        BOOL success = [g_controllerHandler saveControllerMappingForGame:gameNameStr controllerIndex:controllerIndex];
        return success ? 0 : 1;
    }
}

// Load controller mappings for game
int Metal_LoadControllerMappingForGame(const char* gameName, int controllerIndex) {
    @autoreleasepool {
        if (!g_controllerHandler || !gameName) {
            return 1;
        }
        
        NSString *gameNameStr = [NSString stringWithUTF8String:gameName];
        BOOL success = [g_controllerHandler loadControllerMappingForGame:gameNameStr controllerIndex:controllerIndex];
        return success ? 0 : 1;
    }
}

// Get controller mapping
int Metal_GetControllerMapping(int controllerIndex, int* mapping, int maxMappings) {
    @autoreleasepool {
        if (!g_controllerHandler || !mapping || maxMappings <= 0) {
            return 0;
        }
        
        return [g_controllerHandler getControllerMapping:controllerIndex mapping:mapping maxMappings:maxMappings];
    }
}

// Set controller mapping
int Metal_SetControllerMapping(int controllerIndex, int* mapping, int mappingSize) {
    @autoreleasepool {
        if (!g_controllerHandler || !mapping || mappingSize <= 0) {
            return 1;
        }
        
        BOOL success = [g_controllerHandler setControllerMapping:controllerIndex mapping:mapping mappingSize:mappingSize];
        return success ? 0 : 1;
    }
}

} // extern "C"
} // extern "C" 