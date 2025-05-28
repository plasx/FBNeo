#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <Foundation/Foundation.h>
#import <GameController/GameController.h>
#import <IOKit/hid/IOHIDManager.h>
#import <Carbon/Carbon.h>
#include "metal_renderer_defines.h"

// Add integer types if not defined
#ifndef INT16
typedef int16_t INT16;
#endif
#ifndef INT32
typedef int32_t INT32;
#endif

// Add key code definitions
// Carbon virtual key codes
#ifndef kVK_ANSI_F1
#define kVK_ANSI_F1                       0x7A
#define kVK_ANSI_F2                       0x78
#define kVK_ANSI_F3                       0x63
#define kVK_ANSI_F4                       0x76
#define kVK_ANSI_F5                       0x60
#define kVK_ANSI_F6                       0x61
#define kVK_ANSI_F7                       0x62
#define kVK_ANSI_F8                       0x64
#define kVK_ANSI_F9                       0x65
#define kVK_ANSI_F10                      0x6D
#define kVK_ANSI_F11                      0x67
#define kVK_ANSI_F12                      0x6F
#endif

// External declarations to FBNeo sound system
extern "C" {
    extern INT16* pBurnSoundOut;      // Buffer for audio output
    extern INT32 nBurnSoundLen;       // Number of samples per frame
    extern INT32 nBurnSoundRate;      // Sample rate (usually 48000)
    
    // Burn sound functions - use only what's not already declared in metal_declarations.h
    int BurnSoundInit();
    void BurnSoundExit();
}

// External C functions for input handling
extern "C" {
    int Metal_InitInput();
    int Metal_ExitInput();
    int Metal_HandleKeyDown(int keyCode);
    int Metal_HandleKeyUp(int keyCode);
    int Metal_ResetInputState();
    int Metal_SetPlayerKeyMap(int player, int* keyMap, int keyMapSize);
    int Metal_SetDefaultKeyMaps();
    int Metal_GetKeyMapping(int* mapping, int maxMappings);
    int Metal_SetKeyMapping(int* mapping, int mappingSize);
    int Metal_SaveKeyMappingForGame(const char* gameName);
    int Metal_LoadKeyMappingForGame(const char* gameName);
    int Metal_SaveGlobalConfig();
    int Metal_LoadGlobalConfig();
    int Metal_GetShaderType();
    void Metal_ShowInputConfig(const char* gameName);
}

// Define FBNeo key codes - these match the codes expected by the emulator
typedef enum {
    FBNeo_KeyUp = 0x01,
    FBNeo_KeyDown = 0x02,
    FBNeo_KeyLeft = 0x03,
    FBNeo_KeyRight = 0x04,
    FBNeo_KeyButton1 = 0x05,
    FBNeo_KeyButton2 = 0x06,
    FBNeo_KeyButton3 = 0x07,
    FBNeo_KeyButton4 = 0x08,
    FBNeo_KeyButton5 = 0x09,
    FBNeo_KeyButton6 = 0x0A,
    FBNeo_KeyCoin = 0x0B,
    FBNeo_KeyStart = 0x0C,
    FBNeo_KeyService = 0x0D,
    FBNeo_KeyReset = 0x0E,
    FBNeo_KeyPause = 0x0F,
    FBNeo_KeyDiagnostic = 0x10,
    // Special keys for emulator control
    FBNeo_KeyMenu = 0x11,
    FBNeo_KeySaveState = 0x12,
    FBNeo_KeyLoadState = 0x13,
    FBNeo_KeyFastForward = 0x14,
    FBNeo_KeyFullscreen = 0x15,
    FBNeo_KeyScreenshot = 0x16,
    FBNeo_KeyQuit = 0x17
} FBNeoKeyCode;

// Define CPS2 input bit positions
#define CPS_INPUT_UP         0
#define CPS_INPUT_DOWN       1
#define CPS_INPUT_LEFT       2
#define CPS_INPUT_RIGHT      3
#define CPS_INPUT_BUTTON1    4
#define CPS_INPUT_BUTTON2    5
#define CPS_INPUT_BUTTON3    6
#define CPS_INPUT_BUTTON4    7
#define CPS_INPUT_BUTTON5    8
#define CPS_INPUT_BUTTON6    9
#define CPS_INPUT_START      0
#define CPS_INPUT_COIN       1

