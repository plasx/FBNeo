//
// main_metal.mm
//
// Entry point for Metal version of FBNeo
// Integrates all Metal components: renderer, audio, and input
//

#import <Cocoa/Cocoa.h>
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <GameController/GameController.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Include our declarations first
#include "metal_declarations.h"
#include "metal_audio.h"
#include "metal_input.h"
#include "metal_renderer.h"

// Global variables for data sharing with stubs
char szAppRomPaths[DIRS_MAX][MAX_PATH];
char szAppDirPath[MAX_PATH];
struct BurnDrvMeta BurnDrvInfo;

// FBNeo function declarations
extern "C" {
    void FixRomPaths();
    int SetROMPath(int index, const char* path);
    int BurnLibInit();
    int BurnLibExit();
    int BurnDrvSelect(int nDrvNum);
    int BurnDrvInit();
    int BurnDrvExit();
    int BurnDrvReset();
    int BurnDrvGetIndex(char* szName);
    char* BurnDrvGetTextA(UINT32 i);
    INT32 BurnDrvGetVisibleSize(INT32* pnWidth, INT32* pnHeight);
    INT32 BurnDrvGetAspect(INT32* pnXAspect, INT32* pnYAspect);
}

// Application delegate
@interface FBNeoAppDelegate : NSObject <NSApplicationDelegate, MTKViewDelegate>
@property (strong) NSWindow *window;
@property (strong) MTKView *metalView;
@property (strong) id<MTLDevice> device;
@property (strong) id<MTLCommandQueue> commandQueue;
@property (strong) CADisplayLink *displayLink;
@property (assign) BOOL isRunning;
@property (assign) BOOL isPaused;
@property (assign) double lastFrameTime;
@property (assign) double frameInterval;
@property (assign) int drvIndex;
@end

@implementation FBNeoAppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    // Initialize variables
    _isRunning = NO;
    _isPaused = NO;
    _lastFrameTime = 0;
    _frameInterval = 1.0 / 60.0; // 60 FPS default
    
    // Get the Metal device
    _device = MTLCreateSystemDefaultDevice();
    if (!_device) {
        NSLog(@"Metal is not supported on this device");
        [NSApp terminate:nil];
        return;
    }
    
    // Create command queue
    _commandQueue = [_device newCommandQueue];
    
    // Create window
    NSRect frame = NSMakeRect(0, 0, 800, 600);
    NSUInteger styleMask = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | 
                           NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable;
    
    _window = [[NSWindow alloc] initWithContentRect:frame
                                          styleMask:styleMask
                                            backing:NSBackingStoreBuffered
                                              defer:NO];
    
    [_window setTitle:@"FBNeo Metal Edition"];
    [_window setReleasedWhenClosed:NO];
    [_window center];
    [_window makeKeyAndOrderFront:nil];
    
    // Create MTKView
    _metalView = [[MTKView alloc] initWithFrame:frame device:_device];
    _metalView.delegate = self;
    _metalView.clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);
    _metalView.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
    _metalView.depthStencilPixelFormat = MTLPixelFormatDepth32Float;
    _metalView.sampleCount = 1;
    _metalView.enableSetNeedsDisplay = NO;  // We'll control frame rendering
    _metalView.paused = NO;                // We want continuous rendering
    _metalView.preferredFramesPerSecond = 60; // CPS2 games run at 60 FPS
    
    // Set the view as the window's content view
    _window.contentView = _metalView;
    
    // Initialize subsystems
    if (![self initializeSubsystems]) {
        [NSApp terminate:nil];
        return;
    }
    
    // Set up display link for vsync'd rendering
    CVDisplayLinkRef displayLink;
    CVDisplayLinkCreateWithActiveCGDisplays(&displayLink);
    
    // Use displayLink to set up frameInterval based on the display's refresh rate
    CVTime refreshPeriod = CVDisplayLinkGetNominalOutputVideoRefreshPeriod(displayLink);
    if (refreshPeriod.timeScale > 0) {
        double refreshRate = (double)refreshPeriod.timeScale / (double)refreshPeriod.timeValue;
        _frameInterval = 1.0 / refreshRate;
        NSLog(@"Display refresh rate: %.2f Hz, frame interval: %.4f seconds", refreshRate, _frameInterval);
    }
    
    CVDisplayLinkRelease(displayLink);
    
    // Set up timer for main game loop
    [self startGameLoop];
}

- (BOOL)initializeSubsystems {
    NSLog(@"Initializing subsystems");
    
    // Initialize audio
    if (AudSoundInit() != 0) {
        NSLog(@"Failed to initialize audio");
        return NO;
    }
    
    // Initialize input
    if (InpInit() != 0) {
        NSLog(@"Failed to initialize input");
        AudSoundExit();
        return NO;
    }
    
    // Initialize video/renderer
    if (MetalRenderer_Init(_device, _metalView) != 0) {
        NSLog(@"Failed to initialize renderer");
        InpExit();
        AudSoundExit();
        return NO;
    }
    
    // Initialize FBNeo library
    if (BurnLibInit() != 0) {
        NSLog(@"Failed to initialize FBNeo library");
        MetalRenderer_Exit();
        InpExit();
        AudSoundExit();
        return NO;
    }
    
    NSLog(@"All subsystems initialized successfully");
    return YES;
}

