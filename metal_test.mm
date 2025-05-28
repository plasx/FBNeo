#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>

@interface AppDelegate : NSObject <NSApplicationDelegate>
@property (strong) NSWindow *window;
@property (strong) MTKView *metalView;
@end

@implementation AppDelegate
- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    NSRect frame = NSMakeRect(0, 0, 800, 600);
    
    _window = [[NSWindow alloc] initWithContentRect:frame
                                          styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable
                                            backing:NSBackingStoreBuffered
                                              defer:NO];
    [_window setTitle:@"FBNeo Metal Test"];
    [_window center];
    
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    if (!device) {
        NSLog(@"Metal is not supported on this device");
        [NSApp terminate:nil];
        return;
    }
    
    _metalView = [[MTKView alloc] initWithFrame:frame device:device];
    _metalView.clearColor = MTLClearColorMake(0.0, 0.0, 0.2, 1.0);
    _metalView.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
    _metalView.depthStencilPixelFormat = MTLPixelFormatDepth32Float;
    _window.contentView = _metalView;
    
    // Setup rendering pipeline
    NSError *error = nil;
    id<MTLLibrary> library = [device newDefaultLibrary];
    if (!library) {
        // Try to load compiled shader library from path
        NSString *shaderPath = [NSString stringWithFormat:@"%@/src/burner/metal/fbneo_shaders.metallib", 
                               [[NSFileManager defaultManager] currentDirectoryPath]];
        NSURL *libraryURL = [NSURL fileURLWithPath:shaderPath];
        library = [device newLibraryWithURL:libraryURL error:&error];
        
        if (!library) {
            NSLog(@"Failed to load Metal library: %@", error);
            [NSApp terminate:nil];
            return;
        }
    }
    
    // Print available shader functions
    NSArray<NSString *> *functionNames = [library functionNames];
    NSLog(@"Available shader functions:");
    for (NSString *name in functionNames) {
        NSLog(@"  - %@", name);
    }
    
    [_window makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    return YES;
}
@end

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        NSLog(@"FBNeo Metal Test Application");
        NSLog(@"Metal device: %@", MTLCreateSystemDefaultDevice().name);
        
        [NSApplication sharedApplication];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
        
        AppDelegate *delegate = [[AppDelegate alloc] init];
        [NSApp setDelegate:delegate];
        
        [NSApp run];
    }
    return 0;
}
