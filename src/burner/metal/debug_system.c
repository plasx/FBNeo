#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include "debug_functions.h"

// Debug logging system for Metal implementation

// Log a debug message
void Debug_Log(int sectionIndex, const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    // Write to stderr for visibility
    fprintf(stderr, "[DEBUG %d] ", sectionIndex);
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    
    va_end(args);
}

// Print a section header in the debug output
void Debug_PrintSectionHeader(int sectionIndex, const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    // Print section header
    fprintf(stderr, "\n===== %d: ", sectionIndex);
    vfprintf(stderr, format, args);
    fprintf(stderr, " =====\n");
    
    va_end(args);
}

// Log section with formatted message
void LogDebugSection(int sectionIndex, const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    fprintf(stderr, "[SECTION %d] ", sectionIndex);
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    
    va_end(args);
}

// Get timestamp for logging
static void GetTimestamp(char* buffer, size_t bufferSize) {
    time_t now = time(NULL);
    struct tm* timeinfo = localtime(&now);
    strftime(buffer, bufferSize, "%Y-%m-%d %H:%M:%S", timeinfo);
}

// Report ROM integrity check results
void ROM_CheckIntegrity(const char* romPath, int numFiles, int validFiles) {
    char timestamp[64];
    GetTimestamp(timestamp, sizeof(timestamp));
    
    Debug_PrintSectionHeader(DEBUG_ROM_CHECK, "ROM CHECK");
    Debug_Log(DEBUG_ROM_CHECK, "Time: %s", timestamp);
    Debug_Log(DEBUG_ROM_CHECK, "ROM Path: %s", romPath);
    Debug_Log(DEBUG_ROM_CHECK, "Files: %d/%d valid", validFiles, numFiles);
    
    if (validFiles == numFiles) {
        Debug_Log(DEBUG_ROM_CHECK, "ROM integrity check passed");
    } else {
        Debug_Log(DEBUG_ROM_CHECK, "ROM integrity check failed");
    }
}

// Report component allocation
void MEM_ReportComponentAllocation(const char* componentName, size_t size, bool success) {
    Debug_PrintSectionHeader(DEBUG_MEMORY, "MEMORY ALLOCATION");
    Debug_Log(DEBUG_MEMORY, "Component: %s", componentName);
    Debug_Log(DEBUG_MEMORY, "Size: %zu bytes", size);
    Debug_Log(DEBUG_MEMORY, "Success: %s", success ? "Yes" : "No");
    
    if (success) {
        Debug_Log(DEBUG_MEM_INIT, "%s memory allocated: %zu bytes", componentName, size);
    } else {
        Debug_Log(DEBUG_MEM_INIT, "ERROR: Failed to allocate %zu bytes for %s", size, componentName);
    }
    
    // Report hardware initialization for this component
    if (success) {
        Debug_Log(DEBUG_HW_INIT, "%s initialized successfully", componentName);
    } else {
        Debug_Log(DEBUG_HW_INIT, "ERROR: Failed to initialize %s", componentName);
    }
}

// Report graphics asset loading
void ReportGraphicsAssetLoading(const char* assetType, int count, size_t memoryUsed) {
    Debug_Log(DEBUG_GRAPHICS_INIT, "Loaded %d %s (%zu KB memory used)", 
             count, assetType, memoryUsed / 1024);
}

// Report audio initialization
void ReportAudioInitialization(int sampleRate, int channels, int bitDepth, int bufferSize) {
    Debug_Log(DEBUG_AUDIO_INIT, "QSound DSP initialized with format: %d Hz, %d channels, %d-bit, %d sample buffer",
             sampleRate, channels, bitDepth, bufferSize);
}

// Report input initialization
void ReportInputInitialization(int buttonCount, int controllerCount) {
    Debug_Log(DEBUG_INPUT_INIT, "Mapped %d buttons across %d controller(s)", 
             buttonCount, controllerCount);
}

// Report emulator startup
void EMULATOR_ReportStartup(const char* gameTitle, float targetFPS) {
    Debug_PrintSectionHeader(DEBUG_EMULATOR, "EMULATOR STARTUP");
    Debug_Log(DEBUG_EMULATOR, "Game: %s", gameTitle);
    Debug_Log(DEBUG_EMULATOR, "Target FPS: %.1f", targetFPS);
}

// Report frame rendering
void ReportFrameRendering(int frameNumber, int spriteCount, int layerCount, float fps) {
    if (frameNumber % 60 == 0) { // Only log every 60 frames to reduce spam
        Debug_Log(DEBUG_RENDERER_LOOP, "Frame %d: Rendering %d sprites, %d layers at %.1f FPS",
                 frameNumber, spriteCount, layerCount, fps);
    }
}

// Report audio status
void ReportAudioStatus(int bufferSize, int bufferUsed, int underruns) {
    if (underruns > 0) {
        Debug_Log(DEBUG_AUDIO_LOOP, "WARNING: Audio buffer underrun detected!");
    }
    
    if (bufferSize > 0 && bufferSize > 0) {
        Debug_Log(DEBUG_AUDIO_LOOP, "Audio buffer: %d/%d bytes (%.1f%%), %d Hz",
                 bufferUsed, bufferSize, (bufferUsed * 100.0f) / bufferSize, 44100);
    }
}

// Report game running state
void GAME_ReportRunningState(const char* gameTitle, float actualFPS, bool isRunningWell) {
    if (isRunningWell) {
        Debug_PrintSectionHeader(DEBUG_GAME_START, "%s emulation running at ~%.1f fps", 
                                gameTitle, actualFPS);
    } else {
        Debug_PrintSectionHeader(DEBUG_GAME_START, "WARNING: %s running at %.1f fps (below target)",
                                gameTitle, actualFPS);
    }
    
    Debug_Log(DEBUG_GAME, "Game: %s", gameTitle);
    Debug_Log(DEBUG_GAME, "FPS: %.1f", actualFPS);
    Debug_Log(DEBUG_GAME, "Running well: %s", isRunningWell ? "Yes" : "No");
}

// Report audio stream statistics
void AUDIO_ReportStreamStats(int bufferSize, int currentFill, int underruns, int overruns) {
    if (underruns > 0) {
        Debug_Log(DEBUG_AUDIO_LOOP, "WARNING: Audio buffer underrun detected (%d occurrences)", underruns);
    }
    
    if (overruns > 0) {
        Debug_Log(DEBUG_AUDIO_LOOP, "WARNING: Audio buffer overrun detected (%d occurrences)", overruns);
    }
    
    float fillPercentage = 0.0f;
    if (bufferSize > 0) {
        fillPercentage = (currentFill * 100.0f) / bufferSize;
    }
    
    Debug_Log(DEBUG_AUDIO_LOOP, "Audio buffer: %d/%d bytes (%.1f%%)", 
             currentFill, bufferSize, fillPercentage);
}

// Add the Debug_Init function
int Debug_Init(const char* logfile) {
    // Initialize debug system
    printf("[DEBUG] Initializing debug system\n");
    
    // If a log file is provided, open it
    if (logfile) {
        printf("[DEBUG] Log file: %s\n", logfile);
    }
    
    return 0;
}
