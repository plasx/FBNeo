#ifndef GRAPHICS_TRACKING_H
#define GRAPHICS_TRACKING_H

#include <stdint.h> // Add for uint32_t
#include "metal_declarations.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char* name;          // Name of graphics asset
    int width;                 // Width in pixels
    int height;                // Height in pixels
    int bpp;                   // Bits per pixel
    int size;                  // Size in bytes
    int isDecoded;             // 1 if successfully decoded
    int memoryUsage;           // Memory usage in bytes
    UINT32 crc;                // CRC32 of the data for validation
} GraphicsAsset;

// Initialize graphics tracking system
void GraphicsTracker_Init(void);

// Register a graphics asset
int GraphicsTracker_RegisterAsset(const char* name, int width, int height, int bpp, int size, UINT8* data);

// Track the decoding of a graphics asset
void GraphicsTracker_TrackDecoding(int assetId, int success);

// Log info about all graphics assets
void GraphicsTracker_LogAssets(void);

// Get info about a specific asset
GraphicsAsset* GraphicsTracker_GetAsset(int assetId);

// Get total graphics memory usage
int GraphicsTracker_GetTotalMemoryUsage(void);

// Track sprite rendering
void GraphicsTracker_TrackRendering(int spriteCount, int renderedCount);

// Graphics system functions
void Graphics_Init(int width, int height);
void Graphics_SetFrameBuffer(unsigned char* buffer);
uint32_t* Graphics_GetFrameBuffer(void);  // Updated return type
void Graphics_FrameRendered(void);
void Graphics_UpdatePalette(int index, unsigned int color);
void Graphics_SetLayerEnabled(int layer, int enabled);
int Graphics_GetLayerEnabled(int layer);
int Graphics_GetScreenWidth(void);
int Graphics_GetScreenHeight(void);
void Graphics_Exit(void);
void Graphics_PrintStatus(void);

// Initialize graphics components system
int Graphics_InitComponents(void);

#ifdef __cplusplus
}
#endif

#endif // GRAPHICS_TRACKING_H 