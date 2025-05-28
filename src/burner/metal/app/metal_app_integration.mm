#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>

#include "metal_keyboard_events.h"
#include "../input/metal_input.h"

// Main integration point to connect Metal app with input system
@interface MetalAppInputIntegration : NSObject
+ (void)setupInputForWindow:(NSWindow *)window withView:(MTKView *)metalView;
+ (void)setupGameControllers;
+ (void)cleanupInput;
@end

@implementation MetalAppInputIntegration {
    void* keyboardViewPtr;
    NSView* keyboardView;
}

static MetalAppInputIntegration* sharedInstance = nil;

+ (instancetype)sharedInstance {
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        sharedInstance = [[self alloc] init];
    });
    return sharedInstance;
}

- (instancetype)init {
    if (self = [super init]) {
        keyboardViewPtr = NULL;
        keyboardView = nil;
    }
    return self;
}

+ (void)setupInputForWindow:(NSWindow *)window withView:(MTKView *)metalView {
    // Initialize the Metal input system
    Metal_InitInput();
    
    // Create a keyboard view that will capture keyboard events
    MetalAppInputIntegration* instance = [MetalAppInputIntegration sharedInstance];
    
    // Create our keyboard view and insert it
    NSRect frame = metalView.frame;
    instance->keyboardViewPtr = MetalApp_CreateKeyboardView(frame);
    instance->keyboardView = (__bridge NSView*)instance->keyboardViewPtr;
    
    // Add it as a subview to the metal view
    [metalView addSubview:instance->keyboardView];
    
    // Make sure it fills the parent view
    instance->keyboardView.frame = metalView.bounds;
    instance->keyboardView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    
    // Make it first responder to get keyboard events
    [window makeFirstResponder:instance->keyboardView];
}

+ (void)setupGameControllers {
    // The Metal_InitInput call in setupInputForWindow already sets up controllers
}

+ (void)cleanupInput {
    MetalAppInputIntegration* instance = [MetalAppInputIntegration sharedInstance];
    
    // Clean up keyboard view
    if (instance->keyboardViewPtr) {
        [instance->keyboardView removeFromSuperview];
        MetalApp_ReleaseKeyboardView(instance->keyboardViewPtr);
        instance->keyboardViewPtr = NULL;
        instance->keyboardView = nil;
    }
    
    // Shut down Metal input system
    Metal_ShutdownInput();
}

@end

// C-compatible interface for the rest of the codebase
extern "C" {
    void MetalApp_SetupInputForWindowAndView(void* windowPtr, void* viewPtr) {
        NSWindow* window = (__bridge NSWindow*)windowPtr;
        MTKView* metalView = (__bridge MTKView*)viewPtr;
        [MetalAppInputIntegration setupInputForWindow:window withView:metalView];
    }
    
    void MetalApp_CleanupInput() {
        [MetalAppInputIntegration cleanupInput];
    }
} 