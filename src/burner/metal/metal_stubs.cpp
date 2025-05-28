#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include our fixed files
#include "fixes/c_cpp_compatibility.h"
#include "metal_bridge.h"
#include "metal_renderer.h"

/*
 * Metal-specific core functions for the FBNeo Metal build
 * 
 * These functions provide the bridge between Metal and the FBNeo core
 */

// Use our safe genre variables instead of the direct macros
void* GBF_HORSHOOT_PTR = (void*)(uintptr_t)1;
void* GBF_VERSHOOT_PTR = (void*)(uintptr_t)2;
void* GBF_SCRFIGHT_PTR = (void*)(uintptr_t)4;
void* GBF_PLATFORM_PTR = (void*)(uintptr_t)2048;
void* GBF_VSFIGHT_PTR = (void*)(uintptr_t)8;
void* GBF_BIOS_PTR = (void*)(uintptr_t)16;
void* GBF_BREAKOUT_PTR = (void*)(uintptr_t)64;
void* GBF_CASINO_PTR = (void*)(uintptr_t)128;
void* GBF_BALLPADDLE_PTR = (void*)(uintptr_t)256;
void* GBF_MAZE_PTR = (void*)(uintptr_t)512;
void* GBF_MINIGAMES_PTR = (void*)(uintptr_t)1024;
void* GBF_QUIZ_PTR = (void*)(uintptr_t)8192;
void* GBF_SPORTS_PTR = (void*)(uintptr_t)524288;
void* GBF_RACING_PTR = (void*)(uintptr_t)131072;
void* GBF_SHOOT_PTR = (void*)(uintptr_t)262144;

// Basic type definitions
typedef int32_t INT32;
typedef uint32_t UINT32;
typedef uint16_t UINT16;
typedef uint8_t UINT8;

// Global state variables
static int g_emulationRunning = 0;   // Is emulation currently running
static int g_pauseEmulation = 0;     // Should emulation be paused

// Frame buffer for rendering
static void* g_pFrameBuffer = NULL;
static int g_nFrameWidth = 384;  // Default width
static int g_nFrameHeight = 224; // Default height
static int g_nBurnBpp = 4;       // Default bytes per pixel (RGBA)

// AI system state
static int g_aiEnabled = 0;
static int g_aiDifficulty = 2;
static int g_aiPlayer = 0;
static int g_aiTrainingMode = 0;
static int g_aiDebugOverlay = 0;

// Metal runtime rendering function - actual impl in metal_renderer.mm
// This is called by BurnDrvFrame to render a frame
INT32 Metal_RunFrame(bool bDraw) { 
    // If not drawing, just run the frame logic without rendering
    if (!bDraw) {
        extern INT32 BurnDrvFrame();
        return BurnDrvFrame();
    }
    
    // Prepare the frame buffer if needed
    if (g_pFrameBuffer == NULL) {
        g_pFrameBuffer = malloc(g_nFrameWidth * g_nFrameHeight * g_nBurnBpp);
        if (g_pFrameBuffer == NULL) {
            printf("Failed to allocate frame buffer\n");
            return 1;
        }
    }
    
    // Set the FBNeo core's frame buffer to our buffer
    extern UINT8* pBurnDraw;
    extern INT32 nBurnPitch;
    extern INT32 nBurnBpp;
    
    // Connect our buffer to FBNeo's drawing system
    pBurnDraw = (UINT8*)g_pFrameBuffer;
    nBurnPitch = g_nFrameWidth * g_nBurnBpp;
    nBurnBpp = g_nBurnBpp;
    
    // Also set the Metal-specific globals
    extern UINT8* pBurnDraw_Metal;
    extern INT32 nBurnPitch_Metal;
    extern INT32 nBurnBpp_Metal;
    
    pBurnDraw_Metal = (UINT8*)g_pFrameBuffer;
    nBurnPitch_Metal = g_nFrameWidth * g_nBurnBpp;
    nBurnBpp_Metal = g_nBurnBpp;
    
    // Run the emulation for one frame
    extern INT32 BurnDrvFrame();
    INT32 result = BurnDrvFrame(); 
    
    // Update the Metal texture with the new frame data
    if (result == 0) {
        // Call the Metal renderer to update the frame
        UpdateMetalFrameTexture(g_pFrameBuffer, g_nFrameWidth, g_nFrameHeight);
    }
    
    return result;
}

