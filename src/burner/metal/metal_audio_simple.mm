#import <Cocoa/Cocoa.h>
#import <AudioToolbox/AudioToolbox.h>
#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#include "metal_compat_layer.h"
#include "metal_audio_stubs.h"
#include <CoreAudio/CoreAudio.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Audio configuration
#define METAL_AUDIO_SAMPLE_RATE 44100
#define METAL_AUDIO_CHANNELS 2
// 735 samples per frame at 44100Hz with 60fps (44100/60 = 735)
#define METAL_AUDIO_BUFFER_SIZE 735
#define METAL_AUDIO_NUM_BUFFERS 4

// Audio state
static AudioQueueRef g_audioQueue = NULL;
static AudioQueueBufferRef g_audioBuffers[METAL_AUDIO_NUM_BUFFERS];
static bool g_audioInitialized = false;
static bool g_audioEnabled = true;
static INT16* g_audioBuffer = NULL;
static INT32 g_audioBufferSize = 0;
static float g_audioVolume = 1.0f;
static INT32 g_audioUnderrunCount = 0;
static INT32 g_audioOverrunCount = 0;

// Audio format description
static AudioStreamBasicDescription g_audioFormat = {
    .mSampleRate = METAL_AUDIO_SAMPLE_RATE,
    .mFormatID = kAudioFormatLinearPCM,
    .mFormatFlags = kLinearPCMFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked,
    .mBytesPerPacket = 4, // 2 channels * 2 bytes per sample
    .mFramesPerPacket = 1,
    .mBytesPerFrame = 4,
    .mChannelsPerFrame = METAL_AUDIO_CHANNELS,
    .mBitsPerChannel = 16,
    .mReserved = 0
};

// Audio queue callback
static void Metal_AudioCallback(void* inUserData, AudioQueueRef inAQ, AudioQueueBufferRef inBuffer) {
    if (!g_audioInitialized || !g_audioEnabled) {
        // Fill with silence
        memset(inBuffer->mAudioData, 0, inBuffer->mAudioDataBytesCapacity);
        inBuffer->mAudioDataByteSize = inBuffer->mAudioDataBytesCapacity;
        AudioQueueEnqueueBuffer(inAQ, inBuffer, 0, NULL);
        return;
    }
    
    // Calculate how many samples we need
    UInt32 bytesRequested = inBuffer->mAudioDataBytesCapacity;
    UInt32 samplesRequested = bytesRequested / (METAL_AUDIO_CHANNELS * sizeof(INT16));
    
    INT16* outputBuffer = (INT16*)inBuffer->mAudioData;
    
    // Check if we have FBNeo audio data
    if (pBurnSoundOut && nBurnSoundLen > 0) {
        // Copy FBNeo audio data
        UInt32 samplesToCopy = (samplesRequested < nBurnSoundLen) ? samplesRequested : nBurnSoundLen;
        
        // Track underrun/overrun for debugging
        if (samplesRequested > nBurnSoundLen) {
            g_audioUnderrunCount++;
            if (g_audioUnderrunCount % 100 == 1) {
                printf("[Metal_AudioCallback] Audio underrun #%d: Need %d samples, have %d\n", 
                       g_audioUnderrunCount, samplesRequested, nBurnSoundLen);
            }
        } else if (samplesRequested < nBurnSoundLen) {
            g_audioOverrunCount++;
            if (g_audioOverrunCount % 100 == 1) {
                printf("[Metal_AudioCallback] Audio overrun #%d: Need %d samples, have %d\n", 
                       g_audioOverrunCount, samplesRequested, nBurnSoundLen);
            }
        }
        
        // Apply volume and copy
        for (UInt32 i = 0; i < samplesToCopy * METAL_AUDIO_CHANNELS; i++) {
            float sample = (float)pBurnSoundOut[i] * g_audioVolume;
            
            // Clamp to prevent overflow
            if (sample > 32767.0f) sample = 32767.0f;
            if (sample < -32768.0f) sample = -32768.0f;
            
            outputBuffer[i] = (INT16)sample;
        }
        
        // Fill remaining with silence if needed
        if (samplesToCopy < samplesRequested) {
            UInt32 remainingBytes = (samplesRequested - samplesToCopy) * METAL_AUDIO_CHANNELS * sizeof(INT16);
            memset(&outputBuffer[samplesToCopy * METAL_AUDIO_CHANNELS], 0, remainingBytes);
        }
        
        inBuffer->mAudioDataByteSize = bytesRequested;
    } else {
        // No audio data available, fill with silence
        memset(outputBuffer, 0, bytesRequested);
        inBuffer->mAudioDataByteSize = bytesRequested;
    }
    
    // Enqueue the buffer back to the audio queue
    AudioQueueEnqueueBuffer(inAQ, inBuffer, 0, NULL);
}

