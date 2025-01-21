// src/burner/metal/metal_input.mm
#import <Cocoa/Cocoa.h>

extern "C" {
  #include "burnint.h"
}

bool gKeyboardState[256];

@implementation MetalInputView : NSView

- (BOOL)acceptsFirstResponder {
    return YES;
}

// KeyDown
- (void)keyDown:(NSEvent *)event {
    unsigned short code = [event keyCode];
    // map code => FBNeo
    // e.g. gKeyboardState[code] = true;
}

// KeyUp
- (void)keyUp:(NSEvent *)event {
    unsigned short code = [event keyCode];
    // e.g. gKeyboardState[code] = false;
}

@end