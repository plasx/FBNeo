#import <Cocoa/Cocoa.h>
#import <MetalKit/MetalKit.h>

// Forward declarations for C functions
extern void updateApp(void);

// Global variables
static NSWindow* window = nil;
static MTKView* metalView = nil;
static NSTimer* renderTimer = nil;
static BOOL isRunning = NO;

// Create a Metal view and add it to a window
void* CreateMetalView(int width, int height) {
    @autoreleasepool {
        // Ensure we're on the main thread
        if (![NSThread isMainThread]) {
            __block void* result = NULL;
            dispatch_sync(dispatch_get_main_queue(), ^{
                result = CreateMetalView(width, height);
            });
            return result;
        }
        
        // Create the application if it doesn't exist
        if (NSApp == nil) {
            [NSApplication sharedApplication];
            [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
            
            // Create a menu bar
            NSMenu* menuBar = [NSMenu new];
            NSMenuItem* appMenuItem = [NSMenuItem new];
            [menuBar addItem:appMenuItem];
            [NSApp setMainMenu:menuBar];
            
            NSMenu* appMenu = [NSMenu new];
            NSMenuItem* quitMenuItem = [[NSMenuItem alloc] initWithTitle:@"Quit" 
                                                                  action:@selector(terminate:) 
                                                           keyEquivalent:@"q"];
            [appMenu addItem:quitMenuItem];
            [appMenuItem setSubmenu:appMenu];
        }
        
        // Create a window
        NSRect frame = NSMakeRect(0, 0, width, height);
        window = [[NSWindow alloc] initWithContentRect:frame
                                             styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable
                                               backing:NSBackingStoreBuffered
                                                 defer:NO];
        [window setTitle:@"FBNeo Metal Test"];
        [window center];
        
        // Create a Metal view
        metalView = [[MTKView alloc] initWithFrame:frame];
        metalView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
        
        // Add the view to the window
        [window setContentView:metalView];
        
        // Show the window
        [window makeKeyAndOrderFront:nil];
        [NSApp activateIgnoringOtherApps:YES];
        
        return (__bridge void*)metalView;
    }
}

// Start the main loop
void RunMainLoop(void) {
    @autoreleasepool {
        if (![NSThread isMainThread]) {
            dispatch_sync(dispatch_get_main_queue(), ^{
                RunMainLoop();
            });
            return;
        }
        
        // Set up a timer to call the update function
        isRunning = YES;
        renderTimer = [NSTimer scheduledTimerWithTimeInterval:1.0/60.0
                                                      repeats:YES
                                                        block:^(NSTimer * _Nonnull timer) {
            if (!isRunning) {
                [timer invalidate];
                return;
            }
            
            @autoreleasepool {
                // Call the C update function
                updateApp();
            }
        }];
        
        // Add window close notification handler
        [[NSNotificationCenter defaultCenter] addObserverForName:NSWindowWillCloseNotification
                                                          object:window
                                                           queue:[NSOperationQueue mainQueue]
                                                      usingBlock:^(NSNotification * _Nonnull note) {
            isRunning = NO;
            [NSApp terminate:nil];
        }];
        
        // Run the application
        [NSApp run];
    }
}

// Shutdown the application
void ShutdownApp(void) {
    @autoreleasepool {
        if (![NSThread isMainThread]) {
            dispatch_sync(dispatch_get_main_queue(), ^{
                ShutdownApp();
            });
            return;
        }
        
        isRunning = NO;
        
        // Invalidate the timer
        if (renderTimer) {
            [renderTimer invalidate];
            renderTimer = nil;
        }
        
        // Close the window
        if (window) {
            [window close];
            window = nil;
        }
        
        // Stop the application
        [NSApp terminate:nil];
    }
} 