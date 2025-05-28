#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "../src/burner/metal/metal_declarations.h"

// Core emulation variables
UINT8* pBurnDraw = NULL;       // Pointer to the frame buffer - will be set by FBNeo before rendering
INT32 nBurnPitch = 0;          // Pitch - how many bytes from one line to the next in memory
INT32 nBurnBpp = 4;            // Bytes per pixel (2, 3, or 4)

// Global error state
MetalErrorInfo g_lastError = {0};

// FBNeo core initialization
int FBNeoInit(void) {
    printf("[FBNEO] Core initialized\n");
    return 0;
}

// FBNeo core shutdown
int FBNeoExit(void) {
    printf("[FBNEO] Core shutdown\n");
    return 0;
}

// Run a frame of emulation
int RunFrame(int bDraw) {
    printf("[FBNEO] Running frame (draw=%d)\n", bDraw);
    return 0;
} 