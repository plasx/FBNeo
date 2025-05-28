#import <Cocoa/Cocoa.h>

extern "C" {
    #include "../input/metal_input.h"
}

@interface MetalKeyboardHandler : NSObject
+ (void)initialize;
+ (int)handleKeyDown:(NSEvent *)event;
+ (int)handleKeyUp:(NSEvent *)event;
+ (int)mapNSEventKeyToCode:(NSEvent *)event;
@end

@implementation MetalKeyboardHandler

+ (void)initialize {
    // Nothing to initialize
}

// Map NSEvent key code to our internal key code
+ (int)mapNSEventKeyToCode:(NSEvent *)event {
    // Return the raw key code for now - macOS virtual key codes are compatible with our input system
    return (int)event.keyCode;
}

// Handle key down events
+ (int)handleKeyDown:(NSEvent *)event {
    int keyCode = [MetalKeyboardHandler mapNSEventKeyToCode:event];
    return Metal_HandleKeyDown(keyCode);
}

// Handle key up events
+ (int)handleKeyUp:(NSEvent *)event {
    int keyCode = [MetalKeyboardHandler mapNSEventKeyToCode:event];
    return Metal_HandleKeyUp(keyCode);
}

@end

// Export C functions for bridging to other parts of the app
extern "C" {
    int MetalApp_HandleKeyDown(NSEvent *event) {
        return [MetalKeyboardHandler handleKeyDown:event];
    }
    
    int MetalApp_HandleKeyUp(NSEvent *event) {
        return [MetalKeyboardHandler handleKeyUp:event];
    }
}

#pragma mark - Keyboard View Implementation

// Special view class that can become first responder to receive key events
@interface MetalKeyboardView : NSView
@end

@implementation MetalKeyboardView

- (BOOL)acceptsFirstResponder {
    return YES;
}

- (void)keyDown:(NSEvent *)event {
    // Handle key down event
    if ([MetalKeyboardHandler handleKeyDown:event] != 0) {
        // If our handler didn't process it, pass it up the responder chain
        [super keyDown:event];
    }
}

- (void)keyUp:(NSEvent *)event {
    // Handle key up event
    if ([MetalKeyboardHandler handleKeyUp:event] != 0) {
        // If our handler didn't process it, pass it up the responder chain
        [super keyUp:event];
    }
}

- (BOOL)performKeyEquivalent:(NSEvent *)event {
    // Handle command-key combinations, return YES if handled
    if (event.modifierFlags & NSEventModifierFlagCommand) {
        // Let the app handle standard command keys (command+q, etc)
        return NO;
    }
    
    // For other keys, handle normally
    if (event.type == NSEventTypeKeyDown) {
        [self keyDown:event];
        return YES;
    } else if (event.type == NSEventTypeKeyUp) {
        [self keyUp:event];
        return YES;
    }
    
    return NO;
}

@end

// Export a function to create a keyboard view
extern "C" {
    void* MetalApp_CreateKeyboardView(NSRect frame) {
        MetalKeyboardView* view = [[MetalKeyboardView alloc] initWithFrame:frame];
        return (__bridge_retained void*)view;
    }
    
    void MetalApp_ReleaseKeyboardView(void* viewPtr) {
        if (viewPtr) {
            MetalKeyboardView* view = (__bridge_transfer MetalKeyboardView*)viewPtr;
            view = nil; // This will release it
        }
    }
} 