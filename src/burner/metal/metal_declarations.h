#pragma once

#include "burn.h"

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations for C++ - only define if not in Objective-C mode
#ifndef __OBJC__
typedef void* NSView;
typedef void* MTLDevice;
typedef void* MTLCommandBuffer;
typedef void* MTLTexture;
#endif

// Metal renderer configuration
typedef struct {
    int width;
    int height;
    bool vsync;
    bool fullscreen;
    float scale;
} MetalRendererConfig;

// Metal render modes
typedef enum {
    RenderModeStandard = 0,     // Standard bilinear filtering
    RenderModeCRT = 1,          // CRT emulation
    RenderModePixelPerfect = 2, // Pixel-perfect (nearest neighbor)
    RenderModeScanlines = 3,    // Scanlines effect
    RenderModeAIEnhanced = 4    // AI-enhanced upscaling (if available)
} MetalRenderMode;

// Metal shader types
typedef enum {
    ShaderTypeBasic = 0,        // Basic bilinear filtering
    ShaderTypeCRT = 1,          // CRT emulation
    ShaderTypePixelPerfect = 2, // Pixel-perfect (nearest neighbor)
    ShaderTypeScanlines = 3,    // Scanlines effect
    ShaderTypeAIEnhanced = 4    // AI-enhanced upscaling (if available)
} MetalShaderType;

// Metal renderer functions
bool Metal_InitRenderer(void* device);
void Metal_ExitRenderer();
int Metal_InitRendererWithView(void* view);
void Metal_SetRenderMode(MetalRenderMode mode);
void Metal_SetShaderType(MetalShaderType type);
void Metal_SetConfig(const MetalRendererConfig* config);
void* Metal_GetFrameTexture();
void Metal_RenderToCommandBuffer(void* commandBuffer);
void Metal_UpdateFrameBuffer(const void* data, int width, int height);
void Metal_Resize(int width, int height);
void Metal_SetFullscreen(bool fullscreen);
void Metal_SetVSync(bool vsync);
void Metal_SetScale(float scale);

// Get frame buffer (not used in Metal implementation)
void* Metal_GetFrameBuffer();

// Render a frame with the provided data
int Metal_RenderFrame(void* frameData, int width, int height);

// Update texture with frame data
int Metal_UpdateTexture(void* data, int width, int height, int pitch);

// Set render state
void Metal_SetRenderState(int state, int value);

// Get renderer info
const char* Metal_GetRendererInfo();

// Run a frame
int Metal_RunFrame(int bDraw);

// Set aspect ratio
void Metal_SetAspectRatio(int width, int height);

// Set preserve aspect ratio
void Metal_SetPreserveAspectRatio(bool preserve);

// Set scanline intensity
void Metal_SetScanlineIntensity(float intensity);

// Set CRT curvature
void Metal_SetCRTCurvature(float curvature);

// Set sharpness
void Metal_SetSharpness(float sharpness);

// Get current width
int Metal_GetWidth();

// Get current height
int Metal_GetHeight();

// Toggle fullscreen mode
void Metal_ToggleFullscreen();

// Renderer states
#define METAL_STATE_VSYNC 0
#define METAL_STATE_FILTERING 1
#define METAL_STATE_CRT 2
#define METAL_STATE_SCANLINES 3

// Error codes
#define METAL_ERROR_NONE 0
#define METAL_ERROR_NO_DEVICE 1
#define METAL_ERROR_NO_VIEW 2
#define METAL_ERROR_NOT_INITIALIZED 3
#define METAL_ERROR_TEXTURE_CREATE 4

#ifndef BOOL
typedef int BOOL;
#endif

#ifdef __cplusplus
}
#endif 