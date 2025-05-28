#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#include "metal_declarations.h"

// Metal device reference maintained by the main app
extern id<MTLDevice> g_metalDevice;

// Current frame texture
static id<MTLTexture> g_frameTexture = nil;
static id<MTLBuffer> g_stagingBuffer = nil;
static int g_frameWidth = 320;
static int g_frameHeight = 240;
static int g_frameStride = 320 * 4;

// Frame conversion utilities
void UpdateMetalFrameTexture(const void* frameData, unsigned int width, unsigned int height) {
    if (!frameData || width == 0 || height == 0) {
        NSLog(@"UpdateMetalFrameTexture: Invalid parameters");
        return;
    }
    
    // Check if we need to recreate the texture due to size change
    if (!g_frameTexture || g_frameWidth != width || g_frameHeight != height) {
        g_frameWidth = width;
        g_frameHeight = height;
        g_frameStride = width * 4; // Assuming 32bpp BGRA
        
        // Create a texture descriptor
        MTLTextureDescriptor* textureDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                                                                              width:width
                                                                                             height:height
                                                                                          mipmapped:NO];
        textureDesc.usage = MTLTextureUsageShaderRead;
        
        // Create the texture
        g_frameTexture = [g_metalDevice newTextureWithDescriptor:textureDesc];
        if (!g_frameTexture) {
            NSLog(@"Failed to create frame texture");
            return;
        }
        
        NSLog(@"Created new frame texture: %dx%d", width, height);
    }
    
    // Calculate buffer size
    NSUInteger bufferSize = width * height * 4;
    
    // Update texture with frame data
    [g_frameTexture replaceRegion:MTLRegionMake2D(0, 0, width, height)
                      mipmapLevel:0
                        withBytes:frameData
                      bytesPerRow:g_frameStride];
}

// Get the current frame texture
id<MTLTexture> GetCurrentFrameTexture() {
    return g_frameTexture;
}

// Helper function to set the Metal device
void SetMetalDevice(id<MTLDevice> device) {
    g_metalDevice = device;
} 