// Initialize Metal audio system
INT32 Metal_InitAudio() {
    printf("[Metal_InitAudio] Initializing audio system\n");
    
    if (g_audioInitialized) {
        printf("[Metal_InitAudio] Audio already initialized\n");
        return 0;
    }
    
    // Set FBNeo audio parameters - make sure they match our definitions
    nBurnSoundRate = METAL_AUDIO_SAMPLE_RATE;
    nBurnSoundLen = METAL_AUDIO_BUFFER_SIZE;
    
    printf("[Metal_InitAudio] Sample rate: %d Hz, Buffer size: %d samples\n", 
           nBurnSoundRate, nBurnSoundLen);
    
    // Allocate audio buffer for FBNeo
    g_audioBufferSize = METAL_AUDIO_BUFFER_SIZE * METAL_AUDIO_CHANNELS * sizeof(INT16);
    g_audioBuffer = (INT16*)malloc(g_audioBufferSize);
    if (!g_audioBuffer) {
        printf("[Metal_InitAudio] ERROR: Failed to allocate audio buffer\n");
        return 1;
    }
    
    // Clear audio buffer
    memset(g_audioBuffer, 0, g_audioBufferSize);
    
    // Set FBNeo audio output pointer - this is crucial for the emulator to write audio data
    pBurnSoundOut = g_audioBuffer;
    
    // Initialize FBNeo audio system
    BurnSoundInit();
    BurnSoundDCFilterReset();
    
    // Create audio queue
    OSStatus status = AudioQueueNewOutput(&g_audioFormat, 
                                         Metal_AudioCallback, 
                                         NULL, 
                                         CFRunLoopGetCurrent(), 
                                         kCFRunLoopCommonModes, 
                                         0, 
                                         &g_audioQueue);
    if (status != noErr) {
        printf("[Metal_InitAudio] ERROR: Failed to create audio queue (status: %d)\n", (int)status);
        free(g_audioBuffer);
        g_audioBuffer = NULL;
        pBurnSoundOut = NULL;
        return 1;
    }
    
    // Set audio queue properties for better performance
    UInt32 propValue = 1;
    status = AudioQueueSetProperty(g_audioQueue, 
                                  kAudioQueueProperty_EnableTimePitch, 
                                  &propValue, 
                                  sizeof(propValue));
    if (status != noErr) {
        printf("[Metal_InitAudio] Warning: Failed to enable time pitch (status: %d)\n", (int)status);
        // Not critical, continue
    }
    
    // Allocate and enqueue audio buffers
    for (int i = 0; i < METAL_AUDIO_NUM_BUFFERS; i++) {
        status = AudioQueueAllocateBuffer(g_audioQueue, 
                                         METAL_AUDIO_BUFFER_SIZE * METAL_AUDIO_CHANNELS * sizeof(INT16), 
                                         &g_audioBuffers[i]);
        if (status != noErr) {
            printf("[Metal_InitAudio] ERROR: Failed to allocate audio buffer %d (status: %d)\n", 
                   i, (int)status);
            Metal_ExitAudio();
            return 1;
        }
        
        // Fill with silence initially
        memset(g_audioBuffers[i]->mAudioData, 0, g_audioBuffers[i]->mAudioDataBytesCapacity);
        g_audioBuffers[i]->mAudioDataByteSize = g_audioBuffers[i]->mAudioDataBytesCapacity;
        
        // Enqueue the buffer
        AudioQueueEnqueueBuffer(g_audioQueue, g_audioBuffers[i], 0, NULL);
    }
    
    // Start the audio queue
    status = AudioQueueStart(g_audioQueue, NULL);
    if (status != noErr) {
        printf("[Metal_InitAudio] ERROR: Failed to start audio queue (status: %d)\n", (int)status);
        Metal_ExitAudio();
        return 1;
    }
    
    // Reset counters
    g_audioUnderrunCount = 0;
    g_audioOverrunCount = 0;
    
    g_audioInitialized = true;
    g_audioEnabled = true;
    
    printf("[Metal_InitAudio] Audio system initialized successfully\n");
    
    return 0;
}