// Called by Metal to render the current frame to the Metal texture
INT32 Metal_RenderFrame(void* frameData, int width, int height) { 
    if (!frameData) return 1;
    
    // If we don't have a frame buffer yet, return error
    if (!g_pFrameBuffer) return 1;
    
    // If dimensions match, copy the frame buffer directly to the Metal texture
    if (width == g_nFrameWidth && height == g_nFrameHeight) {
        memcpy(frameData, g_pFrameBuffer, width * height * g_nBurnBpp);
        return 0;
    }
    
    // If dimensions don't match, we need to scale the image
    // For simplicity, we'll just center the image and leave black borders
    // A more sophisticated approach would use Metal compute shaders for proper scaling
    
    // Clear the output buffer first
    memset(frameData, 0, width * height * g_nBurnBpp);
    
    // Calculate position to center the image
    int offsetX = (width - g_nFrameWidth) / 2;
    int offsetY = (height - g_nFrameHeight) / 2;
    
    // Ensure offsets are positive
    if (offsetX < 0) offsetX = 0;
    if (offsetY < 0) offsetY = 0;
    
    // Copy line by line
    for (int y = 0; y < g_nFrameHeight && y + offsetY < height; y++) {
        uint8_t* src = (uint8_t*)g_pFrameBuffer + y * g_nFrameWidth * g_nBurnBpp;
        uint8_t* dst = (uint8_t*)frameData + ((y + offsetY) * width + offsetX) * g_nBurnBpp;
        
        int copyWidth = g_nFrameWidth;
        if (offsetX + copyWidth > width) copyWidth = width - offsetX;
        
        if (copyWidth > 0) {
            memcpy(dst, src, copyWidth * g_nBurnBpp);
        }
    }
    
    return 0;
}

// Update texture callback for Metal renderer
void UpdateMetalFrameTexture(const void* frameData, unsigned int width, unsigned int height) {
    // Real implementation will be called from Metal
    extern void MetalRenderer_UpdateFrame(const void* frameData, unsigned int width, unsigned int height);
    MetalRenderer_UpdateFrame(frameData, width, height);
}

// Input functions - these will be implemented in metal_input.mm
INT32 InputInit() { 
    extern INT32 MetalInput_Init();
    return MetalInput_Init();
}

INT32 InputExit() { 
    extern INT32 MetalInput_Exit();
    return MetalInput_Exit();
}

INT32 InputMake(bool bCopy) { 
    extern INT32 MetalInput_Make(bool bCopy);
    return MetalInput_Make(bCopy);
}

// High color conversion function - real implementation
UINT32 BurnHighCol32(INT32 r, INT32 g, INT32 b, INT32 i) {
    // RGBA format for Metal
    return ((r & 0xff) << 24) | ((g & 0xff) << 16) | ((b & 0xff) << 8) | (i & 0xff);
}

// Additional functions to handle frame buffer management
void Metal_SetFrameBufferSize(int width, int height) {
    if (width == g_nFrameWidth && height == g_nFrameHeight) return;
    
    g_nFrameWidth = width;
    g_nFrameHeight = height;
    
    // Reallocate frame buffer if needed
    if (g_pFrameBuffer) {
        free(g_pFrameBuffer);
        g_pFrameBuffer = malloc(width * height * g_nBurnBpp);
    }
}

int Metal_GetFrameWidth() {
    return g_nFrameWidth;
}

int Metal_GetFrameHeight() {
    return g_nFrameHeight;
}

void* Metal_GetFrameBuffer() {
    return g_pFrameBuffer;
}

void Metal_SetBurnBpp(int bpp) {
    g_nBurnBpp = bpp;
}

// Initialize Metal/CoreML integration
int Metal_InitAI(void) {
    printf("Metal_InitAI called.\n");
    
    // Allocate frame buffer if not already done
    if (!g_pFrameBuffer) {
        g_pFrameBuffer = malloc(g_nFrameWidth * g_nFrameHeight * g_nBurnBpp);
        if (!g_pFrameBuffer) {
            printf("Error: Failed to allocate frame buffer memory.\n");
            return -1;
        }
        
        // Clear buffer
        memset(g_pFrameBuffer, 0, g_nFrameWidth * g_nFrameHeight * g_nBurnBpp);
    }
    
    g_aiEnabled = 1;
    printf("Metal AI initialized successfully.\n");
    
    return 0;
}

