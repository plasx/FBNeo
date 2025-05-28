#ifndef METAL_RENDERER_C_H
#define METAL_RENDERER_C_H

// This is a C-compatible header for the Metal renderer
// It provides a clean interface between C and Objective-C++ code

#include <stdint.h>
#include <stdbool.h>
#include "metal_declarations.h"

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
@class NSView;
@protocol MTLDevice;
@protocol MTLCommandBuffer;
@protocol MTLTexture;

// Metal renderer configuration
typedef struct {
    int width;
    int height;
    float scanlineIntensity;
    float crtCurvature;
    float sharpenAmount;
    bool useVSync;
    bool useTripleBuffering;
    float gameAspectRatio;
    bool preserveAspectRatio;
    bool useIntegerScaling;
    int renderMode;
} MetalRendererConfig;

// Render modes
typedef enum {
    RenderModeStandard = 0,     // Standard bilinear filtering
    RenderModeCRT = 1,          // CRT emulation
    RenderModePixelPerfect = 2, // Pixel-perfect (nearest neighbor)
    RenderModeScanlines = 3,    // Scanlines effect
    RenderModeAIEnhanced = 4    // AI-enhanced upscaling (if available)
} MetalRenderMode;

// Shader types
typedef enum {
    ShaderTypeBasic = 0,        // Basic bilinear filtering
    ShaderTypeCRT = 1,          // CRT emulation
    ShaderTypePixelPerfect = 2, // Pixel-perfect (nearest neighbor)
    ShaderTypeScanlines = 3,    // Scanlines effect
    ShaderTypeAIEnhanced = 4    // AI-enhanced upscaling (if available)
} MetalShaderType;

// Initialize Metal renderer with a device
bool Metal_InitRenderer(id<MTLDevice> device);

// Initialize Metal renderer with a view
int Metal_InitRendererWithView(NSView* view);

// Shutdown Metal renderer
void Metal_ShutdownRenderer();

// Get frame buffer (not used in Metal implementation)
void* Metal_GetFrameBuffer();

// Render a frame with the provided data
int Metal_RenderFrame(void* frameData, int width, int height);

// Update texture with frame data
void Metal_UpdateTexture(void* data, int width, int height);

// Set render state
void Metal_SetRenderState(int state, int value);

// Get renderer info
const char* Metal_GetRendererInfo();

// Run a frame
int Metal_RunFrame(int bDraw);

// Get frame texture
id<MTLTexture> Metal_GetFrameTexture();

// Render to a command buffer
void Metal_RenderToCommandBuffer(id<MTLCommandBuffer> commandBuffer);

// Set renderer configuration
void Metal_SetConfig(MetalRendererConfig* config);

// Get renderer configuration
void Metal_GetConfig(MetalRendererConfig* config);

// Set shader type
void Metal_SetShaderType(MetalShaderType type);

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

// Helper functions
int Metal_ConvertRGBtoYUV(void* rgbData, void* yuvData, int width, int height, int format);
int Metal_ConvertYUVtoRGB(void* yuvData, void* rgbData, int width, int height, int format);
int Metal_GetAvailableShaders(char** shaderNames, int maxShaders);
int Metal_LoadCustomShader(const char* path);

#ifdef __cplusplus
}
#endif

#endif // METAL_RENDERER_C_H 