#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#include "burner/metal/metal_declarations.h"
#include "burner/metal/metal_renderer.h"

// Simplified application delegate
@interface MetalAppDelegate : NSObject <NSApplicationDelegate>
@property (strong) NSWindow *window;
@property (strong) MTKView *metalView;
@end

@implementation MetalAppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    // Create a Metal device
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    if (!device) {
        NSLog(@"Metal is not supported on this device");
        [NSApp terminate:self];
        return;
    }
    
    // Create a window
    NSRect frame = NSMakeRect(0, 0, 800, 600);
    NSWindow *window = [[NSWindow alloc] initWithContentRect:frame
                                                   styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable
                                                     backing:NSBackingStoreBuffered
                                                       defer:NO];
    [window setTitle:@"FBNeo Metal"];
    
    // Create a Metal view
    _metalView = [[MTKView alloc] initWithFrame:frame device:device];
    [_metalView setColorPixelFormat:MTLPixelFormatBGRA8Unorm];
    [_metalView setClearColor:MTLClearColorMake(0.0, 0.0, 0.0, 1.0)];
    
    // Set the view as the window's content view
    [window setContentView:_metalView];
    [window makeKeyAndOrderFront:nil];
    _window = window;
    
    // Initialize the Metal renderer - cast the view to a void* for C compatibility
    void* viewPtr = (__bridge void*)_metalView;
    if (MetalRenderer_Init(viewPtr) != 0) {
        NSLog(@"Failed to initialize Metal renderer");
        [NSApp terminate:self];
        return;
    }
    
    // Basic initialization
    BurnLibInit();
    
    // Setup game loop
    dispatch_source_t timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, dispatch_get_main_queue());
    dispatch_source_set_timer(timer, DISPATCH_TIME_NOW, NSEC_PER_SEC / 60, NSEC_PER_SEC / 100);
    dispatch_source_set_event_handler(timer, ^{
        [self gameLoop];
    });
    dispatch_resume(timer);
}

- (void)gameLoop {
    // Update the game state
    Metal_RunFrame(1); // Run with drawing
    
    // Request a redraw of the view
    [_metalView setNeedsDisplay:YES];
}

- (void)applicationWillTerminate:(NSNotification *)notification {
    // Clean up
    MetalRenderer_Exit();
    BurnLibExit();
}

@end

// Main entry point
int main(int argc, const char *argv[]) {
    @autoreleasepool {
        NSApplication *app = [NSApplication sharedApplication];
        MetalAppDelegate *delegate = [[MetalAppDelegate alloc] init];
        [app setDelegate:delegate];
        [app run];
    }
    return 0;
}

// Implementations of required C functions
extern "C" {
    // Stubs for functions needed by the Metal renderer
    void MetalRenderer_Exit() {
        // Stub implementation
    }
    
    int MetalRenderer_Init(void* view) {
        // Stub implementation
        return 0;
    }
    
    void MetalRenderer_Render(void* commandBuffer) {
        // Stub implementation
    }
    
    int InpInit() {
        return 0;
    }
    
    int InpExit() {
        // Stub implementation
        return 0;
    }
    
    void InpUpdate() {
        // Stub implementation
    }
} 