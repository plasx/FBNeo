#import <Cocoa/Cocoa.h>
#import <CoreAudio/CoreAudio.h>
#import <AudioToolbox/AudioToolbox.h>
#include "metal_bridge.h"

// Audio state
static AudioQueueRef audioQueue = NULL;
static AudioQueueBufferRef audioBuffers[3];
static const int kNumberBuffers = 3;
static int bufferSizeFrames = 0;
static bool audioRunning = false;

// Audio callback function
static void AudioQueueCallback(void* inUserData, AudioQueueRef inAQ, AudioQueueBufferRef inBuffer) {
    // Check if audio is enabled
    if (!Metal_IsAudioEnabled()) {
        // Fill with silence if audio is disabled
        memset(inBuffer->mAudioData, 0, inBuffer->mAudioDataBytesCapacity);
        inBuffer->mAudioDataByteSize = inBuffer->mAudioDataBytesCapacity;
        AudioQueueEnqueueBuffer(inAQ, inBuffer, 0, NULL);
        return;
    }
    
    // Get sound buffer from the emulator
    short* soundBuffer = Metal_GetAudioBuffer();
    int bufferSize = Metal_GetAudioBufferSize();
    
    if (!soundBuffer || bufferSize <= 0) {
        // If no emulator sound, fill with silence
        memset(inBuffer->mAudioData, 0, inBuffer->mAudioDataBytesCapacity);
        inBuffer->mAudioDataByteSize = inBuffer->mAudioDataBytesCapacity;
        AudioQueueEnqueueBuffer(inAQ, inBuffer, 0, NULL);
        return;
    }
    
    // Copy sound data 
    int volumeScale = Metal_GetVolume();
    int bytesToCopy = bufferSize * 2 * sizeof(short); // Stereo
    
    // Make sure we don't exceed the buffer capacity
    if (bytesToCopy > inBuffer->mAudioDataBytesCapacity) {
        bytesToCopy = inBuffer->mAudioDataBytesCapacity;
    }
    
    // Apply volume scaling
    if (volumeScale < 100) {
        // Create a temporary buffer for volume adjustment
        short* scaledBuffer = (short*)malloc(bytesToCopy);
        if (scaledBuffer) {
            // Scale volume
            for (int i = 0; i < bytesToCopy / sizeof(short); i++) {
                scaledBuffer[i] = (short)((int)soundBuffer[i] * volumeScale / 100);
            }
            
            // Copy scaled buffer
            memcpy(inBuffer->mAudioData, scaledBuffer, bytesToCopy);
            free(scaledBuffer);
        } else {
            // Fallback - direct copy
            memcpy(inBuffer->mAudioData, soundBuffer, bytesToCopy);
        }
    } else {
        // Full volume - direct copy
        memcpy(inBuffer->mAudioData, soundBuffer, bytesToCopy);
    }
    
    inBuffer->mAudioDataByteSize = bytesToCopy;
    AudioQueueEnqueueBuffer(inAQ, inBuffer, 0, NULL);
    
    // Run the emulator for the next frame of audio
    Metal_RunFrame(false);
}

