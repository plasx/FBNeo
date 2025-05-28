#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Basic types
typedef unsigned char UINT8;
typedef unsigned int UINT32;
typedef int INT32;
typedef short INT16;

// Basic variables
extern UINT8* g_pFrameBuffer;
extern int g_nFrameWidth;
extern int g_nFrameHeight;
extern int g_nBPP;

// Function declarations
void Metal_SetFrameBuffer(UINT8* buffer, int width, int height, int bpp);
void Metal_UpdateDisplay();
int Metal_Init();
int Metal_Exit();

#ifdef __cplusplus
}
#endif