// Exit Metal audio system
INT32 Metal_ExitAudio() {
    printf("[Metal_ExitAudio] Shutting down audio system\n");
    
    if (!g_audioInitialized) {
        return 0;
    }
    
    if (g_audioQueue) {
        // Stop the audio queue
        AudioQueueStop(g_audioQueue, true);
        
        // Dispose of the audio queue
        AudioQueueDispose(g_audioQueue, true);
        g_audioQueue = NULL;
    }
    
    // Free audio buffer
    if (g_audioBuffer) {
        free(g_audioBuffer);
        g_audioBuffer = NULL;
    }
    
    // Reset FBNeo audio pointers
    pBurnSoundOut = NULL;
    nBurnSoundLen = 0;
    
    // Exit FBNeo audio system
    BurnSoundExit();
    
    g_audioInitialized = false;
    printf("[Metal_ExitAudio] Audio system shutdown complete\n");
    
    return 0;
}

// Enable/disable audio output
void Metal_SetAudioEnabled(bool enabled) {
    if (g_audioEnabled == enabled) {
        return;
    }
    
    g_audioEnabled = enabled;
    printf("[Metal_SetAudioEnabled] Audio %s\n", enabled ? "enabled" : "disabled");
    
    if (g_audioInitialized && g_audioQueue) {
        if (enabled) {
            AudioQueueStart(g_audioQueue, NULL);
        } else {
            AudioQueuePause(g_audioQueue);
        }
    }
}

// Set audio volume (0.0 to 1.0)
void Metal_SetAudioVolume(float volume) {
    if (volume < 0.0f) volume = 0.0f;
    if (volume > 1.0f) volume = 1.0f;
    
    g_audioVolume = volume;
    printf("[Metal_SetAudioVolume] Volume set to %.2f\n", volume);
}

// Get current audio volume
float Metal_GetAudioVolume() {
    return g_audioVolume;
}

// Check if audio is initialized
bool Metal_IsAudioInitialized() {
    return g_audioInitialized;
}

// Print current audio status
void Metal_PrintAudioStatus() {
    printf("===== AUDIO STATUS =====\n");
    printf("Initialized: %s\n", g_audioInitialized ? "Yes" : "No");
    printf("Enabled: %s\n", g_audioEnabled ? "Yes" : "No");
    printf("Sample Rate: %d Hz\n", METAL_AUDIO_SAMPLE_RATE);
    printf("Buffer Size: %d samples\n", METAL_AUDIO_BUFFER_SIZE);
    printf("Total Buffer Size: %d bytes\n", g_audioBufferSize);
    printf("Channels: %d\n", METAL_AUDIO_CHANNELS);
    printf("Volume: %.2f\n", g_audioVolume);
    printf("FBNeo Sound Rate: %d\n", nBurnSoundRate);
    printf("FBNeo Sound Length: %d\n", nBurnSoundLen);
    printf("FBNeo Sound Buffer: %p\n", pBurnSoundOut);
    printf("Audio Underruns: %d\n", g_audioUnderrunCount);
    printf("Audio Overruns: %d\n", g_audioOverrunCount);
    printf("=======================\n");
}

// Update audio (called each frame)
void Metal_UpdateAudio() {
    if (!g_audioInitialized || !g_audioEnabled) {
        return;
    }
    
    // Render audio from FBNeo core
    if (pBurnSoundOut && nBurnSoundLen > 0) {
        // Clear the buffer first
        memset(pBurnSoundOut, 0, nBurnSoundLen * METAL_AUDIO_CHANNELS * sizeof(INT16));
        
        // Call FBNeo's sound rendering function
        extern INT32 BurnSoundRender(INT16* pSoundBuf, INT32 nSegmentLength);
        INT32 result = BurnSoundRender(pBurnSoundOut, nBurnSoundLen);
        
        if (result != 0) {
            // If sound rendering failed, fill with silence
            memset(pBurnSoundOut, 0, nBurnSoundLen * METAL_AUDIO_CHANNELS * sizeof(INT16));
        }
    }
}

// Render audio for a specific number of samples (used for testing/debugging)
INT32 Metal_RenderAudio(INT16* buffer, INT32 samples) {
    if (!g_audioInitialized || !buffer || samples <= 0) {
        return 1;
    }
    
    // Clear the buffer
    memset(buffer, 0, samples * METAL_AUDIO_CHANNELS * sizeof(INT16));
    
    // Render audio
    return BurnSoundRender(buffer, samples);
}

// Get estimated audio latency in milliseconds
float Metal_GetAudioLatency() {
    if (!g_audioInitialized) {
        return 0.0f;
    }
    
    // Calculate latency based on buffer size and sample rate
    float latencyMs = (float)(METAL_AUDIO_BUFFER_SIZE * METAL_AUDIO_NUM_BUFFERS * 1000) / METAL_AUDIO_SAMPLE_RATE;
    return latencyMs;
} 