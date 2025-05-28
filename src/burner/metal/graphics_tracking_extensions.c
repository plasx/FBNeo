#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "metal_declarations.h"
#include "graphics_tracking.h"

// Define the frame buffer structure
EmulatorFrameBuffer g_frameBuffer = {0};

// Track graphics assets
#define MAX_ASSETS 256
static GraphicsAsset g_assets[MAX_ASSETS];
static int g_assetCount = 0;

// Initialize graphics tracking components
void GraphicsTracker_Init() {
    // Reset asset tracking
    memset(g_assets, 0, sizeof(g_assets));
    g_assetCount = 0;
    
    METAL_LOG_INFO("Graphics asset tracker initialized");
}

// Register a graphics asset and return its ID
int GraphicsTracker_RegisterAsset(const char* name, int width, int height, int bpp, int size, UINT8* data) {
    if (g_assetCount >= MAX_ASSETS) {
        METAL_LOG_ERROR("Maximum number of graphics assets reached");
        return -1;
    }
    
    int id = g_assetCount++;
    GraphicsAsset* asset = &g_assets[id];
    
    // Initialize asset info
    asset->name = name ? strdup(name) : strdup("unnamed");
    asset->width = width;
    asset->height = height;
    asset->bpp = bpp;
    asset->size = size;
    asset->isDecoded = 0;
    
    // Calculate memory usage
    asset->memoryUsage = (width * height * bpp) / 8;
    
    // Calculate CRC if data is available
    if (data && size > 0) {
        // Simple CRC calculation
        UINT32 crc = 0;
        for (int i = 0; i < size; i++) {
            crc = (crc + data[i]) % 0xFFFFFFFF;
        }
        asset->crc = crc;
    } else {
        asset->crc = 0;
    }
    
    METAL_LOG_DEBUG("Registered graphics asset: %s (%dx%d, %d bpp)", 
                  asset->name, width, height, bpp);
    
    return id;
}

// Track sprite rendering
void GraphicsTracker_TrackRendering(int spriteCount, int renderedCount) {
    // Log rendering statistics
    if (Metal_IsDebugMode()) {
        static int frameCount = 0;
        frameCount++;
        
        // Log every 60 frames to avoid spam
        if (frameCount % 60 == 0) {
            float renderRatio = (spriteCount > 0) ? ((float)renderedCount / spriteCount) : 0.0f;
            METAL_LOG_DEBUG("Rendered %d/%d sprites (%.1f%%)", 
                          renderedCount, spriteCount, renderRatio * 100.0f);
        }
    }
}

// Get info about a specific asset
GraphicsAsset* GraphicsTracker_GetAsset(int assetId) {
    if (assetId < 0 || assetId >= g_assetCount) {
        return NULL;
    }
    
    return &g_assets[assetId];
}

// Get total graphics memory usage
int GraphicsTracker_GetTotalMemoryUsage() {
    int total = 0;
    
    for (int i = 0; i < g_assetCount; i++) {
        total += g_assets[i].memoryUsage;
    }
    
    return total;
}

// Log info about all graphics assets
void GraphicsTracker_LogAssets() {
    METAL_LOG_INFO("Graphics assets (%d total):", g_assetCount);
    
    for (int i = 0; i < g_assetCount; i++) {
        GraphicsAsset* asset = &g_assets[i];
        METAL_LOG_INFO("  [%d] %s: %dx%d, %d bpp, %d bytes, CRC32: 0x%08X", 
                     i, asset->name, asset->width, asset->height, 
                     asset->bpp, asset->size, asset->crc);
    }
    
    METAL_LOG_INFO("Total graphics memory usage: %d bytes", GraphicsTracker_GetTotalMemoryUsage());
}

// Track the decoding of a graphics asset
void GraphicsTracker_TrackDecoding(int assetId, int success) {
    if (assetId < 0 || assetId >= g_assetCount) {
        return;
    }
    
    g_assets[assetId].isDecoded = success;
    
    if (Metal_IsDebugMode()) {
        if (success) {
            METAL_LOG_DEBUG("Successfully decoded asset: %s", g_assets[assetId].name);
        } else {
            METAL_LOG_ERROR("Failed to decode asset: %s", g_assets[assetId].name);
        }
    }
}

// Rendering statistics tracking
static struct {
    int framesRendered;
    int spritesRendered;
    int totalSprites;
    int vertexCount;
    int drawCalls;
} g_renderStats = {0};

// Reset render statistics
void GraphicsTracker_ResetStats() {
    g_renderStats.framesRendered = 0;
    g_renderStats.spritesRendered = 0;
    g_renderStats.totalSprites = 0;
    g_renderStats.vertexCount = 0;
    g_renderStats.drawCalls = 0;
}

// Get render statistics
void GraphicsTracker_GetStats(int* framesRendered, int* spritesRendered, int* totalSprites, 
                            int* vertexCount, int* drawCalls) {
    if (framesRendered) *framesRendered = g_renderStats.framesRendered;
    if (spritesRendered) *spritesRendered = g_renderStats.spritesRendered;
    if (totalSprites) *totalSprites = g_renderStats.totalSprites;
    if (vertexCount) *vertexCount = g_renderStats.vertexCount;
    if (drawCalls) *drawCalls = g_renderStats.drawCalls;
}

// Used by the renderer to track a frame being rendered
void GraphicsTracker_TrackFrame(int spriteCount, int drawCalls, int vertexCount) {
    g_renderStats.framesRendered++;
    g_renderStats.spritesRendered += spriteCount;
    g_renderStats.totalSprites += spriteCount;
    g_renderStats.drawCalls += drawCalls;
    g_renderStats.vertexCount += vertexCount;
    
    // Periodically log stats in debug mode
    if (Metal_IsDebugMode() && g_renderStats.framesRendered % 600 == 0) {
        METAL_LOG_DEBUG("Render stats: %d frames, %.1f sprites/frame, %.1f draw calls/frame",
                      g_renderStats.framesRendered,
                      (float)g_renderStats.spritesRendered / g_renderStats.framesRendered,
                      (float)g_renderStats.drawCalls / g_renderStats.framesRendered);
    }
} 