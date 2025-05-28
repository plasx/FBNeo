#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>

@interface MetalViewDelegate : NSObject <MTKViewDelegate>
@end

@implementation MetalViewDelegate
- (void)mtkView:(MTKView *)view drawableSizeWillChange:(CGSize)size {
    // Handle resize if needed
}

- (void)drawInMTKView:(MTKView *)view {
    // Get the command buffer for this frame
    id<MTLCommandQueue> commandQueue = [view.device newCommandQueue];
    id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];
    
    // Get the current drawable
    id<CAMetalDrawable> drawable = view.currentDrawable;
    
    if (drawable) {
        // Create render pass descriptor
        MTLRenderPassDescriptor *renderPassDescriptor = view.currentRenderPassDescriptor;
        
        if (renderPassDescriptor) {
            // Create render encoder
            id<MTLRenderCommandEncoder> renderEncoder = 
                [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
            
            // Clear the view
            [renderEncoder endEncoding];
            
            // Present drawable
            [commandBuffer presentDrawable:drawable];
        }
    }
    
    // Commit command buffer
    [commandBuffer commit];
}
@end

@interface AppDelegate : NSObject <NSApplicationDelegate>
@property (strong) NSWindow *window;
@property (strong) MTKView *metalView;
@property (strong) MetalViewDelegate *viewDelegate;
@end

@implementation AppDelegate
- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    // Create main window
    NSRect frame = NSMakeRect(0, 0, 800, 600);
    _window = [[NSWindow alloc] initWithContentRect:frame
                                          styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable
                                            backing:NSBackingStoreBuffered
                                              defer:NO];
    [_window setTitle:@"FBNeo Metal Test"];
    [_window center];
    
    // Create Metal view
    _metalView = [[MTKView alloc] initWithFrame:frame device:MTLCreateSystemDefaultDevice()];
    if (!_metalView.device) {
        NSLog(@"Metal is not supported on this device");
        [NSApp terminate:nil];
        return;
    }
    
    // Configure Metal view
    _metalView.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
    _metalView.clearColor = MTLClearColorMake(0.0, 0.2, 0.4, 1.0);
    _metalView.enableSetNeedsDisplay = NO;
    _metalView.paused = NO;
    _window.contentView = _metalView;
    
    // Set up delegate
    _viewDelegate = [[MetalViewDelegate alloc] init];
    _metalView.delegate = _viewDelegate;
    
    // Show window
    [_window makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];
    
    NSLog(@"Metal device: %@", _metalView.device.name);
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    return YES;
}
@end

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        NSLog(@"FBNeo Metal Test - Simple Window");
        
        [NSApplication sharedApplication];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
        
        AppDelegate *delegate = [[AppDelegate alloc] init];
        [NSApp setDelegate:delegate];
        
        [NSApp run];
    }
    return 0;
} 