#include "metal_declarations.h"
#include <string.h>
#include <stdio.h>

// ROM path management
char g_szROMPath[MAX_PATH] = {0};

// Export all functions with C linkage to match the declarations in the headers
extern "C" {

// ROM path getter function
const char* GetROMPathString() {
    return g_szROMPath[0] ? g_szROMPath : NULL;
}

// ROM path setter function
int SetCurrentROMPath(const char* path) {
    if (!path) {
        return 1;
    }
    
    strncpy(g_szROMPath, path, MAX_PATH - 1);
    g_szROMPath[MAX_PATH - 1] = '\0';
    
    printf("ROM path set to: %s\n", g_szROMPath);
    return 0;
}

// Minimal Metal API implementations

// Stub implementation for Metal_Init
int Metal_Init(void* viewPtr, MetalDriverSettings* settings) {
    printf("Metal_Init called with view: %p\n", viewPtr);
    return 0;
}

// Stub implementation for Metal_Exit
void Metal_Exit() {
    printf("Metal_Exit called\n");
}

// Stub implementation for Metal_RunFrame
INT32 Metal_RunFrame(int bDraw) {
    static int frameCount = 0;
    if (frameCount % 60 == 0) {
        printf("Metal_RunFrame called (frame %d)\n", frameCount);
    }
    frameCount++;
    return 0;
}

// Stub implementation for Metal_InitFBNeo
int Metal_InitFBNeo() {
    printf("Metal_InitFBNeo called\n");
    return 0;
}

// Stub implementation for Metal_InitInput
int Metal_InitInput() {
    printf("Metal_InitInput called\n");
    return 0;
}

// Stub implementation for Metal_InitAI
int Metal_InitAI() {
    printf("Metal_InitAI called\n");
    return 0;
}

// Stub implementation for Metal_RenderFrame
INT32 Metal_RenderFrame(void* frameData, int width, int height) {
    // No implementation needed for stub
    return 0;
}

// Initialize the Metal renderer
int Metal_InitRenderer(int width, int height, int bpp) {
    printf("Metal_InitRenderer(%d, %d, %d) called from bridge\n", width, height, bpp);
    return 0;
}

// Shutdown the Metal renderer
void Metal_ShutdownRenderer() {
    printf("Metal_ShutdownRenderer() called from bridge\n");
}

} // extern "C" 