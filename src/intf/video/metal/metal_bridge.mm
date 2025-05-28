#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <Cocoa/Cocoa.h>
#import "metal_bridge.h"

// External variables
extern uint32_t* g_frameBuffer;
extern int g_frameWidth;
extern int g_frameHeight;
extern bool g_frameUpdated;
extern PostProcessParams g_postProcessParams;

// Window reference
static NSWindow* g_window = nil;
static MTKView* g_metalView = nil;

// Core frame buffer reference
static unsigned char* g_coreFrameBuffer = NULL;
static int g_coreFrameWidth = 0;
static int g_coreFrameHeight = 0;
static int g_coreFramePitch = 0;

// Initialize the Metal renderer
int Metal_Init(void* windowHandle, int width, int height) {
    NSLog(@"Metal_Init: Initializing Metal renderer %dx%d", width, height);
    
    // Store dimensions for later use
    g_coreFrameWidth = width;
    g_coreFrameHeight = height;
    
    // Create window if not provided
    if (windowHandle == NULL) {
        NSRect frame = NSMakeRect(0, 0, width, height);
        g_window = [[NSWindow alloc] initWithContentRect:frame
                                              styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable
                                                backing:NSBackingStoreBuffered
                                                  defer:NO];
        
        [g_window setTitle:@"FBNeo Metal Renderer"];
        [g_window center];
        
        // Show window
        [g_window makeKeyAndOrderFront:nil];
        
        windowHandle = (__bridge void*)g_window.contentView;
    }
    
    // Create Metal view
    if (g_metalView == nil) {
        // Get Metal device
        id<MTLDevice> device = MTLCreateSystemDefaultDevice();
        if (!device) {
            NSLog(@"Metal is not supported on this device");
            return 0;
        }
        
        // Assuming windowHandle is an NSView*
        NSView* parentView = (__bridge NSView*)windowHandle;
        g_metalView = [[MTKView alloc] initWithFrame:parentView.bounds device:device];
        g_metalView.clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);
        g_metalView.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
        g_metalView.depthStencilPixelFormat = MTLPixelFormatInvalid;
        g_metalView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
        g_metalView.enableSetNeedsDisplay = YES;
        
        [parentView addSubview:g_metalView];
    }
    
    // Initialize internal frame buffer dimensions to match core
    g_frameWidth = g_coreFrameWidth;
    g_frameHeight = g_coreFrameHeight;
    
    // Update post-processing parameters based on game dimensions
    g_postProcessParams.textureSize[0] = g_frameWidth;
    g_postProcessParams.textureSize[1] = g_frameHeight;
    g_postProcessParams.scanlineCount = g_frameHeight;
    
    return 1;
}

// Shutdown the Metal renderer
void Metal_Shutdown() {
    NSLog(@"Metal_Shutdown: Cleaning up Metal resources");
    
    // Clean up Metal view
    if (g_metalView) {
        [g_metalView removeFromSuperview];
        g_metalView = nil;
    }
    
    // Clean up window if we created it
    if (g_window) {
        [g_window close];
        g_window = nil;
    }
    
    // Free frame buffer memory
    if (g_frameBuffer) {
        free(g_frameBuffer);
        g_frameBuffer = NULL;
    }
    
    // Reset core frame buffer pointer
    g_coreFrameBuffer = NULL;
}

// Set the frame buffer pointer and dimensions
void Metal_SetFrameBuffer(unsigned char* buffer, int width, int height, int pitch) {
    NSLog(@"Metal_SetFrameBuffer: Setting frame buffer %dx%d, pitch=%d, buffer=%p", width, height, pitch, buffer);
    
    // Validate input parameters
    if (width <= 0 || height <= 0) {
        NSLog(@"Metal_SetFrameBuffer: ERROR - Invalid dimensions (%dx%d)", width, height);
        return;
    }
    
    if (pitch <= 0) {
        // Use a default pitch if none is specified
        pitch = width * 4;
        NSLog(@"Metal_SetFrameBuffer: Using default pitch=%d", pitch);
    }
    
    // Check if the buffer pointer is valid
    if (buffer == NULL) {
        NSLog(@"Metal_SetFrameBuffer: WARNING - NULL buffer received, creating fallback buffer");
        
        // Create a fallback buffer if FBNeo core didn't provide one
        static unsigned char* fallbackBuffer = NULL;
        static int fallbackSize = 0;
        
        int requiredSize = height * pitch;
        if (fallbackBuffer == NULL || fallbackSize < requiredSize) {
            // Free previous buffer if it exists but is too small
            if (fallbackBuffer != NULL) {
                free(fallbackBuffer);
            }
            
            // Allocate a new buffer
            fallbackBuffer = (unsigned char*)malloc(requiredSize);
            if (fallbackBuffer) {
                fallbackSize = requiredSize;
                // Initialize with a checkered pattern to make it obvious this is a fallback
                for (int y = 0; y < height; y++) {
                    for (int x = 0; x < width; x++) {
                        int pixelOffset = y * pitch + x * 4;
                        // Create a purple/blue checkered pattern
                        bool isAlternate = ((x / 32) + (y / 32)) % 2 == 0;
                        fallbackBuffer[pixelOffset + 0] = isAlternate ? 0xFF : 0x00; // B
                        fallbackBuffer[pixelOffset + 1] = 0x00;                      // G
                        fallbackBuffer[pixelOffset + 2] = isAlternate ? 0xFF : 0x80; // R
                        fallbackBuffer[pixelOffset + 3] = 0xFF;                      // A
                    }
                }
                NSLog(@"Metal_SetFrameBuffer: Created fallback buffer %p size=%d", fallbackBuffer, fallbackSize);
            } else {
                NSLog(@"Metal_SetFrameBuffer: ERROR - Failed to allocate fallback buffer");
                return;
            }
        }
        
        // Use the fallback buffer
        buffer = fallbackBuffer;
    }
    
    // Store frame buffer info
    g_coreFrameBuffer = buffer;
    g_coreFrameWidth = width;
    g_coreFrameHeight = height;
    g_coreFramePitch = pitch;
    
    // Update internal dimensions if needed
    if (g_frameWidth != width || g_frameHeight != height || g_frameBuffer == NULL) {
        NSLog(@"Metal_SetFrameBuffer: Recreating internal frame buffer for %dx%d", width, height);
        
        g_frameWidth = width;
        g_frameHeight = height;
        
        // Free previous frame buffer
        if (g_frameBuffer) {
            free(g_frameBuffer);
            g_frameBuffer = NULL;
        }
        
        // Create our internal frame buffer
        g_frameBuffer = (uint32_t*)malloc(width * height * sizeof(uint32_t));
        if (!g_frameBuffer) {
            NSLog(@"Metal_SetFrameBuffer: ERROR - Failed to allocate internal frame buffer");
            return;
        }
        
        // Update post-processing parameters
        g_postProcessParams.textureSize[0] = width;
        g_postProcessParams.textureSize[1] = height;
        g_postProcessParams.scanlineCount = height;
        
        NSLog(@"Metal_SetFrameBuffer: Created internal frame buffer %p size=%dx%d", 
              g_frameBuffer, width, height);
    }
    
    // Trigger an initial frame update
    Metal_UpdateFrame();
}

