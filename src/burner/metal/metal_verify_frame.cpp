#include "metal_declarations.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Frame verification tool for Metal port
// This utility helps diagnose frame buffer and texture update issues

// Forward declarations
void Metal_VerifyRenderPipeline(int width, int height);
void Metal_AnalyzeFrameBuffer(const void* buffer, int width, int height, int bpp);
extern void UpdateMetalFrameTexture(const void* frameData, unsigned int width, unsigned int height);

// Pixel format conversion utilities
static unsigned int RGBA_to_BGRA(unsigned int rgba) {
    // Convert RGBA to BGRA
    return ((rgba & 0xFF000000) |           // Alpha (unchanged)
            ((rgba & 0x00FF0000) >> 16) |   // Red -> Blue
            (rgba & 0x0000FF00) |           // Green (unchanged)
            ((rgba & 0x000000FF) << 16));   // Blue -> Red
}

// Verify the rendering pipeline by sending test patterns
void Metal_VerifyRenderPipeline(int width, int height) {
    printf("\n=== Metal Render Pipeline Verification ===\n");
    
    // Use standard dimensions if none provided
    if (width <= 0) width = 320;
    if (height <= 0) height = 240;
    
    printf("Using dimensions: %dx%d\n", width, height);
    
    // Step 1: Create a test frame buffer
    int bpp = 32;  // Always use 32bpp for Metal
    int bytesPerPixel = bpp / 8;
    int bufferSize = width * height * bytesPerPixel;
    
    unsigned char* testBuffer = (unsigned char*)malloc(bufferSize);
    if (!testBuffer) {
        printf("ERROR: Failed to allocate test buffer\n");
        return;
    }
    
    // Clear the buffer first
    memset(testBuffer, 0, bufferSize);
    
    // Step 2: Fill with a recognizable pattern (vertical color bars)
    unsigned int* pixels = (unsigned int*)testBuffer;
    int numBars = 8;
    int barWidth = width / numBars;
    
    // Color bars (RGBA format for clear identification)
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
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int barIndex = x / barWidth;
            if (barIndex >= numBars) barIndex = numBars - 1;
            
            // Set the pixel color
            pixels[y * width + x] = colors[barIndex];
        }
    }
    
    // Step 3: Add a checksum pattern in the corner for verification
    // This helps identify if the pattern was correctly transmitted
    unsigned int checksum = 0;
    for (int i = 0; i < 1000 && i < bufferSize / 4; i++) {
        checksum += pixels[i];
    }
    
    // Draw a small verification box in the corner
    for (int y = 0; y < 16 && y < height; y++) {
        for (int x = 0; x < 16 && x < width; x++) {
            if (x == 0 || x == 15 || y == 0 || y == 15) {
                pixels[y * width + x] = 0xFF00FF00; // Green border
            } else if (x == y) {
                pixels[y * width + x] = 0xFFFF0000; // Red diagonal
            } else if (x == 8 || y == 8) {
                pixels[y * width + x] = 0xFF0000FF; // Blue crosshair
            }
        }
    }
    
    // Step 4: Analyze the frame buffer content
    printf("\nOriginal frame buffer analysis:\n");
    Metal_AnalyzeFrameBuffer(testBuffer, width, height, bpp);
    
    // Step 5: Update the Metal texture with the test frame
    printf("\nUpdating Metal texture with test pattern...\n");
    UpdateMetalFrameTexture(testBuffer, width, height);
    
    // Step 6: Generate a series of test patterns using the built-in function
    printf("\nGenerating built-in test patterns...\n");
    for (int i = 0; i < 4; i++) {
        printf("Test pattern %d...\n", i);
        Metal_GenerateTestPattern(i);
        // In a real app, we would add a delay between patterns
    }
    
    // Cleanup
    free(testBuffer);
    printf("=== Verification complete ===\n\n");
}

// Analyze frame buffer content
void Metal_AnalyzeFrameBuffer(const void* buffer, int width, int height, int bpp) {
    if (!buffer || width <= 0 || height <= 0) {
        printf("Invalid buffer or dimensions\n");
        return;
    }
    
    int bytesPerPixel = bpp / 8;
    unsigned int totalBytes = width * height * bytesPerPixel;
    const unsigned char* byteBuffer = (const unsigned char*)buffer;
    
    // Basic statistics
    unsigned int nonZeroBytes = 0;
    unsigned int checksum = 0;
    unsigned int maxValue = 0;
    
    // Scan the first 10KB or the entire buffer, whichever is smaller
    unsigned int scanSize = totalBytes < 10240 ? totalBytes : 10240;
    
    for (unsigned int i = 0; i < scanSize; i++) {
        if (byteBuffer[i] != 0) {
            nonZeroBytes++;
            if (byteBuffer[i] > maxValue) maxValue = byteBuffer[i];
        }
        checksum += byteBuffer[i];
    }
    
    printf("Buffer analysis (%dx%d, %d bpp):\n", width, height, bpp);
    printf("  Total size: %u bytes\n", totalBytes);
    printf("  Non-zero bytes: %u/%u (%.1f%%)\n", 
           nonZeroBytes, scanSize, (nonZeroBytes * 100.0f / scanSize));
    printf("  Checksum (first 10KB): 0x%08X\n", checksum);
    printf("  Max byte value: 0x%02X\n", maxValue);
    
    // Sample some pixels if 32bpp
    if (bpp == 32) {
        const unsigned int* pixels = (const unsigned int*)buffer;
        printf("  Sample pixels (RGBA format):\n");
        
        // Calculate total number of pixels
        unsigned int totalPixels = totalBytes / 4;
        
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
}

// External entry point for verification tool
extern "C" {
    int Metal_VerifyFramePipeline(int width, int height) {
        printf("Starting Metal frame pipeline verification...\n");
        Metal_VerifyRenderPipeline(width, height);
        return 0;
    }
} 