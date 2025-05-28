#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <Cocoa/Cocoa.h>

// FBNeo include for video driver
extern "C" {
    // Metal initialization & setup
    int Metal_Init();
    void Metal_Exit();
    void Metal_SetScreenSize(int width, int height);
    void Metal_RenderFrame(unsigned char* buffer, int width, int height, int pitch);
    void Metal_SetWindowTitle(const char* title);
    
    // For testing
    void RunTestPattern(int testMode);
}

// Test pattern generation
void RunTestPattern(int testMode) {
    // Create a test pattern buffer
    int width = 384;
    int height = 224;
    int pitch = width * 4; // 4 bytes per pixel (BGRA)
    
    // Allocate buffer
    unsigned char* buffer = (unsigned char*)malloc(height * pitch);
    if (!buffer) {
        NSLog(@"Failed to allocate test pattern buffer");
        return;
    }
    
    // Set window title based on test mode
    char title[128];
    snprintf(title, sizeof(title), "FBNeo Metal - Test Pattern %d", testMode);
    Metal_SetWindowTitle(title);
    
    // Generate test pattern
    for (int frame = 0; frame < 600; frame++) {
        // Fill the buffer based on test mode
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                unsigned char* pixel = buffer + (y * pitch) + (x * 4);
                
                switch (testMode) {
                    case 0: { // Gradient pattern
                        pixel[0] = (unsigned char)((float)x / width * 255); // B
                        pixel[1] = (unsigned char)((float)y / height * 255); // G
                        pixel[2] = (unsigned char)((float)(x + y) / (width + height) * 255); // R
                        pixel[3] = 255; // A
                        break;
                    }
                    case 1: { // Animated pattern
                        float time = frame / 60.0f;
                        pixel[0] = (unsigned char)((sin(x * 0.1f + time) * 0.5f + 0.5f) * 255); // B
                        pixel[1] = (unsigned char)((cos(y * 0.1f + time) * 0.5f + 0.5f) * 255); // G
                        pixel[2] = (unsigned char)((sin((x + y) * 0.1f + time) * 0.5f + 0.5f) * 255); // R
                        pixel[3] = 255; // A
                        break;
                    }
                    case 2: { // CPS2-style test pattern
                        // Grid pattern
                        bool isGridLine = (x % 32 == 0) || (y % 32 == 0);
                        if (isGridLine) {
                            pixel[0] = 255; // B (white grid lines)
                            pixel[1] = 255; // G
                            pixel[2] = 255; // R
                        } else {
                            // Fill with a color based on position
                            int gridX = x / 32;
                            int gridY = y / 32;
                            
                            if ((gridX + gridY) % 2 == 0) {
                                // Checkerboard pattern
                                pixel[0] = 64; // B
                                pixel[1] = 0;  // G
                                pixel[2] = 128; // R
                            } else {
                                pixel[0] = 128; // B
                                pixel[1] = 64;  // G
                                pixel[2] = 0; // R
                            }
                        }
                        pixel[3] = 255; // A
                        break;
                    }
                    default: { // Checkerboard
                        bool isWhite = ((x / 16) + (y / 16)) % 2 == 0;
                        pixel[0] = isWhite ? 255 : 0; // B
                        pixel[1] = isWhite ? 255 : 0; // G
                        pixel[2] = isWhite ? 255 : 0; // R
                        pixel[3] = 255; // A
                        break;
                    }
                }
            }
        }
        
        // Draw frame counter in top-left corner
        char frameText[16];
        snprintf(frameText, sizeof(frameText), "Frame: %d", frame);
        
        // Render frame
        Metal_RenderFrame(buffer, width, height, pitch);
        
        // Sleep to control frame rate
        [NSThread sleepForTimeInterval:1.0/60.0];
    }
    
    // Free buffer
    free(buffer);
}

// Application delegate
@interface AppDelegate : NSObject <NSApplicationDelegate>
@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    // Initialize Metal
    Metal_Init();
    
    // Set up the screen size
    Metal_SetScreenSize(384, 224);
    
    // Set window title
    Metal_SetWindowTitle("FBNeo Metal Integration Test");
    
    // Run test pattern in a separate thread
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        RunTestPattern(1); // Run animated test pattern
    });
}

- (void)applicationWillTerminate:(NSNotification *)notification {
    // Clean up Metal
    Metal_Exit();
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    return YES;
}

@end

// Main entry point
int main(int argc, const char * argv[]) {
    @autoreleasepool {
        NSLog(@"Starting FBNeo Metal Integration Test");
        
        // Create application
        [NSApplication sharedApplication];
        
        // Create app delegate
        AppDelegate *delegate = [[AppDelegate alloc] init];
        [NSApp setDelegate:delegate];
        
        // Create basic menu
        NSMenu *menubar = [[NSMenu alloc] init];
        NSMenuItem *appMenuItem = [[NSMenuItem alloc] init];
        [menubar addItem:appMenuItem];
        NSMenu *appMenu = [[NSMenu alloc] init];
        NSMenuItem *quitMenuItem = [[NSMenuItem alloc] initWithTitle:@"Quit" 
                                                             action:@selector(terminate:) 
                                                      keyEquivalent:@"q"];
        [appMenu addItem:quitMenuItem];
        [appMenuItem setSubmenu:appMenu];
        [NSApp setMainMenu:menubar];
        
        // Run the application
        [NSApp activateIgnoringOtherApps:YES];
        [NSApp run];
    }
    return 0;
} 