#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>
#include <time.h>
#include "debug_controller.h"
#include "debug_system.h"
#include "audio_loop_monitor.h"

// Audio loop state constants
#define AUDIO_FRAME_SIZE 2048
#define AUDIO_BUFFER_COUNT 3
#define AUDIO_SAMPLE_RATE 44100
#define AUDIO_CHANNELS 2
#define AUDIO_MAX_STREAMS 8
#define AUDIO_STATS_UPDATE_INTERVAL_MS 1000

// Audio stream states
typedef enum {
    AUDIO_STREAM_INACTIVE,
    AUDIO_STREAM_PLAYING,
    AUDIO_STREAM_PAUSED,
    AUDIO_STREAM_ERROR
} AudioStreamState;

// Audio stream statistics
typedef struct {
    int id;
    AudioStreamState state;
    uint64_t totalSamplesPlayed;
    uint64_t lastSamplesPlayed;
    float currentVolume;
    float bufferFullness;
    int underruns;
    int overruns;
    char name[64];
} AudioStreamStats;

// Audio loop statistics
typedef struct {
    bool initialized;
    bool isRunning;
    int activeStreams;
    int sampleRate;
    int channels;
    int bitsPerSample;
    int bufferSize;
    int bufferCount;
    float cpuUsage;
    float bufferFullness;
    int underruns;
    int overruns;
    struct timespec startTime;
    struct timespec lastUpdateTime;
    AudioStreamStats streams[AUDIO_MAX_STREAMS];
    pthread_mutex_t mutex;
} AudioLoopStats;

// Global audio loop stats - marked as possibly unused to suppress warnings
__attribute__((unused)) static AudioLoopStats g_audioLoopStats;

// Audio stream statistics
typedef struct {
    char name[32];
    float volume;       // 0.0 to 1.0
    float bufferFill;   // 0.0 to 1.0
    int samplesPlayed;
    bool active;
} AudioStream;

// Audio loop state
typedef struct {
    int bufferFill;
    int bufferSize;
    int sampleRate;
    float elapsedTime;
    bool underrun;
    bool initialized;
    bool active;
    AudioStream streams[MAX_AUDIO_STREAMS];
} AudioLoopState;

static AudioLoopState g_audioLoopState = {0};

// Initialize audio loop monitor
void AudioLoop_Init(void) {
    printf("AudioLoop_Init: Initializing audio loop monitor\n");
    
    // Initialize state
    g_audioLoopState.bufferFill = 0;
    g_audioLoopState.bufferSize = 0;
    g_audioLoopState.sampleRate = 44100;
    g_audioLoopState.elapsedTime = 0.0f;
    g_audioLoopState.underrun = false;
    g_audioLoopState.initialized = true;
    g_audioLoopState.active = false;
    
    // Clear streams
    memset(g_audioLoopState.streams, 0, sizeof(g_audioLoopState.streams));
    
    // Debug output
    Debug_Log(DEBUG_AUDIO_LOOP, "Audio loop monitor initialized");
}

// Configure audio buffer size and sample rate
void AudioLoop_Configure(int bufferSize, int sampleRate) {
    if (!g_audioLoopState.initialized) {
        AudioLoop_Init();
    }
    
    g_audioLoopState.bufferSize = bufferSize;
    g_audioLoopState.sampleRate = sampleRate;
    
    printf("AudioLoop_Configure: Buffer size=%d, sample rate=%d\n", bufferSize, sampleRate);
}

// Start audio loop
void AudioLoop_Start(void) {
    if (!g_audioLoopState.initialized) {
        AudioLoop_Init();
    }
    
    g_audioLoopState.active = true;
    
    // Debug output
    Debug_PrintSectionHeader(DEBUG_AUDIO_LOOP, "Audio streaming activated (CoreAudio backend).");
    
    printf("AudioLoop_Start: Audio streaming activated\n");
}

// Stop audio loop
void AudioLoop_Stop(void) {
    if (!g_audioLoopState.initialized || !g_audioLoopState.active) {
        return;
    }
    
    g_audioLoopState.active = false;
    
    // Debug output
    Debug_Log(DEBUG_AUDIO_LOOP, "Audio streaming deactivated");
    
    printf("AudioLoop_Stop: Audio streaming deactivated\n");
}

