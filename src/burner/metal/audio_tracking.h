#ifndef AUDIO_TRACKING_H
#define AUDIO_TRACKING_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char* name;       // Audio asset name
    int sampleRate;         // Sample rate in Hz
    int channels;           // Number of channels (1=mono, 2=stereo)
    int bitsPerSample;      // Bits per sample (8, 16)
    int size;               // Size in bytes
    int isLoaded;           // 1 if successfully loaded
    float duration;         // Duration in seconds
    uint32_t crc;             // CRC32 of the data for validation
} AudioAsset;

// Initialize audio tracking system
void Audio_Init(void);

// Configure audio system
void Audio_Configure(int sampleRate, int channels, int bitDepth, int bufferSize);

// Mark CoreAudio as initialized
void Audio_SetCoreAudioInitialized(bool initialized, float latencyMs);

// Mark QSound DSP as initialized
void Audio_SetQSoundInitialized(bool initialized);

// Mark audio mixer as initialized
void Audio_SetAudioMixerInitialized(bool initialized, int numChannels);

// Mark sound bank as loaded
void Audio_SetSoundBankLoaded(bool loaded, int numSounds);

// Mark FM synthesis as initialized
void Audio_SetFMSynthInitialized(bool initialized);

// Generate audio initialization report
void Audio_GenerateReport(void);

// Initialize audio components
void Audio_InitComponents(void);

// Register audio asset (stub to satisfy compilation)
int Audio_RegisterAsset(const char* name, int sampleRate, int channels, int bitDepth, 
                      int bufferSize, float duration, uint8_t* data);

// Track the loading of an audio asset
void AudioTracker_TrackLoading(int assetId, int success);

// Log info about all audio assets
void AudioTracker_LogAssets(void);

// Get info about a specific asset
AudioAsset* AudioTracker_GetAsset(int assetId);

// Track audio playback
void AudioTracker_TrackPlayback(int assetId, float volume, float pan);

// Track audio buffer statistics
void AudioTracker_TrackBufferStats(int bufferSize, int bufferUsed, int underruns, int overruns);

#ifdef __cplusplus
}
#endif

#endif // AUDIO_TRACKING_H 