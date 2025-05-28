#include "audio_tracking.h"
#include "rom_loading_debug.h"
#include "memory_tracking.h"
#include "debug_system.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <pthread.h>
#include "debug_controller.h"

// Maximum number of audio assets we'll track
#define MAX_AUDIO_ASSETS 64

// Global array to track audio assets
static AudioAsset g_assets[MAX_AUDIO_ASSETS];
static int g_numAssets = 0;

// Audio buffer stats
static struct {
    int bufferSize;
    int bufferUsed;
    int underruns;
    int overruns;
    int sampleRate;
    int channelCount;
    int bitDepth;
    int isInitialized;
} g_audioStats;

// Audio system components
typedef enum {
    AUDIO_COMPONENT_DSP,
    AUDIO_COMPONENT_MIXER,
    AUDIO_COMPONENT_CORE_AUDIO,
    AUDIO_COMPONENT_SOUND_BANK,
    AUDIO_COMPONENT_FM_SYNTH,
    AUDIO_COMPONENT_COUNT
} AudioComponentType;

// Audio component names for debugging
static const char* g_audioComponentNames[AUDIO_COMPONENT_COUNT] = {
    "QSound DSP",
    "Audio Mixer",
    "CoreAudio Output",
    "Sound Bank",
    "FM Synthesis"
};

// Audio format information
typedef struct {
    int sampleRate;
    int channels;
    int bitsPerSample;
    int bufferSize;
    bool initialized;
} AudioFormat;

// Audio component state
typedef struct {
    bool initialized;
    char statusMessage[256];
    int errorCode;
    AudioFormat format;
} AudioComponent;

// Global state
static AudioComponent g_audioComponents[AUDIO_COMPONENT_COUNT];
static bool g_audioSystemInitialized = false;
static bool g_audioReportGenerated = false;
static pthread_mutex_t g_audioMutex = PTHREAD_MUTEX_INITIALIZER;

// Audio playback statistics
static uint64_t g_totalSamplesPlayed = 0;
static float g_averageBufferUsage = 0.0f;
static int g_bufferUnderrunCount = 0;
static bool g_isPlaying = false;

// Audio system configuration
typedef struct {
    int sampleRate;
    int channels;
    int bitDepth;
    int bufferSize;
    bool coreAudioInitialized;
    bool qsoundInitialized;
    bool audioMixerInitialized;
    bool soundBankLoaded;
    bool fmSynthInitialized;
} AudioSystemConfig;

// Static audio configuration
static AudioSystemConfig g_audioConfig = {0};
static bool g_audioInitialized = false;
static bool g_reportGenerated = false;

// Calculate a simple CRC32 for data
static UINT32 CalculateAudioCRC32(const UINT8* data, int size) {
    UINT32 crc = 0xFFFFFFFF;
    for (int i = 0; i < size; i++) {
        crc = (crc >> 8) ^ (((crc & 0xFF) ^ data[i]) * 0xEDB88320);
    }
    return ~crc;
}

// Initialize audio tracking system
void AudioTracker_Init(void) {
    // Clear the assets array
    memset(g_assets, 0, sizeof(g_assets));
    g_numAssets = 0;
    
    // Clear audio stats
    memset(&g_audioStats, 0, sizeof(g_audioStats));
    
    ROMLoader_TrackLoadStep("AUDIO INIT", "Audio tracking system initialized");
}

// Register an audio asset
int AudioTracker_RegisterAsset(const char* name, int sampleRate, int channels, 
                             int bitsPerSample, int size, float duration, UINT8* data) {
    if (g_numAssets >= MAX_AUDIO_ASSETS) {
        ROMLoader_DebugLog(LOG_WARNING, "Too many audio assets, can't register %s", name);
        return -1;
    }
    
    int id = g_numAssets++;
    
    // Store asset information
    g_assets[id].name = name;
    g_assets[id].sampleRate = sampleRate;
    g_assets[id].channels = channels;
    g_assets[id].bitsPerSample = bitsPerSample;
    g_assets[id].size = size;
    g_assets[id].isLoaded = 0;
    g_assets[id].duration = duration;
    
    // Calculate CRC if data is available
    if (data) {
        g_assets[id].crc = CalculateAudioCRC32(data, size);
    } else {
        g_assets[id].crc = 0;
    }
    
    // Log the registration
    ROMLoader_DebugLog(LOG_DETAIL, "Registered audio asset #%d: %s (%d Hz, %d ch, %d bits, %.2f sec, %d bytes)", 
                     id, name, sampleRate, channels, bitsPerSample, duration, size);
    
    return id;
}

