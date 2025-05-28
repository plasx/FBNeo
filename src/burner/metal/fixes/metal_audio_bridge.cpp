#include "metal_bridge.h"
#include "../metal_audio_integration.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>  // For usleep

// =============================================================================
// FBNeo Metal - Audio Bridge Implementation
// =============================================================================
// This file implements the audio bridge functions that connect the FBNeo core
// with the Metal audio implementation.
// =============================================================================

// Required globals that FBNeo core expects
extern INT16* pBurnSoundOut = NULL;   // Sound output buffer
extern INT32 nBurnSoundLen = 0;       // Sound buffer length
extern INT32 nBurnSoundRate = 44100;  // Sound sample rate
extern INT32 nBurnSoundActive = 0;    // Sound is active
extern INT32 nAudNextSound = 0;       // What sound segment to start next

// Local globals for this implementation
static INT16* pAudioBuffer = NULL;
static INT32 nAudioBufferSize = 0;
static INT32 nAudioVolume = 100;
static INT32 bAudioInitialized = 0;

// -----------------------------------------------------------------------------
// C ABI Audio Bridge Functions
// -----------------------------------------------------------------------------

// Initialize the sound interface
INT32 BurnSoundInit() {
    // Log initialization
    Metal_LogDebug("BurnSoundInit() called");
    
    // If already initialized, exit previous instance
    if (bAudioInitialized) {
        BurnSoundExit();
    }
    
    // Call through to Metal audio initialization
    INT32 result = FBNeo_AudioInit();
    if (result != 0) {
        Metal_ReportError("Failed to initialize audio system");
        return result;
    }
    
    // Allocate buffer for audio mixing
    nAudioBufferSize = nBurnSoundLen * 2 * sizeof(INT16); // Stereo
    pAudioBuffer = (INT16*)malloc(nAudioBufferSize);
    if (!pAudioBuffer) {
        Metal_ReportError("Failed to allocate audio buffer");
        FBNeo_AudioExit();
        return 1;
    }
    
    // Clear buffer
    memset(pAudioBuffer, 0, nAudioBufferSize);
    
    // Set buffer pointer for FBNeo core
    pBurnSoundOut = pAudioBuffer;
    
    // Set active flag
    nBurnSoundActive = 1;
    bAudioInitialized = 1;
    
    // Set the volume
    BurnSoundSetVolume(nAudioVolume);
    
    Metal_LogDebug("BurnSoundInit() completed successfully");
    return 0;
}

// Exit the sound interface
INT32 BurnSoundExit() {
    Metal_LogDebug("BurnSoundExit() called");
    
    // Call through to Metal audio exit
    FBNeo_AudioExit();
    
    // Free buffer if allocated
    if (pAudioBuffer) {
        free(pAudioBuffer);
        pAudioBuffer = NULL;
    }
    
    // Reset pointers and state
    pBurnSoundOut = NULL;
    nBurnSoundActive = 0;
    bAudioInitialized = 0;
    
    return 0;
}

// Play the sound
INT32 BurnSoundPlay() {
    // Set active flag
    nBurnSoundActive = 1;
    
    // Resume audio playback
    FBNeo_AudioPause(0);
    
    return 0;
}

// Stop the sound
INT32 BurnSoundStop() {
    // Clear active flag
    nBurnSoundActive = 0;
    
    // Pause audio playback
    FBNeo_AudioPause(1);
    
    return 0;
}

// Check if sound is active and working
INT32 BurnSoundCheck() {
    return bAudioInitialized && nBurnSoundActive;
}

// Set the audio volume
INT32 BurnSoundSetVolume(INT32 nVol) {
    // Store the volume value (0-100)
    nAudioVolume = nVol;
    
    // Convert to float (0.0-1.0) and set via Metal audio API
    FBNeo_AudioSetVolumePercent(nVol);
    
    return 0;
}

