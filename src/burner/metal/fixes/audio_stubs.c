#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Maximum number of sound channels
#define MAX_SOUND_CHANNELS 16

// Sample rate for sound generation
#define SAMPLE_RATE 44100

// Sound buffer size (in samples)
#define SOUND_BUFFER_SIZE (SAMPLE_RATE / 30 * 2) // ~1/30 sec, stereo

// Sound channel structure
typedef struct {
    int active;
    int frequency;
    int volume;
    float phase;
    int duration;
    int type; // 0=sine, 1=square, 2=noise
} SoundChannel;

// Global sound state
static SoundChannel g_SoundChannels[MAX_SOUND_CHANNELS] = {0};
static short g_SoundBuffer[SOUND_BUFFER_SIZE * 2]; // *2 for stereo
static int g_SoundBufferPos = 0;

// Sound generation function
void Metal_GenerateSound() {
    // Check if we have any active sound channels
    int activeChannels = 0;
    for (int i = 0; i < MAX_SOUND_CHANNELS; i++) {
        if (g_SoundChannels[i].active) {
            activeChannels++;
        }
    }
    
    // Every 60 frames, report on audio status
    static int audioFrameCounter = 0;
    if (++audioFrameCounter % 60 == 0) {
        fprintf(stderr, "[AUDIO LOOP] Audio streaming active: %d channels, %d Hz\n",
               activeChannels, SAMPLE_RATE);
    }
    
    // Generate silence for now
    memset(g_SoundBuffer, 0, sizeof(g_SoundBuffer));
}

// Sound helper functions
void Metal_PlaySound(int channel, int frequency, int volume, int duration, int type) {
    fprintf(stderr, "[AUDIO LOOP] Playing sound: ch=%d, freq=%d, vol=%d, type=%d\n",
           channel, frequency, volume, type);
    
    if (channel < 0 || channel >= MAX_SOUND_CHANNELS) return;
    
    g_SoundChannels[channel].active = 1;
    g_SoundChannels[channel].frequency = frequency;
    g_SoundChannels[channel].volume = volume;
    g_SoundChannels[channel].phase = 0.0f;
    g_SoundChannels[channel].duration = duration;
    g_SoundChannels[channel].type = type;
}

// Stop a sound
void Metal_StopSound(int channel) {
    if (channel < 0 || channel >= MAX_SOUND_CHANNELS) return;
    g_SoundChannels[channel].active = 0;
    
    fprintf(stderr, "[AUDIO LOOP] Stopping sound: ch=%d\n", channel);
}

// Register an audio asset
int Audio_RegisterAsset(const char* name, int sampleRate, int channels, int bitDepth, 
                       int bufferSize, float duration, void* data) {
    fprintf(stderr, "[AUDIO INIT] Registered audio asset: %s, %d Hz, %d ch, %d-bit\n",
           name, sampleRate, channels, bitDepth);
    return 1; // Return a dummy asset ID
}

// Sound initialization
void BurnSoundInit() {
    fprintf(stderr, "[AUDIO INIT] Initializing sound subsystem\n");
    
    // Initialize sound buffer
    memset(g_SoundBuffer, 0, sizeof(g_SoundBuffer));
    
    // For game sound effects
    for (int i = 0; i < MAX_SOUND_CHANNELS; i++) {
        g_SoundChannels[i].active = 0;
    }
    
    fprintf(stderr, "[AUDIO INIT] Audio subsystem initialized successfully\n");
}

// Sound exit
int BurnSoundExit() {
    fprintf(stderr, "[AUDIO INIT] Shutting down sound system\n");
    return 0;
} 