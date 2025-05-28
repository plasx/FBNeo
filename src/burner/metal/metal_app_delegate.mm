#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>
#include "ai/ai_input_integration.h"

extern "C" {
    void InitializeAIMenu(void* menuPtr);
}

@interface MetalAppDelegate : NSObject <NSApplicationDelegate>
@end

@implementation MetalAppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
    // Existing initialization code...
    
    // Initialize AI subsystem
    AI::InitializeAIInputSystem();
    
    // Initialize AI menu
    InitializeAIMenu((__bridge void *)[NSApp mainMenu]);
    
    // Rest of initialization...
}

- (void)applicationWillTerminate:(NSNotification *)aNotification {
    // Existing shutdown code...
    
    // Shutdown AI subsystem
    AI::ShutdownAIInputSystem();
    
    // Rest of shutdown...
}

- (void)runFrame {
    // Existing frame processing...
    
    // Process AI frame
    AI::ProcessAIFrame();
    
    // Rest of frame processing...
} 

@end 