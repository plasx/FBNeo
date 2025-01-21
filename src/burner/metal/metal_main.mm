// src/burner/metal/metal_main.mm

#import <Cocoa/Cocoa.h>
#import "metal_window.h"

// If you need FBNeo headers, include them as C:
extern "C" {
   #include "burnint.h"   // for BurnDrvInit, BurnDrvFrame, etc.
   // #include "burn.h" or anything else if needed
}

// Optional global pointer:
NSApplication* gApplication = nil;

// main() - entry point for macOS
int main(int argc, char* argv[]) {
    @autoreleasepool {
        // Create the shared app instance
        gApplication = [NSApplication sharedApplication];

        // Instantiate our custom app delegate from metal_window.mm
        MetalAppDelegate* delegate = [[MetalAppDelegate alloc] init];
        [gApplication setDelegate:delegate];

        // Run the event loop
        [gApplication run];
    }
    return 0;
}