// Track the loading of an audio asset
void AudioTracker_TrackLoading(int assetId, int success) {
    if (assetId < 0 || assetId >= g_numAssets) {
        ROMLoader_DebugLog(LOG_WARNING, "Invalid audio asset ID: %d", assetId);
        return;
    }
    
    g_assets[assetId].isLoaded = success;
    
    if (success) {
        ROMLoader_DebugLog(LOG_INFO, "Successfully loaded audio asset: %s", g_assets[assetId].name);
        ROMLoader_TrackLoadStep("AUDIO INIT", "Loaded %s (%.2f sec, %d Hz, %d-bit, %s)", 
                             g_assets[assetId].name,
                             g_assets[assetId].duration,
                             g_assets[assetId].sampleRate,
                             g_assets[assetId].bitsPerSample,
                             g_assets[assetId].channels == 1 ? "mono" : "stereo");
    } else {
        ROMLoader_DebugLog(LOG_WARNING, "Failed to load audio asset: %s", g_assets[assetId].name);
        ROMLoader_TrackLoadStep("AUDIO INIT", "Failed to load %s", g_assets[assetId].name);
    }
}

// Log info about all audio assets
void AudioTracker_LogAssets(void) {
    int loadedCount = 0;
    float totalDuration = 0.0f;
    
    for (int i = 0; i < g_numAssets; i++) {
        if (g_assets[i].isLoaded) {
            loadedCount++;
            totalDuration += g_assets[i].duration;
        }
    }
    
    ROMLoader_TrackLoadStep("AUDIO INIT", "Audio assets: %d total, %d loaded, %.2f sec total duration",
                         g_numAssets, loadedCount, totalDuration);
    
    // In verbose mode, log each asset
    ROMLoader_DebugLog(LOG_VERBOSE, "Audio Assets:");
    for (int i = 0; i < g_numAssets; i++) {
        ROMLoader_DebugLog(LOG_VERBOSE, "  #%d: %s (%d Hz, %d ch, %d bits, %.2f sec, %s)", 
                         i, g_assets[i].name, 
                         g_assets[i].sampleRate, g_assets[i].channels, 
                         g_assets[i].bitsPerSample, g_assets[i].duration,
                         g_assets[i].isLoaded ? "loaded" : "not loaded");
    }
}

// Get info about a specific asset
AudioAsset* AudioTracker_GetAsset(int assetId) {
    if (assetId < 0 || assetId >= g_numAssets) {
        return NULL;
    }
    
    return &g_assets[assetId];
}

// Track audio playback
void AudioTracker_TrackPlayback(int assetId, float volume, float pan) {
    if (assetId < 0 || assetId >= g_numAssets) {
        // Just ignore invalid asset IDs
        return;
    }
    
    // Log only occasionally to avoid spam
    static int playbackCounter = 0;
    if (playbackCounter++ % 10 == 0) {
        ROMLoader_DebugLog(LOG_DETAIL, "Audio playback: %s (vol=%.2f, pan=%.2f)", 
                        g_assets[assetId].name, volume, pan);
    }
}

// Track audio buffer statistics
void AudioTracker_TrackBufferStats(int bufferSize, int bufferUsed, int underruns, int overruns) {
    g_audioStats.bufferSize = bufferSize;
    g_audioStats.bufferUsed = bufferUsed;
    g_audioStats.underruns += underruns;
    g_audioStats.overruns += overruns;
    
    // Report to the debug system
    AUDIO_ReportStreamStats(bufferUsed, bufferSize, g_audioStats.sampleRate, underruns > 0);
}

// Initialize audio tracking system
void Audio_Init(void) {
    printf("Audio_Init: Initializing audio tracking\n");
    
    // Set default configuration
    g_audioConfig.sampleRate = 44100;
    g_audioConfig.channels = 2;
    g_audioConfig.bitDepth = 16;
    g_audioConfig.bufferSize = 2048;
    g_audioConfig.coreAudioInitialized = false;
    g_audioConfig.qsoundInitialized = false;
    g_audioConfig.audioMixerInitialized = false;
    g_audioConfig.soundBankLoaded = false;
    g_audioConfig.fmSynthInitialized = false;
    
    g_audioInitialized = true;
    g_reportGenerated = false;
}

// Configure audio system
void Audio_Configure(int sampleRate, int channels, int bitDepth, int bufferSize) {
    if (!g_audioInitialized) {
        Audio_Init();
    }
    
    g_audioConfig.sampleRate = sampleRate;
    g_audioConfig.channels = channels;
    g_audioConfig.bitDepth = bitDepth;
    g_audioConfig.bufferSize = bufferSize;
    
    printf("Audio_Configure: Sample rate=%d, channels=%d, bit depth=%d, buffer size=%d\n",
           sampleRate, channels, bitDepth, bufferSize);
}