// Update buffer fill level
void AudioLoop_UpdateBufferFill(int currentFill, bool underrun) {
    if (!g_audioLoopState.initialized) {
        AudioLoop_Init();
    }
    
    g_audioLoopState.bufferFill = currentFill;
    g_audioLoopState.underrun = underrun;
    
    // Report stats to debug
    AUDIO_ReportStreamStats(currentFill, g_audioLoopState.bufferSize, 
                          g_audioLoopState.sampleRate, underrun);
}

// Register an audio stream
int AudioLoop_RegisterStream(const char* name) {
    if (!g_audioLoopState.initialized) {
        AudioLoop_Init();
    }
    
    // Find free slot
    for (int i = 0; i < MAX_AUDIO_STREAMS; i++) {
        if (!g_audioLoopState.streams[i].active) {
            strncpy(g_audioLoopState.streams[i].name, name, sizeof(g_audioLoopState.streams[i].name) - 1);
            g_audioLoopState.streams[i].name[sizeof(g_audioLoopState.streams[i].name) - 1] = '\0';
            g_audioLoopState.streams[i].volume = 1.0f;
            g_audioLoopState.streams[i].bufferFill = 0.0f;
            g_audioLoopState.streams[i].samplesPlayed = 0;
            g_audioLoopState.streams[i].active = true;
            return i;
        }
    }
    
    return -1;
}

// Update stream statistics
void AudioLoop_UpdateStream(int streamId, float volume, float bufferFill, int samplesPlayed) {
    if (!g_audioLoopState.initialized || streamId < 0 || streamId >= MAX_AUDIO_STREAMS) {
        return;
    }
    
    if (!g_audioLoopState.streams[streamId].active) {
        return;
    }
    
    g_audioLoopState.streams[streamId].volume = volume;
    g_audioLoopState.streams[streamId].bufferFill = bufferFill;
    g_audioLoopState.streams[streamId].samplesPlayed = samplesPlayed;
}

// Report audio loop statistics
void AudioLoop_ReportStats(float elapsedTime) {
    if (!g_audioLoopState.initialized || !g_audioLoopState.active) {
        return;
    }
    
    g_audioLoopState.elapsedTime = elapsedTime;
    
    // Basic stats
    Debug_Log(DEBUG_AUDIO_LOOP, "Audio streaming stats (%.1f seconds elapsed):", elapsedTime);
    
    // Buffer info
    float bufferPercentage = 0.0f;
    if (g_audioLoopState.bufferSize > 0) {
        bufferPercentage = (float)g_audioLoopState.bufferFill / g_audioLoopState.bufferSize * 100.0f;
    }
    
    Debug_Log(DEBUG_AUDIO_LOOP, "Audio buffer: %d/%d bytes (%.1f%%), %d Hz",
             g_audioLoopState.bufferFill, g_audioLoopState.bufferSize, 
             bufferPercentage, g_audioLoopState.sampleRate);
    
    // Stream info
    for (int i = 0; i < MAX_AUDIO_STREAMS; i++) {
        if (g_audioLoopState.streams[i].active) {
            Debug_Log(DEBUG_AUDIO_LOOP, "Stream %d (%s): %.1f%% volume, %.1f%% buffer, %d samples played",
                     i, g_audioLoopState.streams[i].name,
                     g_audioLoopState.streams[i].volume * 100.0f,
                     g_audioLoopState.streams[i].bufferFill * 100.0f,
                     g_audioLoopState.streams[i].samplesPlayed);
        }
    }
}

// Initialize and generate a report
void AudioLoop_InitAndGenerateReport(void) {
    printf("AudioLoop_InitAndGenerateReport: Setting up audio loop monitor\n");
    
    // Initialize audio loop monitor
    AudioLoop_Init();
    
    // Configure with standard values
    AudioLoop_Configure(2048, 44100);
    
    // Start audio streaming
    AudioLoop_Start();
    
    // Register some example streams
    int stream1 = AudioLoop_RegisterStream("Stream 0");
    int stream2 = AudioLoop_RegisterStream("Stream 1");
    
    // Set some example stats
    AudioLoop_UpdateBufferFill(1740, false);
    AudioLoop_UpdateStream(stream1, 0.8f, 0.9f, 44100);
    AudioLoop_UpdateStream(stream2, 1.0f, 0.7f, 22050);
    
    // Report stats
    AudioLoop_ReportStats(0.0f);
    
    printf("AudioLoop_InitAndGenerateReport: Report generated successfully\n");
}
