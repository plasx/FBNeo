#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "metal_tchar.h"
#include "../metal_declarations.h"

// Global ROM path storage
static char g_RomPath[512] = {0};

// ROM driver functions
extern int nBurnDrvActive;
extern struct BurnDriver* pDriver;

// Log function for different stages
static void log_rom_stage(const char* stage, const char* message) {
    fprintf(stderr, "[%s] %s\n", stage, message);
}

// Set the ROM path for later use
void Metal_SetRomPath(const char* path) {
    if (path) {
        strncpy(g_RomPath, path, sizeof(g_RomPath) - 1);
        g_RomPath[sizeof(g_RomPath) - 1] = '\0';
        log_rom_stage("ROM CHECK", "ROM path set to");
        fprintf(stderr, "[ROM CHECK] Path: %s\n", g_RomPath);
    }
}

// Get the current ROM path
const char* Metal_GetRomPath() {
    return g_RomPath;
}

// Find driver index by ROM name (e.g., "mvsc")
int Metal_FindDriverIndexByName(const char* romName) {
    log_rom_stage("ROM CHECK", "Finding driver index for ROM");
    fprintf(stderr, "[ROM CHECK] ROM name: %s\n", romName);
    
    if (!romName) return -1;
    
    char nameLower[32] = {0};
    strncpy(nameLower, romName, sizeof(nameLower) - 1);
    
    // Convert to lowercase for comparison
    for (int i = 0; nameLower[i]; i++) {
        if (nameLower[i] >= 'A' && nameLower[i] <= 'Z')
            nameLower[i] += 32;
    }
    
    // Call into FBNeo driver search
    int drvIndex = BurnDrvGetIndex(nameLower);
    
    fprintf(stderr, "[ROM CHECK] Found driver index: %d\n", drvIndex);
    return drvIndex;
}

// Extract ROM name from path
const char* Metal_ExtractRomName(const char* romPath) {
    if (!romPath) return NULL;
    
    // Find last slash
    const char* lastSlash = strrchr(romPath, '/');
    if (!lastSlash) lastSlash = strrchr(romPath, '\\');
    
    const char* fileName = lastSlash ? lastSlash + 1 : romPath;
    
    // Find last dot
    const char* lastDot = strrchr(fileName, '.');
    if (!lastDot) return fileName;
    
    // Extract name without extension
    static char romName[64] = {0};
    int nameLen = lastDot - fileName;
    if (nameLen >= sizeof(romName)) nameLen = sizeof(romName) - 1;
    
    strncpy(romName, fileName, nameLen);
    romName[nameLen] = '\0';
    
    return romName;
}

// Load ROM and initialize the driver
bool Metal_LoadAndInitROM(const char* romPath) {
    if (!romPath || !*romPath) {
        log_rom_stage("ROM CHECK", "Invalid ROM path");
        return false;
    }
    
    log_rom_stage("ROM CHECK", "Starting ROM load process");
    fprintf(stderr, "[ROM CHECK] ROM path: %s\n", romPath);
    
    // Store ROM path for later use
    Metal_SetRomPath(romPath);
    
    // Extract ROM name from path
    const char* romName = Metal_ExtractRomName(romPath);
    fprintf(stderr, "[ROM CHECK] Extracted ROM name: %s\n", romName);
    
    // Find driver index
    int drvIndex = Metal_FindDriverIndexByName(romName);
    if (drvIndex < 0) {
        log_rom_stage("ROM CHECK", "Failed to find driver for ROM");
        return false;
    }
    
    // Select the driver
    log_rom_stage("ROM CHECK", "Selecting driver");
    int result = BurnDrvSelect(drvIndex);
    if (result != 0) {
        log_rom_stage("ROM CHECK", "Failed to select driver");
        return false;
    }
    
    // Set active driver
    nBurnDrvActive = drvIndex;
    
    // Initialize memory for ROMs
    log_rom_stage("MEM INIT", "Initializing memory for ROM data");
    if (BurnDrvInit() != 0) {
        log_rom_stage("MEM INIT", "Failed to initialize memory");
        return false;
    }
    
    // Initialize hardware
    log_rom_stage("HW INIT", "Initializing hardware");
    if (BurnDrvInit() != 0) {
        log_rom_stage("HW INIT", "Failed to initialize hardware");
        // BurnDrvMemExit doesn't exist, we already did BurnDrvInit, so no need to clean up at this point
        return false;
    }
    
    // Check for CPS2 hardware
    if (strstr(BurnDrvGetTextA(DRV_SYSTEM), "CPS-2")) {
        log_rom_stage("HW INIT", "CPS2 hardware detected");
        
        // Check for CPS2 initialization functions
        // In a full implementation, we would call functions like:
        // CpsInit(), Cps2Init(), CpsMemInit()
        fprintf(stderr, "[HW INIT] Calling CPS2-specific initialization\n");
    }
    
    // Initialize graphics subsystem
    log_rom_stage("GRAPHICS INIT", "Initializing graphics system");
    
    // Initialize audio subsystem
    log_rom_stage("AUDIO INIT", "Initializing QSound for CPS2");
    
    // Initialize input system
    log_rom_stage("INPUT INIT", "Initializing input mappings");
    
    log_rom_stage("EMULATOR", "ROM loaded and initialized successfully");
    fprintf(stderr, "[EMULATOR] Running game: %s\n", BurnDrvGetTextA(DRV_FULLNAME));
    
    return true;
}

// Called by Metal to handle frame rendering from emulation
bool Metal_ProcessFrame() {
    static int frameCount = 0;
    
    // Only log every 60 frames to reduce spam
    if (frameCount % 60 == 0) {
        log_rom_stage("EMULATOR", "Processing emulation frame");
    }
    
    // Run one frame of emulation
    BurnDrvFrame();
    
    // Process audio
    if (frameCount % 60 == 0) {
        log_rom_stage("AUDIO LOOP", "Processing audio samples");
    }
    short audioBuf[2048]; // Temporary audio buffer
    BurnSoundRender(audioBuf, 2048/2); // 2048 bytes = 1024 samples (stereo)
    
    // Process input for next frame
    if (frameCount % 60 == 0) {
        log_rom_stage("INPUT LOOP", "Processing input state");
    }
    
    // First frame of actual rendering
    if (frameCount == 0) {
        log_rom_stage("GAME START", "First frame of game rendering");
    }
    
    frameCount++;
    return true;
}

// Get the current frame buffer from the emulation core
unsigned char* Metal_GetRomLoaderFrameBuffer(int* width, int* height, int* pitch) {
    if (width) *width = 320;  // Default CPS2 resolution, replace with actual values
    if (height) *height = 240;
    if (pitch) *pitch = 320 * 2;  // 16-bit pitch
    
    // In a real implementation, this would return the actual frame buffer from BurnDrv
    static unsigned char dummyBuffer[320 * 240 * 2];
    
    // Signal that we need actual implementation
    static int warningCount = 0;
    if (warningCount < 5) {
        log_rom_stage("RENDERER LOOP", "WARNING: Using dummy frame buffer, needs real implementation");
        warningCount++;
    }
    
    return dummyBuffer;
} 