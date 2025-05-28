#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>  // For free()

// Type definitions required by our functions
typedef int32_t INT32;
typedef uint8_t UINT8;

// External references to frame buffer variables defined in metal_declarations.c
extern UINT8* pBurnDraw_Metal;
extern INT32 nBurnPitch_Metal;
extern INT32 nBurnBpp_Metal;

// Minimal implementation of BurnLibInit
INT32 BurnLibInit() {
    // Initialize basic values
    pBurnDraw_Metal = NULL;
    nBurnPitch_Metal = 0;
    nBurnBpp_Metal = 0;
    
    // Return success
    return 0;
}

// Minimal implementation of BurnLibExit
INT32 BurnLibExit() {
    // Clean up any resources
    if (pBurnDraw_Metal) {
        free(pBurnDraw_Metal);
        pBurnDraw_Metal = NULL;
    }
    
    // Return success
    return 0;
}

// Minimal implementation of Metal_RunFrame
INT32 Metal_RunFrame(int bDraw) {
    // Simulate running a frame
    if (bDraw) {
        // Would normally render a frame
    }
    
    // Return success
    return 0;
} 