#ifndef METAL_RENDERER_H
#define METAL_RENDERER_H

// Include standard C headers
#include <stdint.h>
#include <stdbool.h>
#include "metal_common.h"

// MetalKit includes for Objective-C++ code only
#ifdef __OBJC__
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
@class MetalRenderer;
@class NSView;
@protocol MTLDevice;
#endif

// Ensure C linkage for non-Objective-C++ code
#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations for C/C++ code - use void* for opaque pointers
#ifndef __OBJC__
typedef void* id;
typedef void NSView;
#endif

// Initialize Metal renderer with a device - use void* for compatibility
bool Metal_InitRenderer(void* device);

// Initialize Metal renderer with a view - use void* for compatibility
int Metal_InitRendererWithView(void* view);

// Shutdown Metal renderer
void Metal_ShutdownRenderer();

// Get frame buffer (not used in Metal implementation)
void* Metal_GetFrameBuffer();

// Render a frame with the provided data
int Metal_RenderFrame(void* frameData, int width, int height);

// Update texture with frame data - changed to match metal_common.h
int Metal_UpdateTexture(void* data, int width, int height, int pitch);

// Set render state
void Metal_SetRenderState(int state, int value);

// Get renderer info
const char* Metal_GetRendererInfo();

// Run a frame
int Metal_RunFrame(int bDraw);

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

// Emulation modes for C code (renamed to avoid enum conflict)
#define EMULATION_MODE_MINIMAL_MACRO 0
#define EMULATION_MODE_CPS2_MACRO    1

// Metal view creation and management
bool MetalRenderer_CreateView(void* windowHandle, int width, int height);
void MetalRenderer_Cleanup();
bool MetalRenderer_VerifyPipeline();

// Texture management
bool MetalRenderer_UpdateTexture(void* data, int width, int height);

// Emulation control
bool MetalRenderer_SetEmulationMode(int mode);
bool MetalRenderer_LoadGame(int gameIndex);

// Legacy functions for compatibility
int MetalRenderer_Init(void* viewPtr, int width, int height);
void MetalRenderer_Exit();
bool MetalRenderer_Resize(int width, int height);
void MetalRenderer_SetPaused(bool paused);
void MetalRenderer_ForceRedraw();
bool MetalRenderer_UpdateFrameTexture(void* frameData, int width, int height, int pitch);
int MetalRenderer_SetAspectRatio(int mode);

// Frame management
int MetalRenderer_UpdateFrame(const void* frameData, int width, int height, int pitch);
void MetalRenderer_Draw();

// Renderer settings
void MetalRenderer_SetVSync(bool enabled);
void MetalRenderer_SetFilter(bool useBilinear);
void MetalRenderer_SetScanlines(bool enabled, float intensity);
void MetalRenderer_SetCRTEffect(bool enabled, float curvature);

// Renderer info
const char* MetalRenderer_GetInfo();
void MetalRenderer_TakeScreenshot(const char* filename);

// Frame buffer management
void* MetalRenderer_GetFrameBuffer();
int MetalRenderer_GetFrameBufferSize();
bool MetalRenderer_IsFrameBufferUpdated();

// Toggle fullscreen mode
void MetalRenderer_ToggleFullscreen(void);

// Render to a command buffer
void MetalRenderer_Render(void* commandBuffer);

// Update and draw the next frame with current buffer
void MetalRenderer_DrawNextFrame(void);

// Set the rendering scale (1.0 = original size)
void MetalRenderer_SetScale(float scale);

// Set whether to use core rendering (vs test pattern)
void MetalRenderer_SetUseCoreRendering(bool useCoreRendering);

// Get if core rendering is enabled
bool MetalRenderer_GetUseCoreRendering(void);

// Set whether rendering is continuous or on-demand
void MetalRenderer_SetContinuousRendering(bool continuous);

// Set scaling mode (0 = nearest, 1 = linear, 2 = scale2x)
int MetalRenderer_SetScalingMode(int mode);

#ifdef __cplusplus
}
#endif

// Objective-C++ class definition - only visible to Objective-C++ code
#ifdef __OBJC__
/**
 * @class MetalRenderer
 * @brief Objective-C++ class for Metal rendering
 * 
 * This class handles the Metal rendering pipeline and provides
 * drawing utilities for both game rendering and overlays.
 */
@interface MetalRenderer : NSObject

/**
 * Initialize with a Metal device and view
 */
- (instancetype)initWithDevice:(id<MTLDevice>)device view:(MTKView*)view;

/**
 * Initialize with a view - for simplified instantiation
 */
- (instancetype)initWithView:(NSView *)view;

/**
 * Draw a frame from buffer
 */
- (void)drawFrame:(void*)buffer width:(int)width height:(int)height pitch:(int)pitch;

/**
 * Update texture with buffer data
 */
- (void)updateTextureWithBuffer:(unsigned char *)buffer width:(int)width height:(int)height pitch:(int)pitch;

/**
 * Render the frame
 */
- (void)renderFrame;

/**
 * Clear the frame buffer
 */
- (void)clearFrame;

/**
 * Present the current frame
 */
- (void)presentFrame;

/**
 * Draw a rectangle with color
 */
- (void)drawRect:(float)x y:(float)y width:(float)width height:(float)height r:(float)r g:(float)g b:(float)b a:(float)a;

/**
 * Draw a triangle with color
 */
- (void)drawTriangle:(float)x1 y1:(float)y1 x2:(float)x2 y2:(float)y2 x3:(float)x3 y3:(float)y3 r:(float)r g:(float)g b:(float)b a:(float)a;

/**
 * Draw text with color
 */
- (void)drawText:(float)x y:(float)y text:(NSString*)text r:(float)r g:(float)g b:(float)b a:(float)a scale:(float)scale;

/**
 * Get viewport width
 */
- (float)getViewportWidth;

/**
 * Get viewport height
 */
- (float)getViewportHeight;

/**
 * Set VSync enabled/disabled
 */
- (void)setVSync:(BOOL)enabled;

/**
 * Begin overlay rendering
 */
- (void)beginOverlayRendering;

/**
 * End overlay rendering
 */
- (void)endOverlayRendering;

@end
#endif

#endif // METAL_RENDERER_H 