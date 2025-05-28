#include "metal_declarations.h"
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Debug application for FBNeo Metal port
// This is a standalone tool for testing the frame pipeline

// Frame buffer variables
static UINT8* g_pFrameBuffer = NULL;
static int g_nFrameWidth = 320;
static int g_nFrameHeight = 240;
static int g_nFrameBpp = 32;
static int g_nFrameSize = 0;

// Metal implementation
static id<MTLDevice> g_device = nil;
static id<MTLTexture> g_texture = nil;
static id<MTLCommandQueue> g_commandQueue = nil;

// Forward declarations
void DebugGenerateTestPattern(int patternType);
void DebugAnalyzeFrameBuffer();
void DebugUpdateTexture();

// Initialize Metal 
bool InitializeMetal() {
    // Get default Metal device
    g_device = MTLCreateSystemDefaultDevice();
    if (!g_device) {
        printf("ERROR: Failed to create Metal device\n");
        return false;
    }
    
    printf("Metal device: %s\n", [g_device.name UTF8String]);
    
    // Create command queue
    g_commandQueue = [g_device newCommandQueue];
    if (!g_commandQueue) {
        printf("ERROR: Failed to create command queue\n");
        return false;
    }
    
    // Create texture descriptor
    MTLTextureDescriptor *textureDesc = [MTLTextureDescriptor 
                                          texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                                          width:g_nFrameWidth
                                          height:g_nFrameHeight
                                          mipmapped:NO];
    
    // Set up texture for optimal performance
    textureDesc.usage = MTLTextureUsageShaderRead | MTLTextureUsageRenderTarget;
    if ([g_device hasUnifiedMemory]) {
        textureDesc.storageMode = MTLStorageModeShared;
    } else {
        textureDesc.storageMode = MTLStorageModePrivate;
    }
    
    // Create texture
    g_texture = [g_device newTextureWithDescriptor:textureDesc];
    if (!g_texture) {
        printf("ERROR: Failed to create texture\n");
        return false;
    }
    
    printf("Metal initialized successfully\n");
    return true;
}

// Initialize frame buffer
bool InitializeFrameBuffer(int width, int height, int bpp) {
    // Free existing buffer if any
    if (g_pFrameBuffer) {
        free(g_pFrameBuffer);
        g_pFrameBuffer = NULL;
    }
    
    // Calculate new size
    int bytesPerPixel = bpp / 8;
    g_nFrameSize = width * height * bytesPerPixel;
    
    // Allocate new buffer
    g_pFrameBuffer = (UINT8*)malloc(g_nFrameSize);
    if (!g_pFrameBuffer) {
        printf("ERROR: Failed to allocate frame buffer of size %d bytes\n", g_nFrameSize);
        return false;
    }
    
    // Clear the buffer
    memset(g_pFrameBuffer, 0, g_nFrameSize);
    
    // Update dimensions
    g_nFrameWidth = width;
    g_nFrameHeight = height;
    g_nFrameBpp = bpp;
    
    printf("Initialized frame buffer: %dx%d, %d bpp (%d bytes)\n", 
           width, height, bpp, g_nFrameSize);
    
    return true;
}