// Update the frame buffer with new data
void Metal_UpdateFrame() {
    // Check if we have valid data
    if (!g_coreFrameBuffer || !g_frameBuffer || g_coreFrameWidth <= 0 || g_coreFrameHeight <= 0) {
        NSLog(@"Metal_UpdateFrame: Invalid frame buffer parameters - buffer=%p, internal=%p, width=%d, height=%d",
             g_coreFrameBuffer, g_frameBuffer, g_coreFrameWidth, g_coreFrameHeight);
        
        // Set a diagnostic pattern to make the issue visible
        if (g_frameBuffer && g_frameWidth > 0 && g_frameHeight > 0) {
            for (int y = 0; y < g_frameHeight; y++) {
                for (int x = 0; x < g_frameWidth; x++) {
                    // Create a red and yellow warning pattern
                    bool isAlternate = ((x / 16) + (y / 16)) % 2 == 0;
                    g_frameBuffer[y * g_frameWidth + x] = isAlternate ? 0xFFFF0000 : 0xFFFFFF00;
                }
            }
            g_frameUpdated = true;
        }
        return;
    }
    
    if (g_coreFramePitch <= 0) {
        NSLog(@"Metal_UpdateFrame: Invalid pitch (%d) - using fallback", g_coreFramePitch);
        // Use a minimal fallback pitch if none is specified
        g_coreFramePitch = g_coreFrameWidth * 4;
    }
    
    NSLog(@"Metal_UpdateFrame: Copying frame data %dx%d (pitch=%d) from %p to %p", 
          g_coreFrameWidth, g_coreFrameHeight, g_coreFramePitch, g_coreFrameBuffer, g_frameBuffer);
    
    // Copy data from core frame buffer to our internal frame buffer
    // This assumes 32-bit BGRA format in the core frame buffer
    int minPitch = g_coreFrameWidth * 4;
    if (g_coreFramePitch == minPitch) {
        // Fast path: direct copy
        memcpy(g_frameBuffer, g_coreFrameBuffer, g_coreFrameWidth * g_coreFrameHeight * 4);
    } else {
        // Slow path: copy line by line
        for (int y = 0; y < g_coreFrameHeight; y++) {
            unsigned char* srcLine = g_coreFrameBuffer + y * g_coreFramePitch;
            uint32_t* destLine = g_frameBuffer + y * g_coreFrameWidth;
            memcpy(destLine, srcLine, minPitch);
        }
    }
    
    // Mark frame as updated
    g_frameUpdated = true;
}

// Render the current frame
void Metal_RenderFrame() {
    if (g_metalView) {
        // Trigger a redraw
        [g_metalView setNeedsDisplay:YES];
    }
}

// Resize the Metal viewport
void Metal_Resize(int width, int height) {
    NSLog(@"Metal_Resize: Resizing Metal view to %dx%d", width, height);
    
    if (g_metalView) {
        // Update post-processing parameters
        g_postProcessParams.screenSize[0] = width;
        g_postProcessParams.screenSize[1] = height;
    }
}

// Toggle post-processing effects
void Metal_TogglePostProcessing(int enabled) {
    // Property must be defined in the MTKView subclass
    // This is just a stub for now
    NSLog(@"Metal_TogglePostProcessing: %s", enabled ? "enabled" : "disabled");
}

// Set scanline intensity (0.0 - 1.0)
void Metal_SetScanlineIntensity(float intensity) {
    g_postProcessParams.scanlineIntensity = intensity;
}

// Set CRT curvature (0.0 - 1.0)
void Metal_SetCRTCurvature(float curvature) {
    g_postProcessParams.curvature = curvature;
}

// Set vignette effect (0.0 - 1.0)
void Metal_SetVignetteStrength(float strength) {
    g_postProcessParams.vignetteStrength = strength;
}

// Set window title
void Metal_SetWindowTitle(const char* title) {
    if (g_window && title) {
        NSString* nsTitle = [NSString stringWithUTF8String:title];
        [g_window setTitle:nsTitle];
    }
} 