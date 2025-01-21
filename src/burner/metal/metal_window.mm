// src/burner/metal/metal_window.mm

#import "metal_window.h"
#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

// We'll reference a function from metal_gui.mm that does the final copy:
extern void MetalRunFrame(CAMetalLayer* layer);

@implementation MetalAppDelegate {
    NSWindow*     _mainWindow;
    CAMetalLayer* _metalLayer;
    id<MTLDevice> _metalDevice;
    NSTimer*      _renderTimer;
}

// Called when the application has finished launching
- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    // 1. Create the NSWindow
    NSRect contentRect = NSMakeRect(100, 100, 640, 480);
    _mainWindow = [[NSWindow alloc]
        initWithContentRect:contentRect
                  styleMask:(NSWindowStyleMaskTitled |
                             NSWindowStyleMaskClosable |
                             NSWindowStyleMaskResizable)
                    backing:NSBackingStoreBuffered
                      defer:NO];
    [_mainWindow setTitle:@"FBNeo Metal"];
    [_mainWindow makeKeyAndOrderFront:nil];

    // 2. Create the Metal device & CAMetalLayer
    _metalDevice = MTLCreateSystemDefaultDevice();
    _metalLayer = [CAMetalLayer layer];
    _metalLayer.device = _metalDevice;
    _metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    _metalLayer.contentsScale = [[_mainWindow screen] backingScaleFactor];

    // 3. Attach the layer to the window's contentView
    NSView* contentView = [_mainWindow contentView];
    [contentView setWantsLayer:YES];
    [contentView setLayer:_metalLayer];

    // 4. Start a timer to run at ~60 FPS
    _renderTimer = [NSTimer scheduledTimerWithTimeInterval:(1.0 / 60.0)
                                                    target:self
                                                  selector:@selector(renderLoop)
                                                  userInfo:nil
                                                   repeats:YES];
}

// This gets called ~60 times/sec
- (void)renderLoop {
    @autoreleasepool {
        // 1. Possibly check if a game is loaded, etc.
        // 2. Call the function that runs FBNeo for 1 frame
        //    and copies pBurnDraw to the CAMetalLayer
        MetalRunFrame(_metalLayer);
    }
}

// If user closes the main window, quit the app
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    return YES;
}

@end