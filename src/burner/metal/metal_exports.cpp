//
// metal_exports.cpp
//
// Metal build exports for FBNeo
//

// Include our metal declarations first
#include "metal_declarations.h"
#include "metal_bridge.h"

// System includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// External declarations from burn.h
extern char** pBurnDrvGetTextA(UINT32 i);
extern int nBurnDrvCount;

// Export paths and information
char szAppRomPaths[DIRS_MAX][MAX_PATH];
char szAppDirPath[MAX_PATH];

// Global pointers for Metal build functions
UINT8* pBurnDraw = NULL;
INT32 nBurnPitch = 0; 
INT32 nBurnBpp = 32;

//
// ROM Path Management
//

// Get the ROM search paths
int GetROMPaths(char paths[DIRS_MAX][MAX_PATH]) {
    if (!paths) {
        return -1;
    }
    
    // Copy the ROM paths
    for (int i = 0; i < DIRS_MAX; i++) {
        if (szAppRomPaths[i][0] != '\0') {
            strcpy(paths[i], szAppRomPaths[i]);
        } else {
            paths[i][0] = '\0';
        }
    }
    
    return 0;
}

// Set the ROM search paths
int SetROMPaths(const char paths[DIRS_MAX][MAX_PATH]) {
    if (!paths) {
        return -1;
    }
    
    // Copy the ROM paths
    for (int i = 0; i < DIRS_MAX; i++) {
        if (paths[i][0] != '\0') {
            strcpy(szAppRomPaths[i], paths[i]);
        } else {
            szAppRomPaths[i][0] = '\0';
        }
    }
    
    return 0;
}

// Get a specific ROM path
const char* GetROMPath(int index) {
    if (index < 0 || index >= DIRS_MAX) {
        return NULL;
    }
    
    return szAppRomPaths[index];
}

// Set a specific ROM path
int SetROMPath(int index, const char* path) {
    if (index < 0 || index >= DIRS_MAX) {
        return -1;
    }
    
    if (!path) {
        szAppRomPaths[index][0] = '\0';
        return 0;
    }
    
    strncpy(szAppRomPaths[index], path, MAX_PATH - 1);
    szAppRomPaths[index][MAX_PATH - 1] = '\0';
    
    return 0;
}

// Get current application directory path
const char* GetAppDir() {
    return szAppDirPath;
}

// Set current application directory path
int SetAppDir(const char* path) {
    if (!path) {
        szAppDirPath[0] = '\0';
        return 0;
    }
    
    strncpy(szAppDirPath, path, MAX_PATH - 1);
    szAppDirPath[MAX_PATH - 1] = '\0';
    
    return 0;
}

//
// Driver Information
//

// Get the number of available drivers
int GetDriverCount() {
    return nBurnDrvCount;
}

// Get the driver name
const char* GetDriverName(int index) {
    // Check if index is valid
    if (index < 0 || index >= nBurnDrvCount) {
        return NULL;
    }
    
    // Get the driver name
    BurnDrvSelect(index);
    return BurnDrvGetTextA(0);
}

// Get the driver full name
const char* GetDriverFullName(int index) {
    // Check if index is valid
    if (index < 0 || index >= nBurnDrvCount) {
        return NULL;
    }
    
    // Get the driver full name
    BurnDrvSelect(index);
    return BurnDrvGetTextA(1);
}

// Re-declare this variable since it's defined in metal_bridge.cpp
extern bool g_gameInitialized;

// Define required variables
// szAppRomPaths and szAppDirPath are defined in rom_fixes.cpp - don't define them here
char szAppBurnVer[16] = "v1.0.0";

// Metal exports - these provide a bridge between the C++ Metal implementation
// and the Metal/Objective-C code.

// Add necessary declarations for FBNeo core functions
#ifdef __cplusplus
extern "C" {
#endif
    // Forward declarations for FBNeo core functions
    INT32 BurnDrvInit();
    INT32 BurnDrvExit();
    INT32 BurnDrvFrame();
    INT32 BurnDrvReset();
#ifdef __cplusplus
}
#endif

// Audio buffer for sound output
extern "C" INT16 *pAudNextSound = NULL;

// Game control functions
extern "C" int Metal_PauseGame(int pause) {
    printf("Metal_PauseGame(%d)\n", pause);
    return 0;
}

extern "C" int Metal_ResetGame() {
    printf("Metal_ResetGame()\n");
    // Call BurnDrvReset to reset the current game
    return BurnDrvReset();
}

extern "C" int Metal_RunGame() {
    printf("Metal_RunGame() - Starting game execution\n");
    
    // Check if a game is initialized
    if (!g_gameInitialized) {
        printf("Error: Cannot run game - no game initialized\n");
        return 1;
    }
    
    // This function should start the game loop or set up a timer for continuous frame execution
    extern int StartGameTimer();
    int result = StartGameTimer();
    if (result != 0) {
        printf("Error: Failed to start game timer: %d\n", result);
        return result;
    }
    
    // Return success
    return 0;
}

// Functions needed for the bridge
extern "C" {
    // UpdateMetalFrameTexture is already defined in metal_app.mm - don't declare it here
    // void UpdateMetalFrameTexture(const void* frameData, unsigned int width, unsigned int height);
    void ToggleMetalPause(int isPaused);
    void SetFullscreen(int fullscreen);
    void SetScalingMode(int mode);
    void SetAspectRatio(int ratio);
    void SetAudioEnabled(int enabled);
    int IsGameRunning();
    int IsGamePaused();
}

// Dummy implementations for interface functions
UINT8* CpsFindGfxRam(int nOffset, int nLen) {
    // Placeholder implementation
    return NULL;
}

void BurnDrvDrawScanline(int y) {
    // Placeholder implementation
}

void BurnDrvUpdateFrame() {
    // Placeholder implementation
}

void* BurnDrvGetMemoryMap(int index) {
    // Placeholder implementation
    return NULL;
}

void RenderFrame(UINT8* pBuffer, int width, int height, int pitch, int bpp) {
    // Placeholder implementation
}

void GetVideoSource(void** buffer, int* width, int* height) {
    // Placeholder implementation
    *buffer = NULL;
    *width = 0;
    *height = 0;
}

unsigned int* GetPalettePtr() {
    // Placeholder implementation
    return NULL;
} 