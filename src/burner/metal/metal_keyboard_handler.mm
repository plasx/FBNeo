#import <Cocoa/Cocoa.h>
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>

// Include interface for C functions
#include "metal_input.h"

// Keyboard view for capturing events
@interface FBNeoKeyboardView : NSView {
    BOOL _keysDown[256];  // State of each key
}

@property (nonatomic, strong) NSMutableArray *keyEvents;

- (void)processKeyEvent:(NSEvent *)event isKeyDown:(BOOL)isDown;
- (BOOL)isKeyDown:(unsigned short)keyCode;

@end

// Implementation of keyboard view
@implementation FBNeoKeyboardView

- (instancetype)initWithFrame:(NSRect)frameRect {
    self = [super initWithFrame:frameRect];
    if (self) {
        // Initialize key state array
        memset(_keysDown, 0, sizeof(_keysDown));
        
        // Create array to store key events
        _keyEvents = [NSMutableArray array];
        
        // Make the view focusable
        self.canBecomeKeyView = YES;
    }
    return self;
}

// Make view accept keyboard events
- (BOOL)acceptsFirstResponder {
    return YES;
}

// Key down handler
- (void)keyDown:(NSEvent *)event {
    if ([event isARepeat]) {
        return;  // Ignore key repeats
    }
    
    [self processKeyEvent:event isKeyDown:YES];
}

// Key up handler
- (void)keyUp:(NSEvent *)event {
    [self processKeyEvent:event isKeyDown:NO];
}

// Process a key event
- (void)processKeyEvent:(NSEvent *)event isKeyDown:(BOOL)isDown {
    unsigned short keyCode = [event keyCode];
    
    // Bounds check
    if (keyCode < 256) {
        // Update key state
        _keysDown[keyCode] = isDown;
        
        // Call C interface
        if (isDown) {
            Metal_HandleKeyDown(keyCode);
        } else {
            Metal_HandleKeyUp(keyCode);
        }
    }
    
    // Add to events array for processing
    [_keyEvents addObject:event];
}

// Check if a key is down
- (BOOL)isKeyDown:(unsigned short)keyCode {
    if (keyCode < 256) {
        return _keysDown[keyCode];
    }
    return NO;
}

// Handle flags changed (modifier keys)
- (void)flagsChanged:(NSEvent *)event {
    NSEventModifierFlags flags = [event modifierFlags];
    
    // Handle shift keys
    if (flags & NSEventModifierFlagShift) {
        _keysDown[kVK_Shift] = YES;
        Metal_HandleKeyDown(kVK_Shift);
    } else {
        _keysDown[kVK_Shift] = NO;
        Metal_HandleKeyUp(kVK_Shift);
    }
    
    // Handle control keys
    if (flags & NSEventModifierFlagControl) {
        _keysDown[kVK_Control] = YES;
        Metal_HandleKeyDown(kVK_Control);
    } else {
        _keysDown[kVK_Control] = NO;
        Metal_HandleKeyUp(kVK_Control);
    }
    
    // Handle alt/option keys
    if (flags & NSEventModifierFlagOption) {
        _keysDown[kVK_Option] = YES;
        Metal_HandleKeyDown(kVK_Option);
    } else {
        _keysDown[kVK_Option] = NO;
        Metal_HandleKeyUp(kVK_Option);
    }
    
    // Handle command keys
    if (flags & NSEventModifierFlagCommand) {
        _keysDown[kVK_Command] = YES;
        Metal_HandleKeyDown(kVK_Command);
    } else {
        _keysDown[kVK_Command] = NO;
        Metal_HandleKeyUp(kVK_Command);
    }
}

@end

