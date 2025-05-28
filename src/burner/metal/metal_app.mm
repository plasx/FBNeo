#include "ai/metal_ai_module.h"

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
    // ... existing initialization code ...
    
    // Initialize renderer
    [self initializeRenderer];
    
    // Initialize AI module
    fbneo::metal::ai::initialize(nullptr);
    NSLog(@"AI module initialized");
    
    // ... rest of the existing code ...
}

- (void)applicationWillTerminate:(NSNotification *)aNotification {
    // Shutdown AI module
    fbneo::metal::ai::shutdown();
    NSLog(@"AI module shutdown");
    
    // ... existing termination code ...
}

- (void)updateAIWithFrameData:(const void*)frameData width:(int)width height:(int)height pitch:(int)pitch {
    if (frameData && width > 0 && height > 0) {
        fbneo::metal::ai::processFrame(frameData, width, height, pitch);
    }
}

- (void)drawFrame {
    // ... existing drawing code ...
    
    // Get current frame data and pass it to AI module
    const void* frameData = [renderer getFrameBufferData];
    int width = [renderer getFrameWidth];
    int height = [renderer getFrameHeight];
    int pitch = [renderer getFramePitch];
    
    [self updateAIWithFrameData:frameData width:width height:height pitch:pitch];
    
    // ... rest of rendering code ...
}

- (void)setupUI {
    // ... existing UI setup code ...
    
    // Initialize AI menu
    extern void Metal_UI_Init();
    Metal_UI_Init();
    
    // ... rest of UI setup ...
} 