// Shutdown Metal/CoreML integration
int Metal_ShutdownAI(void) {
    printf("Metal_ShutdownAI called.\n");
    
    // Free frame buffer
    if (g_pFrameBuffer) {
        free(g_pFrameBuffer);
        g_pFrameBuffer = NULL;
    }
    
    g_aiEnabled = 0;
    printf("Metal AI shutdown successfully.\n");
    
    return 0;
}

// Initialize AI system for a specific game
int Metal_InitAIForGame(const char* gameId) {
    printf("Metal_InitAIForGame called for game: %s\n", gameId ? gameId : "unknown");
    
    return 0;
}

// Start AI processing
int Metal_StartAI(void) {
    printf("Metal_StartAI called.\n");
    
    g_aiEnabled = 1;
    return 0;
}

// Stop AI processing
int Metal_StopAI(void) {
    printf("Metal_StopAI called.\n");
    
    g_aiEnabled = 0;
    return 0;
}

// Update AI processing (typically called once per frame)
int Metal_UpdateAI(void) {
    // No implementation needed for stub
    return 0;
}

// Check if AI module is loaded
bool Metal_IsAIModuleLoaded(void) {
    return g_pFrameBuffer != NULL;
}

// Check if AI is currently active
bool Metal_IsAIActive(void) {
    return g_aiEnabled != 0;
}

// Set AI difficulty level
int Metal_SetAIDifficulty(int level) {
    printf("Metal_SetAIDifficulty called with level: %d\n", level);
    
    g_aiDifficulty = level;
    return 0;
}

// Set which player AI should control
int Metal_SetAIPlayer(int player) {
    printf("Metal_SetAIPlayer called with player: %d\n", player);
    
    g_aiPlayer = player;
    return 0;
}

// Enable/disable AI training mode
int Metal_EnableAITrainingMode(int enable) {
    printf("Metal_EnableAITrainingMode called with enable: %d\n", enable);
    
    g_aiTrainingMode = enable;
    return 0;
}

// Enable/disable AI debug overlay
int Metal_EnableAIDebugOverlay(int enable) {
    printf("Metal_EnableAIDebugOverlay called with enable: %d\n", enable);
    
    g_aiDebugOverlay = enable;
    return 0;
}

// Load AI model from provided path
int Metal_LoadAIModel(const char* modelPath) {
    printf("Metal_LoadAIModel called with path: %s\n", modelPath ? modelPath : "NULL");
    
    return 0;
}

// Save current AI model to provided path
int Metal_SaveAIModel(const char* modelPath) {
    printf("Metal_SaveAIModel called with path: %s\n", modelPath ? modelPath : "NULL");
    
    return 0;
}

// Get information about current AI model
int Metal_GetAIModelInfo(char* infoBuffer, int bufferSize) {
    if (!infoBuffer || bufferSize <= 0) {
        return -1;
    }
    
    const char* info = "FBNeo Metal AI Model Stub";
    strncpy(infoBuffer, info, bufferSize);
    infoBuffer[bufferSize - 1] = '\0'; // Ensure null-termination
    
    return 0;
}

// Set frame buffer size
int Metal_SetFrameSize(int width, int height, int bpp) {
    printf("Metal_SetFrameSize called: %dx%d @ %d bpp\n", width, height, bpp);
    
    // Update dimensions
    g_nFrameWidth = width;
    g_nFrameHeight = height;
    g_nBurnBpp = bpp;
    
    // Reallocate frame buffer
    if (g_pFrameBuffer) {
        free(g_pFrameBuffer);
        g_pFrameBuffer = malloc(width * height * bpp);
        if (!g_pFrameBuffer) {
            printf("Error: Failed to reallocate frame buffer.\n");
            return -1;
        }
        
        // Clear buffer
        memset(g_pFrameBuffer, 0, width * height * bpp);
    }
    
    return 0;
}
