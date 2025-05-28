#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Initialize Metal audio system
int Metal_InitAudio();

// Update audio with new samples
void Metal_UpdateAudio(const void* samples, int numSamples, int channels, int sampleRate);

// Shutdown audio system
void Metal_ShutdownAudio();

// Audio control functions
void Metal_PauseAudio(int pause);
void Metal_SetAudioVolume(float volume);

#ifdef __cplusplus
}
#endif 