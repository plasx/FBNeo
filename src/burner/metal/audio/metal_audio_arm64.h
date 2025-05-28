#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// ARM64-specific Metal audio implementation for FBNeo
// This implementation uses AVAudioEngine and is optimized for Apple Silicon

// Core functions

// Initialize the audio system
// Returns 0 on success, non-zero on failure
int Metal_AudioInit();

// Process a frame's worth of audio from FBNeo core
// Returns 0 on success, non-zero on failure
int Metal_AudioFrame();

// Shutdown the audio system
void Metal_ShutdownAudio();

// Audio control functions

// Update with new audio samples
// samples: pointer to buffer of audio data (typically INT16 samples)
// numSamples: number of sample frames (not bytes)
// channels: number of audio channels (typically 2 for stereo)
// sampleRate: audio sample rate in Hz
void Metal_UpdateAudio(const void* samples, int numSamples, int channels, int sampleRate);

// Pause/resume audio playback
// pause: non-zero to pause, 0 to resume
void Metal_PauseAudio(int pause);

// Set volume level (0.0 - 1.0)
void Metal_SetAudioVolume(float volume);

// Performance monitoring functions

// Get current audio CPU usage as a percentage (0.0 - 1.0)
float Metal_GetAudioCPUUsage();

// Get current buffer fill level (0.0 - 1.0)
float Metal_GetBufferFillLevel();

#ifdef __cplusplus
} // extern "C"
#endif 