// Initialize CoreAudio
bool InitCoreAudio(int sampleRate, int bufferSize) {
    // Clean up any existing audio
    if (audioQueue) {
        AudioQueueStop(audioQueue, true);
        AudioQueueDispose(audioQueue, true);
        audioQueue = NULL;
    }
    
    AudioStreamBasicDescription audioFormat;
    memset(&audioFormat, 0, sizeof(audioFormat));
    audioFormat.mSampleRate = sampleRate;
    audioFormat.mFormatID = kAudioFormatLinearPCM;
    audioFormat.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked;
    audioFormat.mBitsPerChannel = 16;
    audioFormat.mChannelsPerFrame = 2; // Stereo
    audioFormat.mFramesPerPacket = 1;
    audioFormat.mBytesPerPacket = 4; // 2 channels * 16 bits per sample / 8 bits per byte
    audioFormat.mBytesPerFrame = 4;  // 2 channels * 16 bits per sample / 8 bits per byte
    
    // Create the output audio queue
    OSStatus status = AudioQueueNewOutput(&audioFormat,
                                         AudioQueueCallback,
                                         NULL,
                                         NULL,
                                         NULL,
                                         0,
                                         &audioQueue);
    
    if (status != noErr) {
        NSLog(@"Error creating audio queue: %d", (int)status);
        return false;
    }
    
    // Calculate bytes per buffer
    bufferSizeFrames = bufferSize;
    int bytesPerBuffer = bufferSizeFrames * audioFormat.mBytesPerFrame;
    
    // Allocate and enqueue audio buffers
    for (int i = 0; i < kNumberBuffers; i++) {
        status = AudioQueueAllocateBuffer(audioQueue, bytesPerBuffer, &audioBuffers[i]);
        if (status != noErr) {
            NSLog(@"Error allocating audio buffer %d: %d", i, (int)status);
            AudioQueueDispose(audioQueue, true);
            audioQueue = NULL;
            return false;
        }
        
        // Fill with silence initially
        memset(audioBuffers[i]->mAudioData, 0, bytesPerBuffer);
        audioBuffers[i]->mAudioDataByteSize = bytesPerBuffer;
        
        status = AudioQueueEnqueueBuffer(audioQueue, audioBuffers[i], 0, NULL);
        if (status != noErr) {
            NSLog(@"Error enqueueing audio buffer %d: %d", i, (int)status);
            AudioQueueDispose(audioQueue, true);
            audioQueue = NULL;
            return false;
        }
    }
    
    // Start the audio queue
    status = AudioQueueStart(audioQueue, NULL);
    if (status != noErr) {
        NSLog(@"Error starting audio queue: %d", (int)status);
        AudioQueueDispose(audioQueue, true);
        audioQueue = NULL;
        return false;
    }
    
    audioRunning = true;
    return true;
}

// Externally accessible functions for Metal implementation

// Initialize audio for Metal - called from metal_bridge.cpp
extern "C" int Metal_InitAudioSystem_UNUSED(int sampleRate) {
    // Initialize Metal audio subsystem
    if (!InitCoreAudio(sampleRate, sampleRate / 60)) {
        NSLog(@"Failed to initialize CoreAudio");
        return 1;
    }
    
    NSLog(@"CoreAudio initialized with sample rate %d", sampleRate);
    return 0;
}

// Pause/resume audio 
extern "C" int Metal_PauseAudio(int pause) {
    if (!audioQueue) {
        return 1;
    }
    
    OSStatus status;
    if (pause) {
        if (audioRunning) {
            status = AudioQueuePause(audioQueue);
            if (status != noErr) {
                NSLog(@"Error pausing audio queue: %d", (int)status);
                return 1;
            }
            audioRunning = false;
        }
    } else {
        if (!audioRunning) {
            status = AudioQueueStart(audioQueue, NULL);
            if (status != noErr) {
                NSLog(@"Error starting audio queue: %d", (int)status);
                return 1;
            }
            audioRunning = true;
        }
    }
    
    return 0;
}

// Shutdown audio
extern "C" int Metal_ShutdownAudio_UNUSED() {
    if (audioQueue) {
        AudioQueueStop(audioQueue, true);
        AudioQueueDispose(audioQueue, true);
        audioQueue = NULL;
        audioRunning = false;
    }
    
    return 0;
}

// Reset audio 
extern "C" int Metal_ResetAudio() {
    // Stop current audio
    Metal_ShutdownAudio();
    
    // Restart with current parameters
    int sampleRate = 44100; // Default
    short* audioBuffer = Metal_GetAudioBuffer();
    if (audioBuffer) {
        // We have a valid audio buffer, so use its parameters
        sampleRate = Metal_GetAudioBufferSize() * 60; // Buffer size is 1/60th of sample rate
    }
    
    return Metal_InitAudioSystem(sampleRate);
} 