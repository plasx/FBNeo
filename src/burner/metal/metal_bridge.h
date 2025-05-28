#ifndef METAL_BRIDGE_H
#define METAL_BRIDGE_H

#include "burnint.h"
#include "metal_common.h"

#ifdef __cplusplus
extern "C" {
#endif

// Bridge functions for FBNeo core
INT32 BurnLibInit_Metal(void);
INT32 BurnLibExit_Metal(void);
INT32 BurnDrvInit_Metal(INT32 nDrvNum);
INT32 BurnDrvExit_Metal(void);

// Frame management
INT32 Metal_RunFrame(int bDraw);
void* Metal_GetFrameBuffer(void);
void SetFrameBufferUpdated(bool updated);
bool IsFrameBufferUpdated(void);

// ROM path management
const char* GetROMPathString(void);
int SetCurrentROMPath(const char* path);

// Metal renderer interface
int Metal_Init(void* viewPtr, MetalDriverSettings* settings);
int Metal_Exit(void);
INT32 Metal_RenderFrame(void* frameData, int width, int height);

// Test functions
int Metal_ShowTestPattern(int width, int height);

#ifdef __cplusplus
}
#endif

#endif // METAL_BRIDGE_H 