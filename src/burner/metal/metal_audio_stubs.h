#ifndef METAL_AUDIO_STUBS_H
#define METAL_AUDIO_STUBS_H

#ifdef __cplusplus
extern "C" {
#endif

// Initialize the audio system
INT32 Metal_InitAudio();

// Shutdown the audio system
INT32 Metal_ExitAudio();

// Update audio for current frame
void Metal_UpdateAudio();

// Print audio status for debugging
void Metal_PrintAudioStatus();

// Enable or disable audio output
void Metal_SetAudioEnabled(bool enabled);

// Set audio volume (0.0 to 1.0)
void Metal_SetAudioVolume(float volume);

// Get current audio volume
float Metal_GetAudioVolume();

// Check if audio is initialized
bool Metal_IsAudioInitialized();

// Render audio for a specific number of samples
INT32 Metal_RenderAudio(INT16* buffer, INT32 samples);

// Get estimated audio latency in milliseconds
float Metal_GetAudioLatency();

#ifdef __cplusplus
}
#endif

#endif // METAL_AUDIO_STUBS_H 