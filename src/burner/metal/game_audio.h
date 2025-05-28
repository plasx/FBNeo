#ifndef _METAL_GAME_AUDIO_H_
#define _METAL_GAME_AUDIO_H_

#ifdef __cplusplus
extern "C" {
#endif

// Number of audio buffers to use
#define AUDIO_NUM_BUFFERS 3

// Audio system for FBNeo Metal implementation

// Initialize the audio system
int GameAudio_Init(int sampleRate, int bufferSize, int channels);

// Shutdown the audio system
int GameAudio_Exit();

// Enable/disable audio
int GameAudio_SetEnabled(bool enabled);

// Set audio volume
void GameAudio_SetVolume(float volume);

// Update the audio buffer with new data
int GameAudio_UpdateBuffer(const short* data, int size);

// Check if audio is enabled
bool GameAudio_IsEnabled();

// Get the current audio volume
int GameAudio_GetVolume();

// Get the audio buffer size
int GameAudio_GetBufferSize();

// Get the sample rate
int GameAudio_GetSampleRate();

// Reset audio state
void GameAudio_Reset();

// Get audio buffer for rendering
void GameAudio_GetBuffer(short *buffer, int samplesRequested);

// Reconfigure audio parameters
void GameAudio_Reconfigure(int newSampleRate, int newBufferSize);

#ifdef __cplusplus
}
#endif

#endif // _METAL_GAME_AUDIO_H_ 