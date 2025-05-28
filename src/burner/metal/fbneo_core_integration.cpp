#include "metal_declarations.h"
#include "metal_bridge.h"
#include <stdio.h>
#include <string.h>

// Define minimal static variables
static bool g_initialized = false;
static bool g_gameRunning = false;
static char g_lastError[1024] = {0};

// Initialize FBNeo core
extern "C" int FBNeo_Core_Initialize() {
    printf("FBNeo_Core_Initialize called\n");
    g_initialized = true;
    return 0;
}

// Shutdown FBNeo core
extern "C" int FBNeo_Core_Shutdown() {
    printf("FBNeo_Core_Shutdown called\n");
    g_initialized = false;
    g_gameRunning = false;
    return 0;
}

// Load a ROM
extern "C" int FBNeo_Core_LoadROM(const char* romName) {
    printf("FBNeo_Core_LoadROM called with ROM: %s\n", romName ? romName : "NULL");
    
    if (!romName || !romName[0]) {
        strncpy(g_lastError, "Invalid ROM path", sizeof(g_lastError) - 1);
        return 1;
    }

    g_gameRunning = true;
    return 0;
}

// Run a frame of emulation
extern "C" int FBNeo_Core_RunFrame(bool render) {
    if (!g_gameRunning) {
        return 1;
    }
    
    // This would normally run one frame of emulation
    // For the minimal implementation, we just return success
    return 0;
}

// Reset the game
extern "C" int FBNeo_Core_Reset() {
    printf("FBNeo_Core_Reset called\n");
    return 0;
}

// Set pause state
extern "C" int FBNeo_Core_SetPause(bool pause) {
    printf("FBNeo_Core_SetPause called with state: %d\n", pause);
    return 0;
}

// Check if core is initialized
extern "C" bool FBNeo_Core_IsInitialized() {
    return g_initialized;
}

// Check if a game is running
extern "C" bool FBNeo_Core_IsGameRunning() {
    return g_gameRunning;
}

// Get the last error message
extern "C" const char* FBNeo_Core_GetLastError() {
    return g_lastError[0] ? g_lastError : "No error";
} 