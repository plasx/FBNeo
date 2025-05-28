#import <Cocoa/Cocoa.h>
#include "burner.h"

// When building without AI support (USE_AI=0), we provide empty stubs
// for the AI menu integration functions

@interface AIMenuIntegration : NSObject

+ (void)setupAIMenu:(NSMenu *)mainMenu;

@end

@implementation AIMenuIntegration

+ (void)setupAIMenu:(NSMenu *)mainMenu {
    // AI features are disabled, so we don't create any menus
    // This is an empty stub to satisfy external references
}

@end

// C-compatible function for initialization from FBNeo
extern "C" {
    void InitializeAIMenu(void* menuPtr) {
        // Empty implementation when AI features are disabled
        // This prevents linker errors when USE_AI=0
    }
} 