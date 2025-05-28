#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "metal_renderer_c.h"
#include "metal_declarations.h"

// Forward declare functions used in this file
int MetalRenderer_GetWidth();
int MetalRenderer_GetHeight();
int MetalRenderer_UpdateFrame(void* frameBuffer, int width, int height);

int main(int argc, char *argv[]) {
    printf("Testing Metal Renderer C Interface\n");
    
    // Try calling some metal renderer functions
    int width = MetalRenderer_GetWidth();
    int height = MetalRenderer_GetHeight();
    printf("Current frame size: %d x %d\n", width, height);
    
    // Test if we can create a sample frame buffer
    void* frameBuffer = malloc(320 * 240 * 4);
    if (frameBuffer) {
        // Initialize with a test pattern
        memset(frameBuffer, 0, 320 * 240 * 4);
        
        // Call the update function (this shouldn't crash even if it does nothing)
        MetalRenderer_UpdateFrame(frameBuffer, 320, 240);
        
        free(frameBuffer);
    }
    
    printf("Test completed\n");
    return 0;
}

// Test function for Metal renderer
int test_metal() {
    // Get current dimensions
    int width = MetalRenderer_GetWidth();
    int height = MetalRenderer_GetHeight();
    
    printf("Metal test: Current dimensions: %dx%d\n", width, height);
    
    // Create test frame buffer
    unsigned char* frameBuffer = (unsigned char*)malloc(320 * 240 * 4);
    if (frameBuffer) {
        // Fill with test pattern
        for (int y = 0; y < 240; y++) {
            for (int x = 0; x < 320; x++) {
                frameBuffer[(y * 320 + x) * 4 + 0] = x & 0xFF;       // B
                frameBuffer[(y * 320 + x) * 4 + 1] = y & 0xFF;       // G
                frameBuffer[(y * 320 + x) * 4 + 2] = (x + y) & 0xFF; // R
                frameBuffer[(y * 320 + x) * 4 + 3] = 0xFF;           // A
            }
        }
        
        // Update frame
        MetalRenderer_UpdateFrame(frameBuffer, 320, 240);
        
        // Clean up
        free(frameBuffer);
    }
    
    return 0;
} 