// Render sound samples
// This is called by FBNeo core to render audio for a frame
INT32 BurnSoundRender(INT16* pSoundBuf, INT32 nSegmentLength) {
    // If sound is not active, clear buffer and return
    if (!nBurnSoundActive) {
        memset(pSoundBuf, 0, nSegmentLength * 2 * sizeof(INT16)); // Stereo
        return 0;
    }
    
    // Process audio frame through Metal audio system
    INT32 result = Metal_ProcessAudioFrame(pSoundBuf, nSegmentLength);
    
    // Update next sound segment
    nAudNextSound += nSegmentLength;
    
    return result;
}

// -----------------------------------------------------------------------------
// Metal Audio Bridge Functions
// -----------------------------------------------------------------------------

// Initialize Metal audio system
INT32 Metal_AudioInit() {
    // Forward to FBNeo_AudioInit implementation
    return FBNeo_AudioInit();
}

// Clean up Metal audio system
INT32 Metal_ExitAudio() {
    // Forward to FBNeo_AudioExit implementation
    FBNeo_AudioExit();
    return 0;
}

// Pause/resume Metal audio playback
INT32 Metal_PauseAudio(INT32 pause) {
    // Forward to FBNeo_AudioPause implementation
    FBNeo_AudioPause(pause);
    return 0;
}

// Resume Metal audio playback
INT32 Metal_ResumeAudio() {
    // Just unpause audio
    FBNeo_AudioPause(0);
    return 0;
}

// Set Metal audio volume
INT32 Metal_SetAudioVolume(float volume) {
    // Forward to FBNeo_AudioSetVolume implementation
    FBNeo_AudioSetVolume(volume);
    return 0;
}

// Get Metal audio volume
float Metal_GetAudioVolume() {
    // Forward to FBNeo_AudioGetVolume implementation
    return FBNeo_AudioGetVolume();
}

// Process a frame of audio
INT32 Metal_AudioFrame() {
    // Forward to FBNeo_AudioFrame implementation
    return FBNeo_AudioFrame();
}

// Sync audio with video
INT32 Metal_SyncAudio() {
    // Get current audio sync mode
    INT32 syncMode = Metal_GetAudioSync();
    
    // No sync needed if mode is 0 (disabled)
    if (syncMode == 0) {
        return 0;
    }
    
    // Get current audio buffer fill level
    float bufferFill = FBNeo_AudioGetBufferFill();
    float latencyMs = Metal_GetAudioLatency();
    
    // Sync strategies based on mode
    if (syncMode == 1) { // Light sync
        // Only sync if buffer is significantly low or high
        if (bufferFill < 0.2f) {
            // Buffer underrun risk - slow down slightly
            usleep(1000); // Small delay
            return 1;
        } else if (bufferFill > 0.8f) {
            // Buffer overrun risk - no delay, let it catch up
            return 2;
        }
    } else if (syncMode == 2) { // Full sync
        // Target 50% buffer fill for optimal latency/stability balance
        float targetFill = 0.5f;
        float deviation = bufferFill - targetFill;
        
        if (deviation < -0.1f) {
            // Buffer too empty, add delay proportional to deviation
            int delayUs = (int)(-deviation * 5000.0f);
            if (delayUs > 0) {
                usleep(delayUs);
                return 1;
            }
        } else if (deviation > 0.1f) {
            // Buffer too full, no delay but signal to caller
            return 2;
        }
    }
    
    // No sync needed
    return 0;
}

// Get audio information
INT32 Metal_GetAudioInfo(INT32* sampleRate, INT32* bufferSize, float* latency, float* cpuUsage) {
    if (sampleRate) *sampleRate = nBurnSoundRate;
    if (bufferSize) *bufferSize = nBurnSoundLen;
    if (latency) *latency = Metal_GetAudioLatency();
    if (cpuUsage) *cpuUsage = FBNeo_AudioGetCPUUsage();
    
    return 0;
} 