- (void)cleanupSubsystems {
    NSLog(@"Cleaning up subsystems");
    
    // Exit FBNeo driver if active
    if (_isRunning) {
        BurnDrvExit();
        _isRunning = NO;
    }
    
    // Clean up FBNeo library
    BurnLibExit();
    
    // Clean up renderer
    MetalRenderer_Exit();
    
    // Clean up input
    InpExit();
    
    // Clean up audio
    AudSoundExit();
    
    NSLog(@"All subsystems cleaned up");
}

- (void)startGameLoop {
    // Set up a timer to drive our game loop
    NSTimer *timer = [NSTimer scheduledTimerWithTimeInterval:_frameInterval
                                                     repeats:YES
                                                       block:^(NSTimer *timer) {
        [self gameLoopTick];
    }];
    
    // Add the timer to the current run loop in common modes
    [[NSRunLoop currentRunLoop] addTimer:timer forMode:NSRunLoopCommonModes];
    
    _lastFrameTime = CACurrentMediaTime();
    NSLog(@"Game loop started");
}

- (void)gameLoopTick {
    if (_isPaused) {
        return;
    }
    
    // Calculate delta time
    double currentTime = CACurrentMediaTime();
    double deltaTime = currentTime - _lastFrameTime;
    _lastFrameTime = currentTime;
    
    // Update input
    InpUpdate();
    
    // Run a frame of emulation
    if (_isRunning) {
        // Run a single frame of emulation with drawing enabled
        Metal_RunFrame(true);
    }
    
    // Force a draw to ensure the frame is displayed
    // The Metal renderer will pick up the frame buffer on the next draw
    [_metalView draw];
    
    // Debug frame rate info (every 60 frames)
    static int frameCounter = 0;
    if (++frameCounter % 60 == 0) {
        NSLog(@"FPS: %.2f", 1.0 / deltaTime);
    }
}

- (BOOL)loadGame:(NSString *)romPath {
    // Exit previous game if running
    if (_isRunning) {
        BurnDrvExit();
        _isRunning = NO;
    }
    
    // Get ROM directory
    NSString *romDir = [romPath stringByDeletingLastPathComponent];
    
    // Set ROM path
    SetROMPath(0, [romDir UTF8String]);
    FixRomPaths();
    
    // Get ROM name without extension
    NSString *romName = [[romPath lastPathComponent] stringByDeletingPathExtension];
    
    // Get driver index
    NSLog(@"Looking for driver: %@", romName);
    int drvIndex = BurnDrvGetIndex((char *)[romName UTF8String]);
    if (drvIndex < 0) {
        NSLog(@"Error: Could not find driver for ROM: %@", romName);
        return NO;
    }
    
    _drvIndex = drvIndex;
    NSLog(@"Driver index: %d", drvIndex);
    
    // Select and initialize the driver
    BurnDrvSelect(drvIndex);
    
    // Get game info
    const char *gameName = BurnDrvGetTextA(DRV_FULLNAME);
    NSLog(@"Loading game: %s", gameName);
    
    // Get game dimensions
    INT32 width, height;
    BurnDrvGetVisibleSize(&width, &height);
    
    // Get aspect ratio
    INT32 xAspect, yAspect;
    BurnDrvGetAspect(&xAspect, &yAspect);
    
    NSLog(@"Game dimensions: %dx%d, Aspect ratio: %d:%d", width, height, xAspect, yAspect);
    
    // Initialize the driver
    int initResult = BurnDrvInit();
    if (initResult != 0) {
        NSLog(@"Error: Failed to initialize driver: %d", initResult);
        return NO;
    }
    
    // Start audio
    MetalAudio_Start();
    
    // Update window title with game name
    [_window setTitle:[NSString stringWithFormat:@"FBNeo Metal - %s", gameName]];
    
    // Resize window based on game dimensions if needed
    float aspectRatio = (float)xAspect / (float)yAspect;
    NSRect windowFrame = [_window frame];
    NSRect contentRect = [_window contentRectForFrameRect:windowFrame];
    
    // Target height based on current width
    float targetHeight = contentRect.size.width / aspectRatio;
    
    // If height needs to change significantly, resize the window
    if (fabs(targetHeight - contentRect.size.height) > 10.0f) {
        float heightDiff = targetHeight - contentRect.size.height;
        windowFrame.size.height += heightDiff;
        [_window setFrame:windowFrame display:YES animate:YES];
    }
    
    _isRunning = YES;
    _isPaused = NO;
    
    NSLog(@"Game loaded successfully");
    return YES;
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    return YES;
}

- (void)applicationWillTerminate:(NSNotification *)notification {
    [self cleanupSubsystems];
}

#pragma mark - MTKViewDelegate