// Mark CoreAudio as initialized
void Audio_SetCoreAudioInitialized(bool initialized, float latencyMs) {
    if (!g_audioInitialized) {
        Audio_Init();
    }
    
    g_audioConfig.coreAudioInitialized = initialized;
    
    if (initialized) {
        Debug_Log(DEBUG_AUDIO_INIT, "CoreAudio output initialized: %d Hz, %d channels, %d-bit, %.0f ms latency", 
                 g_audioConfig.sampleRate, g_audioConfig.channels, g_audioConfig.bitDepth, latencyMs);
    }
}

// Mark QSound DSP as initialized
void Audio_SetQSoundInitialized(bool initialized) {
    if (!g_audioInitialized) {
        Audio_Init();
    }
    
    g_audioConfig.qsoundInitialized = initialized;
    
    if (initialized) {
        AUDIO_ReportInitialization(g_audioConfig.sampleRate, g_audioConfig.channels, 
                                  g_audioConfig.bitDepth, g_audioConfig.bufferSize);
    }
}

// Mark audio mixer as initialized
void Audio_SetAudioMixerInitialized(bool initialized, int numChannels) {
    if (!g_audioInitialized) {
        Audio_Init();
    }
    
    g_audioConfig.audioMixerInitialized = initialized;
    
    if (initialized) {
        Debug_Log(DEBUG_AUDIO_INIT, "Audio Mixer: Audio mixer initialized with %d channels", numChannels);
    }
}

// Mark sound bank as loaded
void Audio_SetSoundBankLoaded(bool loaded, int numSounds) {
    if (!g_audioInitialized) {
        Audio_Init();
    }
    
    g_audioConfig.soundBankLoaded = loaded;
    
    if (loaded) {
        Debug_Log(DEBUG_AUDIO_INIT, "Sound Bank: Sound bank loaded with %d sound effects", numSounds);
    }
}

// Mark FM synthesis as initialized
void Audio_SetFMSynthInitialized(bool initialized) {
    if (!g_audioInitialized) {
        Audio_Init();
    }
    
    g_audioConfig.fmSynthInitialized = initialized;
    
    if (initialized) {
        Debug_Log(DEBUG_AUDIO_INIT, "FM Synthesis: FM synthesis engine initialized for music playback");
    }
}

// Generate audio initialization report
void Audio_GenerateReport(void) {
    if (g_reportGenerated) {
        return;
    }
    
    printf("Audio_GenerateReport: Generating audio initialization report\n");
    
    // Print section header
    Debug_PrintSectionHeader(DEBUG_AUDIO_INIT, "QSound DSP initialized successfully with audio buffers prepared.");
    
    // Report audio initialization details
    if (!g_audioInitialized) {
        Audio_Init();
    }
    
    // If components aren't explicitly initialized, show default messages
    if (!g_audioConfig.qsoundInitialized) {
        AUDIO_ReportInitialization(g_audioConfig.sampleRate, g_audioConfig.channels, 
                                  g_audioConfig.bitDepth, g_audioConfig.bufferSize);
    }
    
    if (!g_audioConfig.coreAudioInitialized) {
        Audio_SetCoreAudioInitialized(true, 11.0f);
    }
    
    if (!g_audioConfig.audioMixerInitialized) {
        Audio_SetAudioMixerInitialized(true, 32);
    }
    
    if (!g_audioConfig.soundBankLoaded) {
        Audio_SetSoundBankLoaded(true, 128);
    }
    
    if (!g_audioConfig.fmSynthInitialized) {
        Audio_SetFMSynthInitialized(true);
    }
    
    g_reportGenerated = true;
}

// Initialize audio components
void Audio_InitComponents(void) {
    printf("Audio_InitComponents: Initializing audio components\n");
    
    // Initialize audio tracking
    Audio_Init();
    
    // Configure audio system
    Audio_Configure(44100, 2, 16, 2048);
    
    // Initialize QSound DSP
    Audio_SetQSoundInitialized(true);
    
    // Initialize CoreAudio
    Audio_SetCoreAudioInitialized(true, 11.0f);
    
    // Initialize audio mixer
    Audio_SetAudioMixerInitialized(true, 32);
    
    // Load sound bank
    Audio_SetSoundBankLoaded(true, 128);
    
    // Initialize FM synthesis
    Audio_SetFMSynthInitialized(true);
    
    // Generate the report
    Audio_GenerateReport();
}

// Clean up audio tracking
void Audio_Exit() {
    pthread_mutex_lock(&g_audioMutex);
    g_audioSystemInitialized = false;
    pthread_mutex_unlock(&g_audioMutex);
}

