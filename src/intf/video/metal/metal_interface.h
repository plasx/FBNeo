#pragma once

#include <Metal/Metal.h>
#include <MetalKit/MetalKit.h>
#include "burnint.h"

// Metal rendering interface for the FBNeo Metal backend

// Shader types
typedef enum {
    ShaderTypeBasic = 0,           // Simple texture shader
    ShaderTypeCRT = 1,             // CRT effect shader
    ShaderTypePixelPerfect = 2,    // Pixel-perfect nearest-neighbor sampling
    ShaderTypeAIEnhanced = 3,      // AI-enhanced shader for upscaling
} MetalShaderType;

// Render mode
typedef enum {
    RenderModeStandard = 0,        // Standard rendering
    RenderModeScanlines = 1,       // Scanline effect
    RenderModeCRT = 2,             // Full CRT effect (curve + scanlines)
    RenderModePixelPerfect = 3,    // Pixel-perfect mode
    RenderModeAIEnhanced = 4,      // AI-enhanced mode
} MetalRenderMode;

// Renderer configuration
typedef struct {
    int width;                     // Viewport width
    int height;                    // Viewport height
    float scanlineIntensity;       // Scanline intensity (0.0 - 1.0)
    float crtCurvature;            // CRT curvature amount (0.0 - 1.0)
    float sharpenAmount;           // Image sharpening (0.0 - 1.0)
    bool useVSync;                 // Use vsync
    bool useTripleBuffering;       // Use triple buffering
    float gameAspectRatio;         // Original game aspect ratio
    bool preserveAspectRatio;      // Preserve aspect ratio
    bool useIntegerScaling;        // Use integer scaling
    MetalRenderMode renderMode;    // Current render mode
} MetalRendererConfig;

#ifdef __cplusplus
extern "C" {
#endif

// Initialize Metal renderer
bool Metal_InitRenderer(void* viewPtr, int width, int height);

// Shut down Metal renderer
void Metal_ShutdownRenderer();

// Reset the renderer (for use after device loss)
void Metal_ResetRenderer();

// Set renderer configuration
void Metal_SetRendererConfig(MetalRendererConfig* config);

// Get renderer configuration
void Metal_GetRendererConfig(MetalRendererConfig* config);

// Update a game frame
bool Metal_UpdateFrame(unsigned char* buffer, int width, int height, int pitch);

// Render the current frame
void Metal_RenderFrame();

// Create a texture from buffer
id<MTLTexture> Metal_CreateTextureFromBuffer(unsigned char* buffer, int width, int height, int pitch);

// Load AI-enhanced texture for the current frame
bool Metal_LoadAIEnhancedTexture(unsigned char* buffer, int width, int height, int pitch);

// Set current shader type
void Metal_SetShaderType(MetalShaderType shaderType);

// Get current shader type
MetalShaderType Metal_GetShaderType();

// Set render mode
void Metal_SetRenderMode(MetalRenderMode renderMode);

// Get render mode
MetalRenderMode Metal_GetRenderMode();

// Take screenshot
bool Metal_TakeScreenshot(const char* filename);

// Get current FPS
float Metal_GetFPS();

#ifdef __cplusplus
}
#endif 