// C compatible interface
extern "C" {

// Create a keyboard view
void* Metal_CreateKeyboardView(NSRect frame) {
    FBNeoKeyboardView* view = [[FBNeoKeyboardView alloc] initWithFrame:frame];
    return (__bridge_retained void*)view;
}

// Release a keyboard view
void Metal_ReleaseKeyboardView(void* viewPtr) {
    if (viewPtr) {
        FBNeoKeyboardView* view = (__bridge_transfer FBNeoKeyboardView*)viewPtr;
        view = nil;
    }
}

// Check if a key is down
int Metal_IsKeyDown(unsigned short keyCode) {
    @autoreleasepool {
        NSArray<NSWindow*>* windows = [NSApp windows];
        for (NSWindow* window in windows) {
            NSView* firstResponder = (NSView*)[window firstResponder];
            if ([firstResponder isKindOfClass:[FBNeoKeyboardView class]]) {
                FBNeoKeyboardView* keyboardView = (FBNeoKeyboardView*)firstResponder;
                return [keyboardView isKeyDown:keyCode] ? 1 : 0;
            }
        }
    }
    return 0;
}

// Default key mapping table
static const int defaultKeyMap[] = {
    // FBNeo Key          macOS Key Code
    FBK_UP,              kVK_UpArrow,       // Up arrow
    FBK_DOWN,            kVK_DownArrow,     // Down arrow
    FBK_LEFT,            kVK_LeftArrow,     // Left arrow
    FBK_RIGHT,           kVK_RightArrow,    // Right arrow
    FBK_ENTER,           kVK_Return,        // Return
    FBK_SPACE,           kVK_Space,         // Space
    FBK_A,               kVK_ANSI_A,        // A
    FBK_B,               kVK_ANSI_B,        // B
    FBK_C,               kVK_ANSI_C,        // C
    FBK_D,               kVK_ANSI_D,        // D
    FBK_E,               kVK_ANSI_E,        // E
    FBK_F,               kVK_ANSI_F,        // F
    FBK_G,               kVK_ANSI_G,        // G
    FBK_H,               kVK_ANSI_H,        // H
    FBK_I,               kVK_ANSI_I,        // I
    FBK_J,               kVK_ANSI_J,        // J
    FBK_K,               kVK_ANSI_K,        // K
    FBK_L,               kVK_ANSI_L,        // L
    FBK_M,               kVK_ANSI_M,        // M
    FBK_N,               kVK_ANSI_N,        // N
    FBK_O,               kVK_ANSI_O,        // O
    FBK_P,               kVK_ANSI_P,        // P
    FBK_Q,               kVK_ANSI_Q,        // Q
    FBK_R,               kVK_ANSI_R,        // R
    FBK_S,               kVK_ANSI_S,        // S
    FBK_T,               kVK_ANSI_T,        // T
    FBK_U,               kVK_ANSI_U,        // U
    FBK_V,               kVK_ANSI_V,        // V
    FBK_W,               kVK_ANSI_W,        // W
    FBK_X,               kVK_ANSI_X,        // X
    FBK_Y,               kVK_ANSI_Y,        // Y
    FBK_Z,               kVK_ANSI_Z,        // Z
    FBK_0,               kVK_ANSI_0,        // 0
    FBK_1,               kVK_ANSI_1,        // 1
    FBK_2,               kVK_ANSI_2,        // 2
    FBK_3,               kVK_ANSI_3,        // 3
    FBK_4,               kVK_ANSI_4,        // 4
    FBK_5,               kVK_ANSI_5,        // 5
    FBK_6,               kVK_ANSI_6,        // 6
    FBK_7,               kVK_ANSI_7,        // 7
    FBK_8,               kVK_ANSI_8,        // 8
    FBK_9,               kVK_ANSI_9,        // 9
    FBK_ESC,             kVK_Escape,        // Escape
    FBK_TAB,             kVK_Tab,           // Tab
    FBK_F1,              kVK_F1,            // F1
    FBK_F2,              kVK_F2,            // F2
    FBK_F3,              kVK_F3,            // F3
    FBK_F4,              kVK_F4,            // F4
    FBK_F5,              kVK_F5,            // F5
    FBK_F6,              kVK_F6,            // F6
    FBK_F7,              kVK_F7,            // F7
    FBK_F8,              kVK_F8,            // F8
    FBK_F9,              kVK_F9,            // F9
    FBK_F10,             kVK_F10,           // F10
    FBK_F11,             kVK_F11,           // F11
    FBK_F12,             kVK_F12,           // F12
};

// Handle key down
int Metal_HandleKeyDown(int keyCode) {
    // Map macOS key code to FBNeo key
    for (unsigned int i = 0; i < sizeof(defaultKeyMap) / sizeof(defaultKeyMap[0]); i += 2) {
        if (defaultKeyMap[i + 1] == keyCode) {
            // Found a mapping, send to FBNeo
            return Metal_InputKeyDown(defaultKeyMap[i]);
        }
    }
    
    return 0;
}

// Handle key up
int Metal_HandleKeyUp(int keyCode) {
    // Map macOS key code to FBNeo key
    for (unsigned int i = 0; i < sizeof(defaultKeyMap) / sizeof(defaultKeyMap[0]); i += 2) {
        if (defaultKeyMap[i + 1] == keyCode) {
            // Found a mapping, send to FBNeo
            return Metal_InputKeyUp(defaultKeyMap[i]);
        }
    }
    
    return 0;
}

// Initialize keyboard input
int Metal_InitKeyboardInput() {
    return 0;
}

// Shutdown keyboard input
int Metal_ExitKeyboardInput() {
    return 0;
}

} // extern "C" 