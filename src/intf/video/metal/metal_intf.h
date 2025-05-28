#pragma once

// Metal interface header for Objective-C++ (.mm) files
// This should be included only from .mm files

#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>

#include "metal_types.h"

// Forward declarations
@class MetalRenderer;
@class MetalView;

// C interface functions (implemented in Objective-C++)
#ifdef __cplusplus
extern "C" {
#endif

// Metal initialization and rendering
bool InitializeMetal(void* nsViewPtr, int width, int height);
void ShutdownMetal();
bool ResizeMetal(int width, int height);
void RenderFrame(void* buffer, int width, int height, int pitch, int bpp);
void ClearFrame();
void PresentFrame();
void SetVSync(bool enabled);

// Game state management functions
int RunFrame();
bool PauseGame();
bool ResumeGame();
void ResetGame();

// Input handling
bool ProcessKeyboardInput();
bool ProcessMouseInput();
void SetMousePosition(int x, int y);

// Global Metal objects
extern MetalRenderer* g_metalRenderer;
extern MetalView* g_metalView;
extern id<MTLDevice> g_metalDevice;
extern id<MTLCommandQueue> g_commandQueue;

// Shared variables
extern int nVidImageWidth;
extern int nVidImageHeight;
extern int nVidImagePitch;
extern int nVidImageBPP;
extern int nVidImageDepth;
extern unsigned char* pVidImage;
extern int nAppVirtualFps;
extern bool bAppFullscreen;
extern bool bAppDoFPS;
extern bool bAppShowFPS;
extern int nVidScrnWidth;
extern int nVidScrnHeight;
extern int nVidDepth;
extern int nVidFullscreen;
extern int nWindowPosX;
extern int nWindowPosY;

// SDL related
extern unsigned int (*SDL_GetTicks)();
extern void (*SDL_Delay)(unsigned int ms);

INT32 VidRecalcPal();

#ifdef __cplusplus
}
#endif 