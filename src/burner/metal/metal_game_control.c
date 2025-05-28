#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "metal_declarations.h"

// Input state tracking
static int g_keyState[512] = {0};  // Track key states

// Game state variables
static char g_gameTitle[256] = "No Game Loaded";
static bool g_gameRunning = false;

// External declarations
extern INT16* pBurnSoundOut;    // Buffer for audio output
extern INT32 nBurnSoundLen;     // Number of samples per frame

// Initialize Metal-specific error handling
MetalErrorInfo g_lastError = {0};

// Set game running state
void Metal_SetGameRunning(bool running) {
    g_gameRunning = running;
}

// Unload the current ROM
void Metal_UnloadROM() {
    // Clean up driver if active
    if (g_gameRunning) {
        BurnDrvExit();
    }
    
    // Reset game state
    Metal_SetGameRunning(false);
    
    // Clear last error
    Metal_ClearLastError();
    
    // Log ROM unload
    METAL_LOG_INFO("ROM unloaded");
}

// Update input state from keyboard and controllers
void Metal_UpdateInputState() {
    // Code to read hardware input and update internal state
    // This is a stub that will need to be expanded
    
    // Process any keys that are held down
    // ...
}

// Process key down events
void Metal_ProcessKeyDown(int keyCode) {
    // Validate key code
    if (keyCode < 0 || keyCode >= 512) {
        return;
    }
    
    // Update key state
    g_keyState[keyCode] = 1;
    
    // Forward to game input handling (implementation depends on core)
    // ...
    
    // Log key press in debug mode
    if (Metal_IsDebugMode()) {
        METAL_LOG_DEBUG("Key down: %d", keyCode);
    }
}

// Process key up events
void Metal_ProcessKeyUp(int keyCode) {
    // Validate key code
    if (keyCode < 0 || keyCode >= 512) {
        return;
    }
    
    // Update key state
    g_keyState[keyCode] = 0;
    
    // Forward to game input handling (implementation depends on core)
    // ...
    
    // Log key release in debug mode
    if (Metal_IsDebugMode()) {
        METAL_LOG_DEBUG("Key up: %d", keyCode);
    }
}

// Check if a key is pressed
int Metal_IsKeyPressed(int keyCode) {
    // Validate key code
    if (keyCode < 0 || keyCode >= 512) {
        return 0;
    }
    
    return g_keyState[keyCode];
}

// Reset all key states
void Metal_ResetKeyStates() {
    memset(g_keyState, 0, sizeof(g_keyState));
    METAL_LOG_DEBUG("Key states reset");
}

// Process audio for current frame
int Metal_ProcessAudio() {
    // Implementation depends on audio system
    // For now, just a stub
    
    // Call BurnSoundRender if FBNeo sound system is active
    if (pBurnSoundOut != NULL && nBurnSoundLen > 0) {
        return BurnSoundRender(pBurnSoundOut, nBurnSoundLen);
    }
    
    return 0;
}

// Generate test pattern
int Metal_GenerateTestPattern(int patternType) {
    // Complete removal of test pattern functionality
    // This function now only returns an error to prevent any test pattern usage
    printf("[Metal_GenerateTestPattern] ERROR: Test pattern generation is permanently disabled.\n");
    printf("[Metal_GenerateTestPattern] Real frame data from the emulation core is always used instead.\n");
    
    // Return error code to indicate failure
    return -1;
}

// Initialize graphics components
int Graphics_InitComponents() {
    METAL_LOG_INFO("Initializing graphics components");
    
    // For now, just initialize the frame buffer
    if (!g_frameBuffer.data) {
        // Create a default frame buffer
        InitFrameBuffer(384, 224);  // Default CPS2 resolution
    }
    
    return 0;
}

// Create or resize the frame buffer
bool InitFrameBuffer(int width, int height) {
    if (width <= 0 || height <= 0) {
        METAL_LOG_ERROR("Invalid frame buffer dimensions: %dx%d", width, height);
        return false;
    }
    
    // Free existing buffer if any
    if (g_frameBuffer.data) {
        free(g_frameBuffer.data);
        g_frameBuffer.data = NULL;
    }
    
    // Allocate new buffer
    size_t bufferSize = width * height * sizeof(uint32_t);
    g_frameBuffer.data = (uint32_t*)malloc(bufferSize);
    
    if (!g_frameBuffer.data) {
        METAL_LOG_ERROR("Failed to allocate frame buffer memory (%zu bytes)", bufferSize);
        return false;
    }
    
    // Initialize buffer properties
    g_frameBuffer.width = width;
    g_frameBuffer.height = height;
    g_frameBuffer.pitch = width * sizeof(uint32_t);
    g_frameBuffer.updated = false;
    
    // Clear buffer to black
    memset(g_frameBuffer.data, 0, bufferSize);
    
    METAL_LOG_INFO("Frame buffer initialized: %dx%d (%zu bytes)", width, height, bufferSize);
    return true;
}

// Get the game title
const char* Metal_GetGameTitle() {
    return g_gameTitle;
}

// Set the game title 
void Metal_SetGameTitle(const char* title) {
    if (title && title[0]) {
        strncpy(g_gameTitle, title, sizeof(g_gameTitle)-1);
        g_gameTitle[sizeof(g_gameTitle)-1] = '\0';
    } else {
        strcpy(g_gameTitle, "Unknown Game");
    }
} 