// Generate test pattern
void DebugGenerateTestPattern(int patternType) {
    if (!g_pFrameBuffer) {
        printf("ERROR: No frame buffer available for test pattern\n");
        return;
    }
    
    printf("Generating test pattern (type %d) for %dx%d frame buffer\n", 
           patternType, g_nFrameWidth, g_nFrameHeight);
    
    // Cast to 32-bit pixel buffer (RGBA/BGRA format)
    unsigned int* pixels = (unsigned int*)g_pFrameBuffer;
    
    // Static counter for animation effects
    static int animFrame = 0;
    animFrame = (animFrame + 1) % 60;
    
    // Clear buffer first
    memset(g_pFrameBuffer, 0, g_nFrameSize);
    
    switch (patternType) {
        case 0: { // Color bars
            int numBars = 8;
            int barWidth = g_nFrameWidth / numBars;
            
            // Standard color bar colors (RGBA format)
            unsigned int colors[8] = {
                0xFFFFFFFF, // White
                0xFFFFFF00, // Yellow
                0xFF00FFFF, // Cyan
                0xFF00FF00, // Green
                0xFFFF00FF, // Magenta
                0xFFFF0000, // Red
                0xFF0000FF, // Blue
                0xFF000000  // Black
            };
            
            for (int y = 0; y < g_nFrameHeight; y++) {
                for (int x = 0; x < g_nFrameWidth; x++) {
                    int barIndex = x / barWidth;
                    if (barIndex >= numBars) barIndex = numBars - 1;
                    pixels[y * g_nFrameWidth + x] = colors[barIndex];
                }
            }
            break;
        }
        
        case 1: { // Gradient pattern
            for (int y = 0; y < g_nFrameHeight; y++) {
                for (int x = 0; x < g_nFrameWidth; x++) {
                    unsigned char r = (unsigned char)((x * 255) / g_nFrameWidth);
                    unsigned char g = (unsigned char)((y * 255) / g_nFrameHeight);
                    unsigned char b = (unsigned char)(((x + y) * 255) / (g_nFrameWidth + g_nFrameHeight));
                    
                    // RGBA format
                    pixels[y * g_nFrameWidth + x] = (0xFF << 24) | (r << 16) | (g << 8) | b;
                }
            }
            break;
        }
        
        case 2: { // Animated checkerboard
            int blockSize = 32;
            for (int y = 0; y < g_nFrameHeight; y++) {
                for (int x = 0; x < g_nFrameWidth; x++) {
                    if (((x / blockSize) + (y / blockSize) + (animFrame / 15)) % 2 == 0) {
                        pixels[y * g_nFrameWidth + x] = 0xFFFFFFFF; // White
                    } else {
                        pixels[y * g_nFrameWidth + x] = 0xFF000000; // Black
                    }
                }
            }
            break;
        }
    }
    
    // Analyze and update texture
    DebugAnalyzeFrameBuffer();
    DebugUpdateTexture();
}

// Analyze frame buffer content
void DebugAnalyzeFrameBuffer() {
    if (!g_pFrameBuffer) {
        printf("ERROR: No frame buffer to analyze\n");
        return;
    }
    
    printf("\n=== FRAME BUFFER ANALYSIS ===\n");
    printf("Buffer: %p, Size: %d bytes, Dimensions: %dx%d, BPP: %d\n", 
           g_pFrameBuffer, g_nFrameSize, g_nFrameWidth, g_nFrameHeight, g_nFrameBpp);
    
    // Basic statistics
    unsigned int nonZeroBytes = 0;
    unsigned int checksum = 0;
    unsigned int maxValue = 0;
    
    // Scan the first 10KB or the entire buffer, whichever is smaller
    unsigned int scanSize = g_nFrameSize < 10240 ? g_nFrameSize : 10240;
    
    for (unsigned int i = 0; i < scanSize; i++) {
        if (g_pFrameBuffer[i] != 0) {
            nonZeroBytes++;
            if (g_pFrameBuffer[i] > maxValue) maxValue = g_pFrameBuffer[i];
        }
        checksum += g_pFrameBuffer[i];
    }
    
    printf("  Non-zero bytes: %u/%u (%.1f%%)\n", 
           nonZeroBytes, scanSize, (nonZeroBytes * 100.0f / scanSize));
    printf("  Checksum (first 10KB): 0x%08X\n", checksum);
    printf("  Max byte value: 0x%02X\n", maxValue);
    
    // Sample some pixels if 32bpp
    if (g_nFrameBpp == 32) {
        const unsigned int* pixels = (const unsigned int*)g_pFrameBuffer;
        printf("  Sample pixels (RGBA format):\n");
        
        unsigned int totalPixels = g_nFrameSize / 4;
        
        for (int i = 0; i < 5 && i < totalPixels; i++) {
            unsigned int pixel = pixels[i];
            unsigned char r = (pixel >> 16) & 0xFF;
            unsigned char g = (pixel >> 8) & 0xFF;
            unsigned char b = pixel & 0xFF;
            unsigned char a = (pixel >> 24) & 0xFF;
            
            printf("    Pixel %d: [R:%02X,G:%02X,B:%02X,A:%02X] = 0x%08X\n", 
                   i, r, g, b, a, pixel);
        }
    }
    
    printf("==============================\n\n");
}

