#ifndef METAL_EXPORTS_H
#define METAL_EXPORTS_H

#pragma once

// Metal exports header
// This defines functions that are implemented elsewhere in the Metal port but used in the bridge

#include "metal_common.h"
#include "metal_declarations.h"

#ifdef __cplusplus
extern "C" {
#endif

// Core initialization functions
INT32 BurnInit();
INT32 BurnInputInit();
INT32 BurnSoundInit();
INT32 BurnInputSetKey(INT32 i, INT32 nState);
INT32 BurnLibInit();
INT32 BurnLibExit();
INT32 BurnDrvInit();
INT32 BurnDrvExit();
INT32 BurnDrvReset();
INT32 BurnDrvFrame();
INT32 BurnRecalcPal();
INT32 BurnDrvGetIndex(char* szName);
char* BurnDrvGetTextA(UINT32 i);
INT32 BurnDrvGetVisibleSize(INT32* pnWidth, INT32* pnHeight);
INT32 BurnDrvGetAspect(INT32* pnXAspect, INT32* pnYAspect);

// Input functions
void InputMake(bool bCopy);
void InputInit();
void InputExit();

// ROM path management
extern char g_szROMPath[MAX_PATH];

// ROM path getter/setter functions
const char* GetROMPathString();
int SetCurrentROMPath(const char* szPath);

// Initialize Metal renderer with a device
bool Metal_InitRenderer(void* device);

// Initialize Metal renderer with a view
int Metal_InitRendererWithView(void* view);

// Shutdown Metal renderer
void Metal_ShutdownRenderer();

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

// Get frame texture
void* Metal_GetFrameTexture();

// Render to a command buffer
void Metal_RenderToCommandBuffer(void* commandBuffer);

// Set renderer configuration
void Metal_SetConfig(const MetalRendererConfig* config);

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

#ifdef __cplusplus
}
#endif

#endif // METAL_EXPORTS_H
