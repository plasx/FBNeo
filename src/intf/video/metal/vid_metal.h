#ifndef _VID_METAL_H_
#define _VID_METAL_H_

#pragma once

#ifdef __OBJC__
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <simd/simd.h>
#endif

#include "../../interface.h"

#include "metal_types.h"
#include "ShaderOptions.h"

#include "burner.h"

// Metal-specific rect structure to avoid conflicts
typedef struct {
    float x;
    float y;
    float width;
    float height;
} METAL_RECT;

// Forward declare InterfaceInfo to avoid header conflicts
struct InterfaceInfo;

// Post processing parameters
typedef struct {
    float scanlineIntensity;
    float scanlineWidth;
    float scanlineOffset;
    float crtCurvature;
    float vignetteStrength;
    float vignetteSmoothness;
#ifdef __OBJC__
    simd_float2 resolution;
    simd_float2 screenSize;
#else
    float resolution[2];
    float screenSize[2];
#endif
    int dynamicResolution;
} MetalPostProcessParams;

// Metal renderer interface
struct VidOutMetal {
    int nWidth;
    int nHeight;
    int nDepth;
    
    int nRotateGame;
    int nRotateScreen;
    
    int nTextureWidth;
    int nTextureHeight;
    
    void* device;        // id<MTLDevice>
    void* commandQueue;  // id<MTLCommandQueue>
    void* texture;       // id<MTLTexture>
    void* library;       // id<MTLLibrary>
    
    MetalPostProcessParams params;
    
    int (*Init)();
    int (*Exit)();
    int (*Frame)(bool bRedraw);
};

// Global instance
extern struct VidOutMetal VidOutMetal;

// Interface functions
int VidMetalInit();
int VidMetalExit();
int VidMetalFrame(bool bRedraw);
int VidMetalSetFullscreen(bool bFullscreen);
int VidMetalSetVSync(bool bVSync);
void Metal_VideoToggleFullscreen();

// Shader control functions
void VidMetalSetShaderOptions(ShaderOptions options);
void VidMetalSetShaderParameters(ShaderParameters params);
void VidMetalEnableScanlines(bool enable, float intensity);
void VidMetalEnableCRTCurvature(bool enable, float amount);
void VidMetalEnableVignette(bool enable, float strength, float smoothness);
void VidMetalEnableDynamicResolution(bool enable);
void VidMetalApplyShaderPreset(int presetIndex);

// Interface functions for FBNeo Metal renderer

// Initialize Metal renderer
INT32 VidMetalInit();

// Shutdown Metal renderer
INT32 VidMetalExit();

// Set screen size
INT32 VidMetalSetScreenSize(UINT32 nWidth, UINT32 nHeight);

// Clear the screen
INT32 VidMetalClear();

// Present a frame
INT32 VidMetalPresentFrame(INT32 nDraw);

// Set up a frame (not used in Metal implementation)
INT32 VidMetalFrame(bool bRedraw);

// Set up high color format
INT32 VidMetalSetBurnHighCol(INT32 nDepth);

// C interface for Metal implementation
#ifdef __cplusplus
extern "C" {
#endif

// Initialize Metal renderer
int Metal_Init();

// Shutdown Metal renderer
void Metal_Exit();

// Set screen size
void Metal_SetScreenSize(int width, int height);

// Render a frame
void Metal_RenderFrame(unsigned char* buffer, int width, int height, int pitch);

// Set window title
void Metal_SetWindowTitle(const char* title);

// External function declarations
extern INT32 VidInit();
extern INT32 VidExit();
extern INT32 VidFrame();
extern INT32 VidRedraw();
extern INT32 VidRecalcPal();
extern INT32 VidImageSize(RECT* pRect, INT32 nGameWidth, INT32 nGameHeight);

// Define plugin structure for registration
static struct VidOut VidOutMetal = {
    VidInit,
    VidExit,
    VidFrame,
    VidRedraw,
    VidRecalcPal,
    VidImageSize,
    _T("Metal Plugin")
};

#ifdef __cplusplus
}
#endif

#endif // _VID_METAL_H_ 