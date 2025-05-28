#ifndef METAL_COMMON_H
#define METAL_COMMON_H

// Define TCHAR first, before any other includes - this is critical for Metal builds
#ifndef TCHAR_DEFINED
#define TCHAR_DEFINED
typedef char TCHAR;
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

// Define basic types first, before including FBNeo headers
typedef uint8_t UINT8;
typedef int8_t INT8;
typedef uint16_t UINT16;
typedef int16_t INT16;
typedef uint32_t UINT32;
typedef int32_t INT32;
typedef uint64_t UINT64;
typedef int64_t INT64;

// Define MAX_PATH
#ifndef MAX_PATH
#define MAX_PATH 512
#endif

// Metal-specific includes
#ifdef __OBJC__
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <Cocoa/Cocoa.h>
#endif

// NOTE: Do NOT include burnint.h here to avoid circular dependencies

// Metal renderer settings
typedef struct {
    int width;
    int height;
    int bpp;
    bool vsync;
    bool fullscreen;
} MetalDriverSettings;

// Metal callback types
typedef void (*MetalInitCallback)(void* context);
typedef void (*MetalRenderFrameCallback)(void* context, int width, int height);
typedef void (*MetalShutdownCallback)(void* context);

// Metal function declarations
#ifdef __cplusplus
extern "C" {
#endif

// Core Metal functions
int Metal_Init(void* viewPtr, MetalDriverSettings* settings);
int Metal_Exit(void);
int Metal_IsActive(void);
const char* Metal_GetRendererInfo(void);

// Frame buffer functions
void* Metal_GetFrameBuffer(void);
int Metal_UpdateTexture(void* data, int width, int height, int pitch);
int Metal_ShowTestPattern(int width, int height);

// Game functions
int Metal_LoadROM(const char* romPath);
int Metal_RunGame(void);
int Metal_ResetGame(void);
int Metal_PauseGame(int pause);

// Input functions
int Metal_HandleKeyDown(int keyCode);
int Metal_HandleKeyUp(int keyCode);
int Metal_InitInput(void);

// Callback registration
void Metal_RegisterCallbacks(MetalInitCallback initFunc,
                           MetalRenderFrameCallback renderFunc,
                           MetalShutdownCallback shutdownFunc);

// Metal window management
void* Metal_CreateWindow(int width, int height, const char* title);
void Metal_DestroyWindow(void* window);
void Metal_ShowWindow(void* window);

#ifdef __cplusplus
}
#endif

#endif // METAL_COMMON_H 