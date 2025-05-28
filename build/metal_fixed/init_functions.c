#include <stdio.h>
#include <stdbool.h>

// Core initialization functions - centralized implementation to avoid duplicates

// Main initialization function
int FBNeoInit(void) {
    printf("FBNeoInit: Initializing FBNeo core\n");
    return 0;
}

// Main exit function
int FBNeoExit(void) {
    printf("FBNeoExit: Shutting down FBNeo core\n");
    return 0;
}

// Run one frame of emulation
int RunFrame(int bDraw) {
    // Just a stub to be replaced with proper implementation
    return 0;
}

// Global variable needed by some parts of FBNeo
bool bDoIpsPatch = false;
