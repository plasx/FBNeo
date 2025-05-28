#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "../metal_renderer_c.h"
#include "../metal_declarations.h"

// Frame buffer variables
static void* g_frameBuffer = NULL;
static int g_frameWidth = 384;
static int g_frameHeight = 224;
static int g_frameBpp = 4;

// Initialize Metal renderer
int Metal_InitRenderer(int width, int height, int bpp) {
    printf("Metal_InitRenderer(%d, %d, %d) called\n", width, height, bpp);
    g_frameWidth = width;
    g_frameHeight = height;
    g_frameBpp = bpp;
    
    // Allocate frame buffer
    if (g_frameBuffer) {
        free(g_frameBuffer);
    }
    
    g_frameBuffer = malloc(width * height * bpp);
    if (!g_frameBuffer) {
        printf("Failed to allocate frame buffer\n");
        return 1;
    }
    
    // Clear buffer
    memset(g_frameBuffer, 0, width * height * bpp);
    
    printf("Metal renderer initialized\n");
    return 0;
}

// Shutdown Metal renderer
void Metal_ShutdownRenderer() {
    printf("Metal_ShutdownRenderer() called\n");
    if (g_frameBuffer) {
        free(g_frameBuffer);
        g_frameBuffer = NULL;
    }
    printf("Metal renderer shut down\n");
}

// Get frame buffer pointer
void* Metal_GetFrameBuffer() {
    printf("Metal_GetFrameBuffer() called\n");
    return g_frameBuffer;
}

// Set a pixel in the frame buffer
void Metal_SetPixel(int x, int y, unsigned int color) {
    if (!g_frameBuffer || x < 0 || y < 0 || x >= g_frameWidth || y >= g_frameHeight) {
        return;
    }
    
    uint8_t* pixel = (uint8_t*)g_frameBuffer + (y * g_frameWidth + x) * g_frameBpp;
    
    if (g_frameBpp == 4) {
        *(uint32_t*)pixel = color;
    } else if (g_frameBpp == 3) {
        pixel[0] = (color >> 16) & 0xFF;
        pixel[1] = (color >> 8) & 0xFF;
        pixel[2] = color & 0xFF;
    } else if (g_frameBpp == 2) {
        *(uint16_t*)pixel = (uint16_t)color;
    }
}

// Clear the frame buffer
void Metal_ClearFrameBuffer(unsigned int color) {
    printf("Metal_ClearFrameBuffer(0x%08X) called\n", color);
    if (!g_frameBuffer) {
        return;
    }
    
    if (g_frameBpp == 4) {
        uint32_t* buffer = (uint32_t*)g_frameBuffer;
        for (int i = 0; i < g_frameWidth * g_frameHeight; i++) {
            buffer[i] = color;
        }
    } else if (g_frameBpp == 3) {
        uint8_t* buffer = (uint8_t*)g_frameBuffer;
        uint8_t r = (color >> 16) & 0xFF;
        uint8_t g = (color >> 8) & 0xFF;
        uint8_t b = color & 0xFF;
        
        for (int i = 0; i < g_frameWidth * g_frameHeight; i++) {
            buffer[i*3+0] = r;
            buffer[i*3+1] = g;
            buffer[i*3+2] = b;
        }
    } else if (g_frameBpp == 2) {
        uint16_t* buffer = (uint16_t*)g_frameBuffer;
        uint16_t value = (uint16_t)color;
        
        for (int i = 0; i < g_frameWidth * g_frameHeight; i++) {
            buffer[i] = value;
        }
    }
}

// Update texture with frame buffer data
int Metal_UpdateTexture(void* data, int width, int height, int pitch) {
    printf("Metal_UpdateTexture(%p, %d, %d, %d) called\n", data, width, height, pitch);
    
    if (!data || width <= 0 || height <= 0 || pitch <= 0) {
        return 1;
    }
    
    // If we don't have a frame buffer yet, create one
    if (!g_frameBuffer) {
        g_frameWidth = width;
        g_frameHeight = height;
        g_frameBpp = 4; // Assume RGBA
        
        g_frameBuffer = malloc(width * height * g_frameBpp);
        if (!g_frameBuffer) {
            return 1;
        }
    }
    
    // Update frame buffer with new data
    if (data) {
        const uint8_t* frameData = (const uint8_t*)data;
        uint8_t* dst = (uint8_t*)g_frameBuffer;
        
        if (width == g_frameWidth && height == g_frameHeight && pitch == width * g_frameBpp) {
            // Direct copy since dimensions match
            memcpy(g_frameBuffer, frameData, pitch * height);
        } else {
            // Copy with stride adjustment
            int minWidth = (width < g_frameWidth) ? width : g_frameWidth;
            int minHeight = (height < g_frameHeight) ? height : g_frameHeight;
            int bytesPerPixel = g_frameBpp;
            
            for (int y = 0; y < minHeight; y++) {
                memcpy(dst + (y * g_frameWidth * bytesPerPixel), 
                      frameData + (y * pitch), 
                      minWidth * bytesPerPixel);
            }
        }
    }
    
    return 0;
}

