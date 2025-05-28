#include "../metal_declarations.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Game name for input config
char currentGameName[MAX_PATH] = "METAL_BUILD";

// Missing function stubs for AI
bool AI_ApplyActions(const struct AIActions* actions) {
    printf("[AI_ApplyActions] Stub called\n");
    return true;
}

// Missing function stubs for Metal AI
int Metal_AI_Initialize(const char* configPath) {
    printf("[Metal_AI_Initialize] Stub called with path: %s\n", configPath ? configPath : "NULL");
    return 0;
}

void Metal_AI_Shutdown() {
    printf("[Metal_AI_Shutdown] Stub called\n");
}

void Metal_AI_ProcessFrame(void* frameData, int width, int height, int frameNumber) {
    // printf("[Metal_AI_ProcessFrame] Stub called for frame %d\n", frameNumber);
}

void Metal_AI_RenderOverlay(void* view) {
    // printf("[Metal_AI_RenderOverlay] Stub called\n");
}

// Missing function stubs for input config
void Metal_ShowInputConfigWindow(const char* gameName) {
    printf("[Metal_ShowInputConfigWindow] Stub called for game: %s\n", gameName ? gameName : "NULL");
}

void Metal_ShowInputConfigWindowWithTab(const char* gameName, const char* tabName) {
    printf("[Metal_ShowInputConfigWindowWithTab] Stub called for game: %s, tab: %s\n", 
           gameName ? gameName : "NULL", tabName ? tabName : "NULL");
}

// Missing function stubs for driver handling
int BurnDrvGetAspect(int* pnXAspect, int* pnYAspect) {
    if (pnXAspect) *pnXAspect = 4;
    if (pnYAspect) *pnYAspect = 3;
    return 0;
}

// Missing function stubs for Metal bridge
int BurnDrvInit_Metal(int nDrvNum) {
    printf("[BurnDrvInit_Metal] Stub called for driver #%d\n", nDrvNum);
    return BurnDrvInit();
}

int BurnLibInit_Metal() {
    printf("[BurnLibInit_Metal] Stub called\n");
    return 0;
}

int BurnLibExit_Metal() {
    printf("[BurnLibExit_Metal] Stub called\n");
    return 0;
}

// Frame pipeline verification
int Metal_VerifyFramePipeline(int width, int height) {
    printf("[Metal_VerifyFramePipeline] Stub called for %dx%d\n", width, height);
    
    // Allocate a test frame buffer
    void* buffer = malloc(width * height * 4);
    if (!buffer) {
        printf("[Metal_VerifyFramePipeline] Failed to allocate buffer\n");
        return 1;
    }
    
    // Clear the buffer
    memset(buffer, 0, width * height * 4);
    
    // Generate a test pattern
    uint32_t* pixels = (uint32_t*)buffer;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            uint8_t r = (x * 255) / width;
            uint8_t g = (y * 255) / height;
            uint8_t b = ((x + y) * 255) / (width + height);
            
            pixels[y * width + x] = 0xFF000000 | (r << 16) | (g << 8) | b;
        }
    }
    
    // Try to render the test pattern
    printf("[Metal_VerifyFramePipeline] Rendering test pattern\n");
    int result = Metal_RenderFrame(buffer, width, height);
    
    // Free the buffer
    free(buffer);
    
    return result;
} 