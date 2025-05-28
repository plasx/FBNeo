#ifndef AUDIO_LOOP_MONITOR_H
#define AUDIO_LOOP_MONITOR_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Maximum number of audio streams to track
#define MAX_AUDIO_STREAMS 8

// Initialize audio loop monitor
void AudioLoop_Init(void);

// Configure audio buffer size and sample rate
void AudioLoop_Configure(int bufferSize, int sampleRate);

// Start audio loop
void AudioLoop_Start(void);

// Stop audio loop
void AudioLoop_Stop(void);

// Update buffer fill level
void AudioLoop_UpdateBufferFill(int currentFill, bool underrun);

// Register an audio stream
int AudioLoop_RegisterStream(const char* name);

// Update stream statistics
void AudioLoop_UpdateStream(int streamId, float volume, float bufferFill, int samplesPlayed);

// Report audio loop statistics
void AudioLoop_ReportStats(float elapsedTime);

// Initialize and generate a report
void AudioLoop_InitAndGenerateReport(void);

#ifdef __cplusplus
}
#endif

#endif // AUDIO_LOOP_MONITOR_H 