// Set render states
void Metal_SetRenderState(int state, int value) {
    printf("Metal_SetRenderState(%d, %d) called\n", state, value);
}

// Get renderer info
const char* Metal_GetRendererInfo() {
    return "Metal Renderer (C Stub)";
}

// Export frame buffer as RGBA8 data
void* Metal_GetFrameBufferData(int* width, int* height) {
    if (width) *width = g_frameWidth;
    if (height) *height = g_frameHeight;
    printf("Metal_GetFrameBufferData() called\n");
    
    // If we already have an RGBA buffer, just return it
    if (g_frameBpp == 4) {
        return g_frameBuffer;
    }
    
    // Otherwise, we need to convert to RGBA
    void* rgbaBuffer = malloc(g_frameWidth * g_frameHeight * 4);
    if (!rgbaBuffer) {
        return NULL;
    }
    
    // Convert from current format to RGBA
    if (g_frameBpp == 3) {
        uint8_t* src = (uint8_t*)g_frameBuffer;
        uint32_t* dst = (uint32_t*)rgbaBuffer;
        
        for (int i = 0; i < g_frameWidth * g_frameHeight; i++) {
            uint8_t r = src[i*3+0];
            uint8_t g = src[i*3+1];
            uint8_t b = src[i*3+2];
            dst[i] = (0xFF << 24) | (r << 16) | (g << 8) | b;
        }
    } else if (g_frameBpp == 2) {
        uint16_t* src = (uint16_t*)g_frameBuffer;
        uint32_t* dst = (uint32_t*)rgbaBuffer;
        
        for (int i = 0; i < g_frameWidth * g_frameHeight; i++) {
            uint16_t pixel = src[i];
            uint8_t r = ((pixel >> 11) & 0x1F) << 3;
            uint8_t g = ((pixel >> 5) & 0x3F) << 2;
            uint8_t b = (pixel & 0x1F) << 3;
            dst[i] = (0xFF << 24) | (r << 16) | (g << 8) | b;
        }
    }
    
    return rgbaBuffer;
}

// Free frame buffer data allocated by Metal_GetFrameBufferData
void Metal_FreeFrameBufferData(void* data) {
    printf("Metal_FreeFrameBufferData() called\n");
    if (data && data != g_frameBuffer) {
        free(data);
    }
}

// Debug and testing functions
int Metal_ShowTestPattern(int width, int height) {
    printf("Metal_ShowTestPattern(%d, %d) called\n", width, height);
    return 0;
}

int Metal_ShowDebugInfo(int enabled) {
    printf("Metal_ShowDebugInfo(%d) called\n", enabled);
    return 0;
}

int Metal_ToggleFullscreen(int enabled) {
    printf("Metal_ToggleFullscreen(%d) called\n", enabled);
    return 0;
}

int Metal_SetScreenshot(const char* path) {
    printf("Metal_SetScreenshot(%s) called\n", path ? path : "NULL");
    return 0;
}

int Metal_GetScreenshot(const char* path) {
    printf("Metal_GetScreenshot(%s) called\n", path ? path : "NULL");
    return 0;
}

int Metal_GetDeviceInfo(char* buffer, int bufferSize) {
    if (buffer && bufferSize > 0) {
        snprintf(buffer, bufferSize, "Apple Metal Device");
    }
    return 0;
}

// Helper functions
int Metal_ConvertRGBtoYUV(void* rgbData, void* yuvData, int width, int height, int format) {
    printf("Metal_ConvertRGBtoYUV() called\n");
    return 0;
}

int Metal_ConvertYUVtoRGB(void* yuvData, void* rgbData, int width, int height, int format) {
    printf("Metal_ConvertYUVtoRGB() called\n");
    return 0;
}

int Metal_GetAvailableShaders(char** shaderNames, int maxShaders) {
    printf("Metal_GetAvailableShaders() called\n");
    return 0;
}

int Metal_LoadCustomShader(const char* path) {
    printf("Metal_LoadCustomShader(%s) called\n", path ? path : "NULL");
    return 0;
}

// Frame dimension functions
int Metal_GetFrameWidth(void) {
    // Default fallback width
    return 320;
}

int Metal_GetFrameHeight(void) {
    // Default fallback height
    return 240;
} 