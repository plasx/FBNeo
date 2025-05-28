#ifndef _METAL_TYPES_H_
#define _METAL_TYPES_H_

#ifdef __OBJC__
#import <Metal/Metal.h>
#endif

#if defined(__cplusplus)
extern "C" {
#endif

// Use the same types as interface.h to avoid conflicts
#include "../../interface.h"

// These are declared in interface.h already, so we don't need to redeclare them
// extern INT32 nVidImageWidth;
// extern INT32 nVidImageHeight;
// extern INT32 nVidImageDepth;
// extern INT32 nVidImageBPP;
// extern INT32 nVidImagePitch;

// Metal-specific defines
#define METAL_RGB565_FORMAT 1
#define METAL_RGBA8888_FORMAT 2

// Function prototypes for Metal interface
int InitMetalShaders();
void ShutdownMetalShaders();
#ifdef __OBJC__
void MetalCopyRect(MTLTexture* texture, void* srcdata, int width, int height, int pitch);
#endif

// Functions exported from Objective-C++ code
extern int InitializeMetal(void* windowHandle, int width, int height);
extern void ShutdownMetal();
extern void ResizeMetal(int width, int height);
extern void RenderFrame(void* buffer, int width, int height, int pitch, int bpp);
extern void ClearFrame();
extern void PresentFrame();
extern void SetVSync(int enabled);

// Game state functions
extern void RunFrame();
extern void PauseGame();
extern void ResumeGame();
extern void ResetGame();

// Input handling
extern int ProcessKeyboardInput(int keyCode, int keyDown);
extern int ProcessMouseInput(int button, int x, int y, int down);
extern void SetMousePosition(int x, int y);

// SDL variables
extern unsigned int SDL_GetTicks();
extern void SDL_Delay(unsigned int ms);

#if defined(__cplusplus)
}
#endif

#endif // _METAL_TYPES_H_ 