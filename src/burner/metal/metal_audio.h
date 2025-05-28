#ifndef METAL_AUDIO_H
#define METAL_AUDIO_H

#include <stdint.h>
#include <stdbool.h>
#include "metal_common.h"

// FBNeo Metal Audio Interface Functions

// Define AudSoundCallbackFunction here to avoid dependency
typedef int (*AudSoundCallbackFunction)(int);

// Update InterfaceInfo structure
typedef struct {
    int nAudSampleRate;
    int nAudVolume;
    bool bAudOkay;
    int sampleRate;
    int channels;
    float volume;
    bool isInitialized;
    bool isPaused;
} InterfaceInfo;

#ifdef __cplusplus
extern "C" {
#endif

// Ring buffer function declarations
int RingBufferFree();
int RingBufferAvailable();
void RingBufferWrite(const short* data, int samples);
int RingBufferRead(short* data, int samples);

// Initialize the audio system
int Metal_AudioInit(int sampleRate, int channels);

// Start audio playback
int Metal_AudioStart();

// Stop audio playback
int Metal_AudioStop();

// Clean up audio resources
int Metal_AudioExit();

// Set audio volume (0.0 - 1.0)
int Metal_SetAudioVolume(float volume);

// Pause/resume audio
int Metal_PauseAudio(int pause);

// Reset the audio system
int Metal_AudioReset();

// Get current audio state
bool Metal_AudioIsRunning();

// Audio callback function
void Metal_AudioCallback(INT16 *data, INT32 samples);

// Set audio callback
int Metal_SetAudioCallback(AudSoundCallbackFunction callback);

// Audio processing
int Metal_AddAudioSamples(const int16_t* samples, int count);
int Metal_ProcessAudioFrame();

// Audio state
bool Metal_IsAudioInitialized();
float Metal_GetAudioBufferFillPercentage();

// Update function declarations to match implementations
int Metal_ShutdownAudio();

#ifdef __cplusplus
}
#endif

#endif // METAL_AUDIO_H 