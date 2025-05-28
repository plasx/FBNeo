#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <Cocoa/Cocoa.h>
#import "metal_renderer_c.h"
#import "metal_renderer.h"

// Forward declarations
@class MetalRenderer;

// Global renderer instance
static MetalRenderer* g_renderer = nil;

// Initialize Metal renderer with a device
bool Metal_InitRenderer(id<MTLDevice> device) {
    if (!device) {
        NSLog(@"Error: No Metal device provided");
        return false;
    }
    
    @autoreleasepool {
        // Create renderer instance
        g_renderer = [[MetalRenderer alloc] initWithDevice:device];
        if (!g_renderer) {
            NSLog(@"Error: Failed to create Metal renderer");
            return false;
        }
        
        return true;
    }
}

// Initialize Metal renderer with a view
int Metal_InitRendererWithView(NSView* view) {
    if (!view) {
        NSLog(@"Error: No view provided");
        return METAL_ERROR_NO_VIEW;
    }
    
    @autoreleasepool {
        // Create renderer instance
        g_renderer = [[MetalRenderer alloc] initWithView:view];
        if (!g_renderer) {
            NSLog(@"Error: Failed to create Metal renderer");
            return METAL_ERROR_NOT_INITIALIZED;
        }
        
        return METAL_ERROR_NONE;
    }
}

// Shutdown Metal renderer
void Metal_ShutdownRenderer() {
    @autoreleasepool {
        g_renderer = nil;
    }
}

// Get frame buffer (not used in Metal implementation)
void* Metal_GetFrameBuffer() {
    return NULL;
}

// Render a frame with the provided data
int Metal_RenderFrame(void* frameData, int width, int height) {
    if (!frameData || width <= 0 || height <= 0) {
        NSLog(@"Metal_RenderFrame: Invalid parameters");
        return METAL_ERROR_TEXTURE_CREATE;
    }
    
    if (!g_renderer) {
        NSLog(@"Metal_RenderFrame: No renderer available");
        return METAL_ERROR_NOT_INITIALIZED;
    }
    
    @autoreleasepool {
        // Calculate pitch (bytes per row)
        int pitch = width * 4; // RGBA format (32-bit color)
        
        // Update texture with frame data
        [g_renderer updateTextureWithBuffer:(uint8_t*)frameData 
                                     width:width 
                                    height:height 
                                     pitch:pitch];
        return METAL_ERROR_NONE;
    }
}

// Update texture with frame data
void Metal_UpdateTexture(void* data, int width, int height) {
    Metal_RenderFrame(data, width, height);
}

// Set render state
void Metal_SetRenderState(int state, int value) {
    if (!g_renderer) return;
    
    @autoreleasepool {
        switch (state) {
            case METAL_STATE_VSYNC:
                [g_renderer setVSync:value != 0];
                break;
                
            case METAL_STATE_FILTERING:
                // Handle filtering state
                break;
                
            case METAL_STATE_CRT:
                [g_renderer setCRTEffect:value != 0];
                break;
                
            case METAL_STATE_SCANLINES:
                [g_renderer setScanlines:value != 0 intensity:0.3f];
                break;
        }
    }
}

// Get renderer info
const char* Metal_GetRendererInfo() {
    return "Metal Renderer";
}

// Run a frame
int Metal_RunFrame(int bDraw) {
    if (!g_renderer) return METAL_ERROR_NOT_INITIALIZED;
    
    @autoreleasepool {
        if (bDraw) {
            [g_renderer renderFrame];
        }
        return METAL_ERROR_NONE;
    }
}

// Get frame texture
id<MTLTexture> Metal_GetFrameTexture() {
    if (!g_renderer) return nil;
    return [g_renderer frameTexture];
}

// Render to a command buffer
void Metal_RenderToCommandBuffer(id<MTLCommandBuffer> commandBuffer) {
    if (!g_renderer || !commandBuffer) return;
    
    @autoreleasepool {
        [g_renderer renderToCommandBuffer:commandBuffer];
    }
}

// Set renderer configuration
void Metal_SetConfig(MetalRendererConfig* config) {
    if (!g_renderer || !config) return;
    
    @autoreleasepool {
        [g_renderer setConfig:config];
    }
}

// Get renderer configuration
void Metal_GetConfig(MetalRendererConfig* config) {
    if (!g_renderer || !config) return;
    
    @autoreleasepool {
        [g_renderer getConfig:config];
    }
}

// Set shader type
void Metal_SetShaderType(MetalShaderType type) {
    if (!g_renderer) return;
    
    @autoreleasepool {
        [g_renderer setShaderType:type];
    }
}

// Set aspect ratio
void Metal_SetAspectRatio(int width, int height) {
    if (!g_renderer) return;
    
    @autoreleasepool {
        [g_renderer setAspectRatio:width height:height];
    }
}

// Set preserve aspect ratio
void Metal_SetPreserveAspectRatio(bool preserve) {
    if (!g_renderer) return;
    
    @autoreleasepool {
        [g_renderer setPreserveAspectRatio:preserve];
    }
}

// Set scanline intensity
void Metal_SetScanlineIntensity(float intensity) {
    if (!g_renderer) return;
    
    @autoreleasepool {
        [g_renderer setScanlineIntensity:intensity];
    }
}

// Set CRT curvature
void Metal_SetCRTCurvature(float curvature) {
    if (!g_renderer) return;
    
    @autoreleasepool {
        [g_renderer setCRTCurvature:curvature];
    }
}

// Set sharpness
void Metal_SetSharpness(float sharpness) {
    if (!g_renderer) return;
    
    @autoreleasepool {
        [g_renderer setSharpness:sharpness];
    }
}

// Get current width
int Metal_GetWidth() {
    if (!g_renderer) return 0;
    return (int)[g_renderer textureWidth];
}

// Get current height
int Metal_GetHeight() {
    if (!g_renderer) return 0;
    return (int)[g_renderer textureHeight];
}

// Toggle fullscreen mode
void Metal_ToggleFullscreen() {
    if (!g_renderer) return;
    
    @autoreleasepool {
        [g_renderer toggleFullscreen];
    }
} 