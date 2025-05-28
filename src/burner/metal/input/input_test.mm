#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>
#import <GameController/GameController.h>

// Include FBNeo core headers
#include "metal_input.h"
#include "burnint.h"
#include "../fixes/cps_input_full.h"
#include "../../gameinp.h"

// Test application for FBNeo Metal port input handling
// This can be compiled and run separately to debug input issues

@interface InputTestDelegate : NSObject <NSApplicationDelegate>
- (void)applicationDidFinishLaunching:(NSNotification *)notification;
- (void)applicationWillTerminate:(NSNotification *)notification;
- (void)testLoop;
@end

@implementation InputTestDelegate {
    BOOL shouldContinue;
    NSTimer *timer;
}

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    shouldContinue = YES;
    
    // Initialize the Metal input system
    if (Metal_InitInput() != 0) {
        NSLog(@"Failed to initialize Metal input system");
        [NSApp terminate:self];
        return;
    }
    
    // Start the test loop timer (30 times per second)
    timer = [NSTimer scheduledTimerWithTimeInterval:1.0/30.0
                                            target:self
                                          selector:@selector(testLoop)
                                          userInfo:nil
                                           repeats:YES];
    
    NSLog(@"Input test started. Press Escape to exit.");
}

- (void)applicationWillTerminate:(NSNotification *)notification {
    // Stop the timer
    [timer invalidate];
    timer = nil;
    
    // Shutdown input system
    Metal_ShutdownInput();
}

- (void)testLoop {
    // Process input events
    Metal_ProcessInput();
    
    // Get a timestamp for the output
    NSDateFormatter *formatter = [[NSDateFormatter alloc] init];
    [formatter setDateFormat:@"HH:mm:ss.SSS"];
    NSString *timestamp = [formatter stringFromDate:[NSDate date]];
    
    // Clear the terminal and print the header
    printf("\033[2J\033[H"); // Clear screen and move cursor to top
    printf("===== FBNeo Metal Input Test [%s] =====\n\n", [timestamp UTF8String]);
    
    // Check for escape key to exit
    extern bool MetalInput::keyboardState[];
    if (MetalInput::keyboardState[53]) { // 53 is Escape on macOS
        shouldContinue = NO;
        [NSApp terminate:self];
        return;
    }
    
    // Print CPS input values (for Capcom games)
    printf("CPS Input Values:\n");
    printf("Player 1 (CpsInp000): %02X %02X %02X %02X\n", 
           CpsInp000[0], CpsInp000[1], CpsInp000[2], CpsInp000[3]);
    printf("Player 2 (CpsInp001): %02X %02X %02X %02X\n", 
           CpsInp001[0], CpsInp001[1], CpsInp001[2], CpsInp001[3]);
    printf("System  (CpsInp011): %02X %02X %02X %02X\n", 
           CpsInp011[0], CpsInp011[1], CpsInp011[2], CpsInp011[3]);
    
    // Print connected gamepads and their state
    extern MetalInput::GamepadState MetalInput::gamepads[];
    printf("\nConnected Controllers:\n");
    
    bool foundController = false;
    for (int i = 0; i < MetalInput::MAX_GAMEPADS; i++) {
        if (MetalInput::gamepads[i].connected) {
            foundController = true;
            printf("Controller %d: %s\n", i + 1, [MetalInput::gamepads[i].name UTF8String]);
            
            // Print button states
            printf("  Buttons: ");
            for (int b = 0; b < 16; b++) {
                if (MetalInput::gamepads[i].buttons[b]) {
                    printf("%2d ", b);
                }
            }
            printf("\n");
            
            // Print analog stick values
            printf("  Left Stick:  X=%.2f  Y=%.2f\n", 
                   MetalInput::gamepads[i].axes[0], 
                   MetalInput::gamepads[i].axes[1]);
            printf("  Right Stick: X=%.2f  Y=%.2f\n", 
                   MetalInput::gamepads[i].axes[2], 
                   MetalInput::gamepads[i].axes[3]);
            printf("  Triggers:    L=%.2f  R=%.2f\n", 
                   MetalInput::gamepads[i].axes[4], 
                   MetalInput::gamepads[i].axes[5]);
            printf("\n");
        }
    }
    
    if (!foundController) {
        printf("No controllers connected\n");
    }
    
    // Print keyboard state (just a few important keys)
    printf("\nKeyboard State:\n");
    printf("  Arrow Keys: %s %s %s %s\n",
           MetalInput::keyboardState[126] ? "UP" : "  ",
           MetalInput::keyboardState[125] ? "DOWN" : "    ",
           MetalInput::keyboardState[123] ? "LEFT" : "    ",
           MetalInput::keyboardState[124] ? "RIGHT" : "     ");
    
    printf("  IJKL Keys:  %s %s %s %s\n",
           MetalInput::keyboardState[34] ? "I" : " ",
           MetalInput::keyboardState[40] ? "K" : " ",
           MetalInput::keyboardState[38] ? "J" : " ",
           MetalInput::keyboardState[37] ? "L" : " ");
    
    printf("  A,S,D,Z,X,C: %s %s %s %s %s %s\n",
           MetalInput::keyboardState[0] ? "A" : " ",
           MetalInput::keyboardState[1] ? "S" : " ",
           MetalInput::keyboardState[2] ? "D" : " ",
           MetalInput::keyboardState[6] ? "Z" : " ",
           MetalInput::keyboardState[7] ? "X" : " ",
           MetalInput::keyboardState[8] ? "C" : " ");
    
    printf("\nPress Escape to exit\n");
}

@end

// Main function for standalone testing
#ifdef STANDALONE_INPUT_TEST
int main(int argc, const char * argv[]) {
    @autoreleasepool {
        NSApplication *app = [NSApplication sharedApplication];
        InputTestDelegate *delegate = [[InputTestDelegate alloc] init];
        [app setDelegate:delegate];
        [app run];
    }
    
    return 0;
}
#endif 