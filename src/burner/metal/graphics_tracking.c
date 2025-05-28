#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "graphics_tracking.h"
#include "metal_declarations.h" // Include for EmulatorFrameBuffer definition

// Simple graphics tracking for CPS2 emulation

// Graphics state
static int g_gfxInitialized = 0;
static int g_screenWidth = 384;      // CPS2 standard width
static int g_screenHeight = 224;     // CPS2 standard height
static int g_frameCounter = 0;
// Don't redefine g_frameBuffer - it's declared externally in metal_declarations.h

// Palette information
static unsigned int g_palette[256];
static int g_paletteUpdated = 0;

// Layer visibility
static int g_layerEnabled[4] = {1, 1, 1, 1};  // All layers enabled by default

// Initialize graphics tracking
void Graphics_Init(int width, int height) {
    g_gfxInitialized = 1;
    g_screenWidth = width;
    g_screenHeight = height;
    g_frameCounter = 0;
    
    // Initialize the global frame buffer
    g_frameBuffer.width = width;
    g_frameBuffer.height = height;
    g_frameBuffer.updated = false;
    
    // Clear palette
    memset(g_palette, 0, sizeof(g_palette));
    g_paletteUpdated = 1;
    
    // Enable all layers
    for (int i = 0; i < 4; i++) {
        g_layerEnabled[i] = 1;
    }
    
    printf("[GFX] Graphics initialized at %dx%d\n", width, height);
}

// Set frame buffer data pointer
void Graphics_SetFrameBuffer(unsigned char* buffer) {
    // Cast to the correct type assuming the buffer is RGBA
    g_frameBuffer.data = (uint32_t*)buffer;
    g_frameBuffer.updated = true;
}

// Get frame buffer
uint32_t* Graphics_GetFrameBuffer() {
    return g_frameBuffer.data;
}

// Track a frame being rendered
void Graphics_FrameRendered() {
    if (g_gfxInitialized) {
        g_frameCounter++;
        g_frameBuffer.updated = true;
        if (g_frameCounter % 60 == 0) {
            printf("[GFX] Frame %d rendered\n", g_frameCounter);
        }
    }
}

// Update palette entry
void Graphics_UpdatePalette(int index, unsigned int color) {
    if (index >= 0 && index < 256) {
        g_palette[index] = color;
        g_paletteUpdated = 1;
    }
}

// Set layer visibility
void Graphics_SetLayerEnabled(int layer, int enabled) {
    if (layer >= 0 && layer < 4) {
        g_layerEnabled[layer] = enabled;
    }
}

// Get layer visibility
int Graphics_GetLayerEnabled(int layer) {
    if (layer >= 0 && layer < 4) {
        return g_layerEnabled[layer];
    }
    return 0;
}

// Get screen width
int Graphics_GetScreenWidth() {
    return g_screenWidth;
}

// Get screen height
int Graphics_GetScreenHeight() {
    return g_screenHeight;
}

// Cleanup graphics tracking
void Graphics_Exit() {
    g_gfxInitialized = 0;
    g_frameBuffer.data = NULL;
    g_frameBuffer.updated = false;
    printf("[GFX] Graphics shutdown\n");
}

// Print graphics status
void Graphics_PrintStatus() {
    if (g_gfxInitialized) {
        printf("[GFX] Status: %dx%d, %d frames rendered\n", 
               g_screenWidth, g_screenHeight, g_frameCounter);
        printf("[GFX] Layers: %s %s %s %s\n",
               g_layerEnabled[0] ? "ON" : "OFF",
               g_layerEnabled[1] ? "ON" : "OFF",
               g_layerEnabled[2] ? "ON" : "OFF",
               g_layerEnabled[3] ? "ON" : "OFF");
    }
} 