// Define key mappings
typedef struct {
    unsigned short keyCode;
    int playerNum;
    int inputBit;
    bool isDirectional;
} KeyMapping;

// Global input state
static bool g_inputInitialized = false;
static NSMutableArray<GCController *> *g_controllers = nil;

// Use a NSMutableDictionary for key mappings instead of the old KeyMapping array
static NSMutableDictionary *g_keyMapDict = nil;

// Initialize the key mappings dictionary
static void InitializeKeyMappings() {
    if (g_keyMapDict == nil) {
        g_keyMapDict = [[NSMutableDictionary alloc] init];
        
        // Player 1 mappings
        g_keyMapDict[@(kVK_UpArrow)] = @(FBNeo_KeyUp);
        g_keyMapDict[@(kVK_DownArrow)] = @(FBNeo_KeyDown);
        g_keyMapDict[@(kVK_LeftArrow)] = @(FBNeo_KeyLeft);
        g_keyMapDict[@(kVK_RightArrow)] = @(FBNeo_KeyRight);
        g_keyMapDict[@(kVK_ANSI_Z)] = @(FBNeo_KeyButton1);
        g_keyMapDict[@(kVK_ANSI_X)] = @(FBNeo_KeyButton2);
        g_keyMapDict[@(kVK_ANSI_C)] = @(FBNeo_KeyButton3);
        g_keyMapDict[@(kVK_ANSI_A)] = @(FBNeo_KeyButton4);
        g_keyMapDict[@(kVK_ANSI_S)] = @(FBNeo_KeyButton5);
        g_keyMapDict[@(kVK_ANSI_D)] = @(FBNeo_KeyButton6);
        g_keyMapDict[@(kVK_ANSI_1)] = @(FBNeo_KeyCoin);
        g_keyMapDict[@(kVK_ANSI_5)] = @(FBNeo_KeyStart);
        
        // Systems keys
        g_keyMapDict[@(kVK_Space)] = @(FBNeo_KeyPause);
        g_keyMapDict[@(kVK_ANSI_R)] = @(FBNeo_KeyReset);
        g_keyMapDict[@(kVK_ANSI_0)] = @(FBNeo_KeyService);
        g_keyMapDict[@(kVK_ANSI_F2)] = @(FBNeo_KeyDiagnostic);
        g_keyMapDict[@(kVK_ANSI_F3)] = @(FBNeo_KeySaveState);
        g_keyMapDict[@(kVK_ANSI_F4)] = @(FBNeo_KeyLoadState);
        g_keyMapDict[@(kVK_Tab)] = @(FBNeo_KeyFastForward);
        g_keyMapDict[@(kVK_ANSI_F12)] = @(FBNeo_KeyScreenshot);
        g_keyMapDict[@(kVK_Escape)] = @(FBNeo_KeyQuit);
        g_keyMapDict[@(kVK_ANSI_F1)] = @(FBNeo_KeyMenu);
        g_keyMapDict[@(kVK_Return)] = @(FBNeo_KeyFullscreen);
    }
}

// FBNeo Input Handler
@interface FBNeoInputHandler : NSObject

// Initialize input system
- (BOOL)initInput;

// Handle key events
- (void)handleKeyDown:(NSEvent *)event;
- (void)handleKeyUp:(NSEvent *)event;

// Reset input state
- (void)resetInputState;

@end

// Global input handler instance
static FBNeoInputHandler *g_inputHandler = nil;

@implementation FBNeoInputHandler

- (instancetype)init {
    self = [super init];
    if (self) {
        InitializeKeyMappings();
    }
    return self;
}

- (BOOL)initInput {
    NSLog(@"Initializing FBNeo input system");
    
    // Reset input state
    [self resetInputState];
    
    g_inputInitialized = YES;
    NSLog(@"Input system initialized");
    
    return YES;
}