- (void)drawInMTKView:(MTKView *)view {
    // Create a command buffer
    id<MTLCommandBuffer> commandBuffer = [_commandQueue commandBuffer];
    
    // Render the current frame
    MetalRenderer_Render(commandBuffer);
    
    // Commit the command buffer
    [commandBuffer commit];
}

- (void)mtkView:(MTKView *)view drawableSizeWillChange:(CGSize)size {
    // Handle window resize
    NSLog(@"Drawable size changed to %.0f x %.0f", size.width, size.height);
}

- (void)togglePause {
    _isPaused = !_isPaused;
    MetalAudio_Pause(_isPaused);
    NSLog(@"Game %@", _isPaused ? @"paused" : @"resumed");
}

- (void)resetGame {
    if (_isRunning) {
        BurnDrvReset();
        NSLog(@"Game reset");
    }
}

- (void)setupMetalView {
    // Create Metal device
    _device = MTLCreateSystemDefaultDevice();
    if (!_device) {
        NSLog(@"Metal is not supported on this device");
        return;
    }

    // Create Metal view
    NSRect frame = NSMakeRect(0, 0, 800, 600); // Initial size, will be adjusted for game
    _metalView = [[MTKView alloc] initWithFrame:frame device:_device];
    _metalView.clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);
    _metalView.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
    _metalView.depthStencilPixelFormat = MTLPixelFormatDepth32Float;
    _metalView.sampleCount = 1;
    
    // Configure for continuous rendering
    _metalView.enableSetNeedsDisplay = NO;  // Don't wait for setNeedsDisplay
    _metalView.paused = NO;                 // Continuously render
    _metalView.preferredFramesPerSecond = 60; // CPS2 games run at 60 FPS
    
    // Create renderer
    _renderer = [[MetalRenderer alloc] initWithDevice:_device];
    _metalView.delegate = _renderer;
    
    // Add view to window
    [self.window.contentView addSubview:_metalView];
    _metalView.frame = self.window.contentView.bounds;
    
    // Auto-resize with window
    [_metalView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
    
    NSLog(@"Metal view setup complete with continuous rendering at 60 FPS");
}

@end

// Command line argument handling
int ProcessCommandLine(int argc, const char* argv[], NSString **romPath) {
    if (argc < 2) {
        printf("Usage: %s <rom_file>\n", argv[0]);
        printf("Example: %s /path/to/roms/mvsc.zip\n", argv[0]);
        return 1;
    }
    
    // Get the ROM file path
    const char* romPathStr = argv[1];
    *romPath = [NSString stringWithUTF8String:romPathStr];
    
    return 0;
}

// Main function
int main(int argc, const char* argv[]) {
    @autoreleasepool {
        NSLog(@"FBNeo Metal Edition starting up");
        
        // Create the application
        [NSApplication sharedApplication];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
        
        // Process command line arguments
        NSString *romPath = nil;
        if (ProcessCommandLine(argc, argv, &romPath) != 0) {
            return 1;
        }
        
        // Create the app delegate
        FBNeoAppDelegate *appDelegate = [[FBNeoAppDelegate alloc] init];
        [NSApp setDelegate:appDelegate];
        
        // Set up main menu
        NSMenu *mainMenu = [[NSMenu alloc] init];
        NSMenuItem *appMenuItem = [[NSMenuItem alloc] init];
        [mainMenu addItem:appMenuItem];
        
        NSMenu *appMenu = [[NSMenu alloc] init];
        [appMenuItem setSubmenu:appMenu];
        
        NSMenuItem *quitMenuItem = [[NSMenuItem alloc] initWithTitle:@"Quit"
                                                              action:@selector(terminate:)
                                                       keyEquivalent:@"q"];
        [appMenu addItem:quitMenuItem];
        
        // Add Game menu
        NSMenuItem *gameMenuItem = [[NSMenuItem alloc] init];
        [gameMenuItem setTitle:@"Game"];
        [mainMenu addItem:gameMenuItem];
        
        NSMenu *gameMenu = [[NSMenu alloc] init];
        [gameMenuItem setSubmenu:gameMenu];
        
        NSMenuItem *pauseMenuItem = [[NSMenuItem alloc] initWithTitle:@"Pause"
                                                               action:@selector(togglePause)
                                                        keyEquivalent:@"p"];
        [pauseMenuItem setTarget:appDelegate];
        [gameMenu addItem:pauseMenuItem];
        
        NSMenuItem *resetMenuItem = [[NSMenuItem alloc] initWithTitle:@"Reset"
                                                               action:@selector(resetGame)
                                                        keyEquivalent:@"r"];
        [resetMenuItem setTarget:appDelegate];
        [gameMenu addItem:resetMenuItem];
        
        [NSApp setMainMenu:mainMenu];
        
        // Run the application
        [NSApp finishLaunching];
        
        // Load ROM if specified
        if (romPath) {
            dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(1.0 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
                [appDelegate loadGame:romPath];
            });
        }
        
        // Run the application
        [NSApp run];
    }
    
    return 0;
}