// Update texture with frame buffer content
void DebugUpdateTexture() {
    if (!g_pFrameBuffer || !g_texture) {
        printf("ERROR: Cannot update texture (buffer or texture not available)\n");
        return;
    }
    
    printf("Updating Metal texture with frame buffer content\n");
    
    // Check if texture dimensions match
    if (g_texture.width != g_nFrameWidth || g_texture.height != g_nFrameHeight) {
        printf("Texture dimensions (%lux%lu) don't match frame buffer (%dx%d) - recreating\n", 
               g_texture.width, g_texture.height, g_nFrameWidth, g_nFrameHeight);
        
        // Recreate texture with correct dimensions
        MTLTextureDescriptor *textureDesc = [MTLTextureDescriptor 
                                             texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                                             width:g_nFrameWidth
                                             height:g_nFrameHeight
                                             mipmapped:NO];
        
        // Set up texture for optimal performance
        textureDesc.usage = MTLTextureUsageShaderRead | MTLTextureUsageRenderTarget;
        if ([g_device hasUnifiedMemory]) {
            textureDesc.storageMode = MTLStorageModeShared;
        } else {
            textureDesc.storageMode = MTLStorageModePrivate;
        }
        
        // Create new texture
        g_texture = [g_device newTextureWithDescriptor:textureDesc];
        if (!g_texture) {
            printf("ERROR: Failed to create new texture\n");
            return;
        }
    }
    
    // Update the texture with the provided data
    MTLRegion region = MTLRegionMake2D(0, 0, g_nFrameWidth, g_nFrameHeight);
    
    // Use proper storage mode for update
    if (g_texture.storageMode == MTLStorageModePrivate) {
        printf("Using blit encoder for discrete GPU\n");
        
        // For discrete GPUs, we need to use a buffer and blit
        id<MTLBuffer> stagingBuffer = [g_device newBufferWithBytes:g_pFrameBuffer
                                                           length:g_nFrameSize
                                                          options:MTLResourceStorageModeShared];
        
        id<MTLCommandBuffer> commandBuffer = [g_commandQueue commandBuffer];
        id<MTLBlitCommandEncoder> blitEncoder = [commandBuffer blitCommandEncoder];
        
        [blitEncoder copyFromBuffer:stagingBuffer
                       sourceOffset:0
                  sourceBytesPerRow:g_nFrameWidth * 4
                sourceBytesPerImage:g_nFrameSize
                         sourceSize:MTLSizeMake(g_nFrameWidth, g_nFrameHeight, 1)
                          toTexture:g_texture
                   destinationSlice:0
                   destinationLevel:0
                  destinationOrigin:MTLOriginMake(0, 0, 0)];
        
        [blitEncoder endEncoding];
        [commandBuffer commit];
        [commandBuffer waitUntilCompleted];
    } else {
        printf("Using direct texture update for unified memory\n");
        
        // For unified memory (Apple Silicon), we can update directly
        [g_texture replaceRegion:region
                     mipmapLevel:0
                       withBytes:g_pFrameBuffer
                     bytesPerRow:g_nFrameWidth * 4];
    }
    
    printf("Texture updated successfully\n");
}

// Save frame buffer to a file for inspection
void DebugSaveFrameBuffer(const char* filename) {
    if (!g_pFrameBuffer) {
        printf("ERROR: No frame buffer to save\n");
        return;
    }
    
    FILE* file = fopen(filename, "wb");
    if (!file) {
        printf("ERROR: Failed to open file for writing: %s\n", filename);
        return;
    }
    
    // Write a simple header with dimensions
    fprintf(file, "FBNeo Debug Frame: %dx%d %dbpp\n", g_nFrameWidth, g_nFrameHeight, g_nFrameBpp);
    
    // Write the raw buffer data
    fwrite(g_pFrameBuffer, 1, g_nFrameSize, file);
    
    fclose(file);
    printf("Frame buffer saved to: %s\n", filename);
}

// Main entry point for test app
int main(int argc, char* argv[]) {
    printf("FBNeo Metal Frame Debug Tool\n");
    printf("---------------------------\n");
    
    // Initialize Metal
    if (!InitializeMetal()) {
        printf("ERROR: Failed to initialize Metal\n");
        return 1;
    }
    
    // Initialize frame buffer
    int width = 320;
    int height = 240;
    int bpp = 32;
    
    // Parse command line arguments
    if (argc > 2) {
        width = atoi(argv[1]);
        height = atoi(argv[2]);
    }
    
    if (!InitializeFrameBuffer(width, height, bpp)) {
        printf("ERROR: Failed to initialize frame buffer\n");
        return 1;
    }
    
    // Generate and test different patterns
    printf("\nTesting pattern 0 (Color bars)...\n");
    DebugGenerateTestPattern(0);
    
    printf("\nTesting pattern 1 (Gradient)...\n");
    DebugGenerateTestPattern(1);
    
    printf("\nTesting pattern 2 (Checkerboard)...\n");
    DebugGenerateTestPattern(2);
    
    // Save the last pattern for inspection
    DebugSaveFrameBuffer("frame_debug.bin");
    
    // Clean up
    if (g_pFrameBuffer) {
        free(g_pFrameBuffer);
        g_pFrameBuffer = NULL;
    }
    
    printf("\nTest completed successfully\n");
    return 0;
} 