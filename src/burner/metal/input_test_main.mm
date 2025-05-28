#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>
#include <stdio.h>
#include "metal_input_stubs.h"
#include "metal_input_defs.h"

// Stub implementations
extern "C" INT32 Metal_SaveState(INT32 nSlot) {
    printf("[Metal_SaveState] Saving state to slot %d\n", nSlot);
    return 0;
}

extern "C" INT32 Metal_LoadState(INT32 nSlot) {
    printf("[Metal_LoadState] Loading state from slot %d\n", nSlot);
    return 0;
}

void Metal_RequestQuit() {
    printf("[Metal_RequestQuit] Requesting application to quit\n");
    
    // Schedule an application quit on the main thread
    dispatch_async(dispatch_get_main_queue(), ^{
        [NSApp terminate:nil];
    });
}

// Test window controller
@interface InputTestWindowController : NSWindowController
@property (nonatomic, strong) NSWindow *window;
@property (nonatomic, strong) NSTextView *textView;
@end

@implementation InputTestWindowController

- (instancetype)init {
    self = [super init];
    if (self) {
        // Create a window
        NSRect frame = NSMakeRect(100, 100, 400, 300);
        NSWindow *window = [[NSWindow alloc] initWithContentRect:frame
                                                       styleMask:NSWindowStyleMaskTitled |
                                                                 NSWindowStyleMaskClosable |
                                                                 NSWindowStyleMaskMiniaturizable |
                                                                 NSWindowStyleMaskResizable
                                                         backing:NSBackingStoreBuffered
                                                           defer:NO];
        window.title = @"FBNeo Input Test";
        [window center];
        self.window = window;
        
        // Create a text view
        NSTextView *textView = [[NSTextView alloc] initWithFrame:frame];
        textView.editable = NO;
        textView.string = @"Press keys to test input handling:\n\n"
                           "- Arrow keys = Player 1 directional controls\n"
                           "- Z,X,C,V,B,N = Player 1 buttons\n"
                           "- Enter = Player 1 Start\n"
                           "- Space = Player 1 Coin\n\n"
                           "- WASD = Player 2 directional controls\n"
                           "- Q,W,E,R,T,Y = Player 2 buttons\n"
                           "- Tab = Player 2 Start\n"
                           "- Caps Lock = Player 2 Coin\n\n"
                           "- F5 = Save State\n"
                           "- F8 = Load State\n"
                           "- Esc = Quit\n"
                           "- F1 = Toggle Debug Overlay\n";
        
        // Make the text view the first responder
        [window.contentView addSubview:textView];
        [window makeFirstResponder:textView];
        self.textView = textView;
        
        // Handle key events
        [textView setNextResponder:self];
    }
    return self;
}

- (void)keyDown:(NSEvent *)event {
    unsigned short keyCode = event.keyCode;
    printf("[keyDown] Key pressed: 0x%02X\n", keyCode);
    Metal_HandleKeyDown(keyCode);
}

- (void)keyUp:(NSEvent *)event {
    unsigned short keyCode = event.keyCode;
    printf("[keyUp] Key released: 0x%02X\n", keyCode);
    Metal_HandleKeyUp(keyCode);
}

@end

// Add stubs for required functions
extern "C" INT32 BurnDrvReset() {
    printf("[BurnDrvReset] Resetting emulation\n");
    return 0;
}

extern "C" INT32 BurnDrvSetInput(INT32 i, INT32 nState) {
    static const char* inputNames[] = {
        "P1 Up", "P1 Down", "P1 Left", "P1 Right",
        "P1 Weak Punch", "P1 Medium Punch", "P1 Strong Punch",
        "P1 Weak Kick", "P1 Medium Kick", "P1 Strong Kick",
        "P1 Start", "P1 Coin",
        "P2 Up", "P2 Down", "P2 Left", "P2 Right",
        "P2 Weak Punch", "P2 Medium Punch", "P2 Strong Punch",
        "P2 Weak Kick", "P2 Medium Kick", "P2 Strong Kick",
        "P2 Start", "P2 Coin",
        "Reset", "Diagnostic", "Service"
    };
    
    const char* inputName = (i >= 0 && i < (int)(sizeof(inputNames)/sizeof(inputNames[0]))) 
        ? inputNames[i] 
        : "Unknown";
    
    if (nState) {
        printf("[BurnDrvSetInput] Input %d (%s) PRESSED\n", i, inputName);
    } else {
        printf("[BurnDrvSetInput] Input %d (%s) RELEASED\n", i, inputName);
    }
    
    return 0;
}

// Main function
int main(int argc, const char * argv[]) {
    @autoreleasepool {
        printf("FBNeo Input System Test\n");
        
        // Initialize the input system
        Metal_InitInput();
        
        // Create a basic application
        [NSApplication sharedApplication];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
        
        // Create a window
        InputTestWindowController *controller = [[InputTestWindowController alloc] init];
        [controller.window makeKeyAndOrderFront:nil];
        
        // Activate the application
        [NSApp activateIgnoringOtherApps:YES];
        
        // Run the application
        [NSApp run];
        
        // Cleanup
        Metal_ExitInput();
        
        return 0;
    }
} 