- (void)handleKeyDown:(NSEvent *)event {
    NSInteger keyCode = event.keyCode;
    
    // Get the FBNeo key code from our mapping dictionary
    NSNumber *fbneoKeyCode = g_keyMapDict[@(keyCode)];
    
    // If key is mapped, process it
    if (fbneoKeyCode) {
        // Call the C function to handle the key press
        int result = Metal_HandleKeyDown([fbneoKeyCode intValue]);
        if (result != 0) {
            NSLog(@"Warning: Failed to process key down event: %ld", (long)keyCode);
        }
    }
}

- (void)handleKeyUp:(NSEvent *)event {
    NSInteger keyCode = event.keyCode;
    
    // Get the FBNeo key code from our mapping dictionary
    NSNumber *fbneoKeyCode = g_keyMapDict[@(keyCode)];
    
    // If key is mapped, process it
    if (fbneoKeyCode) {
        // Call the C function to handle the key release
        int result = Metal_HandleKeyUp([fbneoKeyCode intValue]);
        if (result != 0) {
            NSLog(@"Warning: Failed to process key up event: %ld", (long)keyCode);
        }
    }
}

- (void)resetInputState {
    // Call C function to reset input state
    Metal_ResetInputState();
}

@end

// C interface implementation
extern "C" {

// Initialize input system
int Metal_InitInput() {
    if (!g_inputHandler) {
        g_inputHandler = [[FBNeoInputHandler alloc] init];
    }
    
    if (![g_inputHandler initInput]) {
        NSLog(@"Failed to initialize input handler");
        return 1;
    }
    
    return 0;
}

// Clean up input system
int Metal_ExitInput() {
    g_inputHandler = nil;
    return 0;
}

// Handle key down event
int Metal_HandleKeyDown(int keyCode) {
    // This would interface with the FBNeo core to handle inputs
    // For now, we'll just log it
    NSLog(@"Metal_HandleKeyDown: %d", keyCode);
    
    // Here you would map to FBNeo input system functions
    // Examples:
    // - Start/stop a player input like up, down, button, etc
    // - Trigger system functions like reset, pause, etc
    
    return 0;
}

// Handle key up event
int Metal_HandleKeyUp(int keyCode) {
    // This would interface with the FBNeo core to handle inputs
    // For now, we'll just log it
    NSLog(@"Metal_HandleKeyUp: %d", keyCode);
    
    // Here you would map to FBNeo input system functions
    // to release a button or direction
    
    return 0;
}

// Reset input state
int Metal_ResetInputState() {
    // Reset all input state in the FBNeo core
    NSLog(@"Metal_ResetInputState");
    return 0;
}

} // extern "C"

// Categories to extend NSWindow and NSView for input handling
@interface NSWindow (FBNeoInputHandling)
@end

@implementation NSWindow (FBNeoInputHandling)

- (void)fbneo_sendKeyDown:(NSEvent *)event {
    if (g_inputHandler) {
        [g_inputHandler handleKeyDown:event];
    }
}

- (void)fbneo_sendKeyUp:(NSEvent *)event {
    if (g_inputHandler) {
        [g_inputHandler handleKeyUp:event];
    }
}

@end

// NSApplication category to add event monitoring
@interface NSApplication (FBNeoInputHandling)
- (void)setupFBNeoInputMonitoring;
@end

@implementation NSApplication (FBNeoInputHandling)

- (void)setupFBNeoInputMonitoring {
    // Initialize input handler if not already done
    if (!g_inputHandler) {
        Metal_InitInput();
    }
    
    // Set up global event monitors for key events
    [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskKeyDown handler:^NSEvent *(NSEvent *event) {
        // Only process key events when our app is active
        if ([NSApp isActive]) {
            if (g_inputHandler) {
                [g_inputHandler handleKeyDown:event];
            }
        }
        return event;
    }];
    
    [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskKeyUp handler:^NSEvent *(NSEvent *event) {
        // Only process key events when our app is active
        if ([NSApp isActive]) {
            if (g_inputHandler) {
                [g_inputHandler handleKeyUp:event];
            }
        }
        return event;
    }];
    
    NSLog(@"FBNeo input monitoring set up");
}

@end 