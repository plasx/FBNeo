#ifndef _METAL_AUDIO_INTEGRATION_H_
#define _METAL_AUDIO_INTEGRATION_H_

// =============================================================================
// FBNeo Metal - Audio System Integration
// =============================================================================
// This header provides the interface between the Metal audio implementation
// and the FBNeo core
// =============================================================================

#include "fixes/metal_interop.h"
#include "fixes/metal_audio_bridge.h"

METAL_BEGIN_C_DECL

// -----------------------------------------------------------------------------
// Metal Audio API
// -----------------------------------------------------------------------------

// Initialize audio system
INT32 FBNeo_AudioInit();

// Process a frame of audio
INT32 FBNeo_AudioFrame();

// Clean up audio resources
void FBNeo_AudioExit();

// Pause/resume audio
void FBNeo_AudioPause(INT32 pause);

// Set audio volume (0.0 - 1.0)
void FBNeo_AudioSetVolume(float volume);

// Get current volume (0.0 - 1.0)
float FBNeo_AudioGetVolume();

// Helper to convert 0-100 integer volume to float (0.0-1.0)
void FBNeo_AudioSetVolumePercent(INT32 volumePercent);

// Get volume as percentage (0-100)
INT32 FBNeo_AudioGetVolumePercent();

// Get CPU usage as percentage (0.0 - 1.0)
float FBNeo_AudioGetCPUUsage();

// Get buffer fill percentage (0.0 - 1.0)
float FBNeo_AudioGetBufferFill();

// Check if audio system is initialized
INT32 FBNeo_AudioIsInitialized();

// Main audio rendering function that should be called each frame
INT32 Metal_ProcessAudioFrame(short* pSoundBuf, INT32 nSegmentLength);

// Audio synchronization functions
void Metal_AudioSetSampleRate(INT32 sampleRate);
void Metal_AudioSetBufferSize(INT32 bufferSize);
float Metal_GetAudioLatency();
void Metal_SetAudioSync(INT32 syncMode);
INT32 Metal_GetAudioSync();

// C ABI compatibility
#ifdef __cplusplus
extern "C" {
#endif

// C wrapper functions
INT32 Metal_InitAudio();
INT32 Metal_ExitAudio();
INT32 Metal_PauseAudio(INT32 pause);
INT32 Metal_ResumeAudio();
INT32 Metal_SetAudioVolume(float volume);
float Metal_GetAudioVolume();
INT32 Metal_AudioFrame();
INT32 Metal_SyncAudio();
INT32 Metal_GetAudioInfo(INT32* sampleRate, INT32* bufferSize, float* latency, float* cpuUsage);

#ifdef __cplusplus
}
#endif

METAL_END_C_DECL

#endif // _METAL_AUDIO_INTEGRATION_H_ 