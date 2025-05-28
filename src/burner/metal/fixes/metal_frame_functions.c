#include "../metal_declarations.h"
#include <stdio.h>

// Global frame buffer information
extern UINT8* pBurnDraw_Metal;
extern INT32 nBurnPitch_Metal;
extern INT32 nBurnBpp_Metal;

// Variables to track frame dimensions
static int g_nFrameWidth = 320;  // Default width
static int g_nFrameHeight = 240; // Default height

// Get the current frame width
int Metal_GetFrameWidth(void) {
    // Just use our stored value since BurnDrvInfo isn't accessible from C
    return g_nFrameWidth;
}

// Get the current frame height
int Metal_GetFrameHeight(void) {
    // Just use our stored value since BurnDrvInfo isn't accessible from C
    return g_nFrameHeight;
}

// Set the current frame dimensions
void Metal_SetFrameDimensions(int width, int height) {
    if (width > 0 && height > 0) {
        g_nFrameWidth = width;
        g_nFrameHeight = height;
    }
}
