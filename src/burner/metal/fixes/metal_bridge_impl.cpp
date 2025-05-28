#include "metal_bridge.h"
#include <stdarg.h>
#include <stdio.h>

// =============================================================================
// FBNeo Metal - Bridge Implementation
// =============================================================================
// This file implements the C interface functions that bridge between the 
// Metal implementation and the FBNeo core.
// =============================================================================

// Global state tracking
static struct {
    bool initialized = false;
    bool paused = false;
} metalState;

// -----------------------------------------------------------------------------
// Global Control Implementation
// -----------------------------------------------------------------------------

// Implementation of global functions declared in metal_bridge.h
METAL_EXPORT_TO_C INT32 Metal_Initialize() {
    if (metalState.initialized) {
        return 0; // Already initialized
    }
    
    // Initialize all subsystems
    INT32 result = 0;
    
    // Try to initialize audio
    result = FBNeo_AudioInit();
    if (result != 0) {
        Metal_ReportError("Failed to initialize audio system");
        return result;
    }
    
    // Try to initialize input
    result = Metal_InputInit();
    if (result != 0) {
        Metal_ReportError("Failed to initialize input system");
        FBNeo_AudioExit(); // Clean up audio since it was successful
        return result;
    }
    
    // Set initialized state
    metalState.initialized = true;
    metalState.paused = false;
    
    return 0;
}

METAL_EXPORT_TO_C void Metal_Shutdown() {
    if (!metalState.initialized) {
        return;
    }
    
    // Shut down audio
    FBNeo_AudioExit();
    
    // Shut down input
    Metal_InputExit();
    
    // Reset state
    metalState.initialized = false;
    metalState.paused = false;
}

METAL_EXPORT_TO_C INT32 Metal_ProcessFrame() {
    if (!metalState.initialized || metalState.paused) {
        return 1; // Not initialized or paused
    }
    
    // Process input first
    Metal_InputUpdate();
    
    // Map inputs to CPS arrays
    Metal_MapInputsToCPS();
    
    // Process audio frame
    FBNeo_AudioFrame();
    
    return 0;
}

METAL_EXPORT_TO_C void Metal_Pause(INT32 pauseState) {
    metalState.paused = (pauseState != 0);
    
    // Pause audio
    FBNeo_AudioPause(pauseState);
}

METAL_EXPORT_TO_C INT32 Metal_IsActive() {
    return metalState.initialized ? 1 : 0;
}

// -----------------------------------------------------------------------------
// Error Reporting Implementation
// -----------------------------------------------------------------------------

METAL_EXPORT_TO_C void Metal_ReportError(const char* message) {
    // Simple implementation that prints to stderr
    fprintf(stderr, "Metal Error: %s\n", message);
}

METAL_EXPORT_TO_C void Metal_LogDebug(const char* format, ...) {
    // Variable argument debug logging
    va_list args;
    va_start(args, format);
    
    fprintf(stdout, "Metal Debug: ");
    vfprintf(stdout, format, args);
    fprintf(stdout, "\n");
    
    va_end(args);
}

// -----------------------------------------------------------------------------
// Audio Bridge Implementation
// -----------------------------------------------------------------------------

// Implementation of functions that connect to the audio framework
METAL_EXPORT_TO_C INT32 AudSoundInit() {
    return FBNeo_AudioInit();
}

METAL_EXPORT_TO_C INT32 AudSoundExit() {
    FBNeo_AudioExit();
    return 0;
}

METAL_EXPORT_TO_C INT32 AudSoundPlay() {
    FBNeo_AudioPause(0); // Resume
    return 0;
}

METAL_EXPORT_TO_C INT32 AudSoundStop() {
    FBNeo_AudioPause(1); // Pause
    return 0;
}

METAL_EXPORT_TO_C INT32 AudSoundSetVolume(INT32 nVolume) {
    FBNeo_AudioSetVolumePercent(nVolume);
    return 0;
}

METAL_EXPORT_TO_C INT32 AudSetCallback(INT32 (*pCallback)(INT32)) {
    // We don't use callbacks in our implementation
    return 0;
} 