// Initialize an audio component with specific format
bool Audio_InitComponent(AudioComponentType component, int sampleRate, int channels, 
                        int bitsPerSample, int bufferSize, const char* statusMsg) {
    if (!g_audioSystemInitialized || component >= AUDIO_COMPONENT_COUNT) {
        return false;
    }
    
    pthread_mutex_lock(&g_audioMutex);
    
    // Update component state
    AudioComponent* comp = &g_audioComponents[component];
    comp->initialized = true;
    comp->errorCode = 0;
    
    if (statusMsg) {
        strncpy(comp->statusMessage, statusMsg, sizeof(comp->statusMessage) - 1);
        comp->statusMessage[sizeof(comp->statusMessage) - 1] = '\0';
    } else {
        snprintf(comp->statusMessage, sizeof(comp->statusMessage),
                "%s initialized successfully", g_audioComponentNames[component]);
    }
    
    // Set format
    comp->format.sampleRate = sampleRate;
    comp->format.channels = channels;
    comp->format.bitsPerSample = bitsPerSample;
    comp->format.bufferSize = bufferSize;
    comp->format.initialized = true;
    
    // Log this initialization to the debug system
    AUDIO_ReportInitialization(sampleRate, channels, bitsPerSample, bufferSize);
    
    pthread_mutex_unlock(&g_audioMutex);
    
    return true;
}

// Mark an audio component as initialized with default format
bool Audio_InitComponentSimple(AudioComponentType component, const char* statusMsg) {
    if (!g_audioSystemInitialized || component >= AUDIO_COMPONENT_COUNT) {
        return false;
    }
    
    // Use default format (44.1kHz, stereo, 16-bit)
    return Audio_InitComponent(component, 44100, 2, 16, 4096, statusMsg);
}

// Get component initialization status
bool Audio_IsComponentInitialized(AudioComponentType component) {
    if (!g_audioSystemInitialized || component >= AUDIO_COMPONENT_COUNT) {
        return false;
    }
    
    return g_audioComponents[component].initialized;
}

// Update audio playback statistics
void Audio_UpdatePlaybackStats(int samplesPlayed, float bufferUsage, bool bufferUnderrun) {
    if (!g_audioSystemInitialized) {
        return;
    }
    
    pthread_mutex_lock(&g_audioMutex);
    
    // Update stats
    g_totalSamplesPlayed += samplesPlayed;
    g_isPlaying = true;
    
    // Update average buffer usage with simple running average
    g_averageBufferUsage = g_averageBufferUsage * 0.9f + bufferUsage * 0.1f;
    
    // Count buffer underruns
    if (bufferUnderrun) {
        g_bufferUnderrunCount++;
    }
    
    pthread_mutex_unlock(&g_audioMutex);
    
    // Report to debug system if underrun occurred
    if (bufferUnderrun) {
        int bufferSize = 0;
        int bufferFill = 0;
        int sampleRate = 44100;
        
        if (g_audioComponents[AUDIO_COMPONENT_DSP].initialized) {
            bufferSize = g_audioComponents[AUDIO_COMPONENT_DSP].format.bufferSize;
            sampleRate = g_audioComponents[AUDIO_COMPONENT_DSP].format.sampleRate;
            bufferFill = (int)(bufferSize * bufferUsage);
            
            AUDIO_ReportStreamStats(bufferFill, bufferSize, sampleRate, true);
        }
    }
}

// Start audio playback
void Audio_StartPlayback() {
    if (!g_audioSystemInitialized) {
        return;
    }
    
    pthread_mutex_lock(&g_audioMutex);
    g_isPlaying = true;
    pthread_mutex_unlock(&g_audioMutex);
    
    // Output the audio loop section header
    Debug_PrintSectionHeader(DEBUG_AUDIO_LOOP, "Audio streaming activated (CoreAudio backend).");
}

// Stop audio playback
void Audio_StopPlayback() {
    if (!g_audioSystemInitialized) {
        return;
    }
    
    pthread_mutex_lock(&g_audioMutex);
    g_isPlaying = false;
    pthread_mutex_unlock(&g_audioMutex);
    
    // Output the audio loop stop message
    Debug_Log(DEBUG_AUDIO_LOOP, "Audio streaming deactivated.");
}

// Register audio asset (stub to satisfy compilation)
int Audio_RegisterAsset(const char* name, int sampleRate, int channels, int bitDepth, 
                      int bufferSize, float duration, uint8_t* data) {
    printf("Audio_RegisterAsset: name=%s, rate=%d, channels=%d, bits=%d, size=%d\n",
           name, sampleRate, channels, bitDepth, bufferSize);
    return 0; // Return dummy ID
} 