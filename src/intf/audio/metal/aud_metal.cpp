#include "aud_metal.h"
#include "burner.h"
#include <AudioToolbox/AudioToolbox.h>

static AudioQueueRef queue;
static AudioQueueBufferRef buffers[3];
static const int bufferSize = 2048;

static void AudioCallback(void *inUserData, AudioQueueRef inAQ, AudioQueueBufferRef inBuffer) {
    // Fill buffer with audio data
    memset(inBuffer->mAudioData, 0, bufferSize);
    inBuffer->mAudioDataByteSize = bufferSize;
    
    // Enqueue buffer
    AudioQueueEnqueueBuffer(queue, inBuffer, 0, NULL);
}

int AudioInit() {
    AudioStreamBasicDescription format;
    format.mSampleRate = 44100;
    format.mFormatID = kAudioFormatLinearPCM;
    format.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
    format.mBitsPerChannel = 16;
    format.mChannelsPerFrame = 2;
    format.mBytesPerFrame = format.mBitsPerChannel * format.mChannelsPerFrame / 8;
    format.mFramesPerPacket = 1;
    format.mBytesPerPacket = format.mBytesPerFrame * format.mFramesPerPacket;
    
    // Create queue
    AudioQueueNewOutput(&format, AudioCallback, NULL, NULL, NULL, 0, &queue);
    
    // Allocate buffers
    for (int i = 0; i < 3; i++) {
        AudioQueueAllocateBuffer(queue, bufferSize, &buffers[i]);
        AudioCallback(NULL, queue, buffers[i]);
    }
    
    // Start queue
    AudioQueueStart(queue, NULL);
    
    return 0;
}

int AudioExit() {
    AudioQueueStop(queue, true);
    AudioQueueDispose(queue, true);
    return 0;
}

int MetalAudioGetSettings(InterfaceInfo* pInfo) {
    return 0;
}

static int MetalBlankSound() { return 0; }
static int MetalSoundCheck() { return 0; }
static int MetalSetCallback(int (*pCallback)(int)) { return 0; }
static int MetalSoundPlay() { return 0; }
static int MetalSoundStop() { return AudioExit(); }
static int MetalSoundSetVolume() { return 0; }

struct AudOut AudOutMetal = {
    MetalBlankSound,
    MetalSoundCheck,
    AudioInit,
    MetalSetCallback,
    MetalSoundPlay,
    MetalSoundStop,
    AudioExit,
    MetalSoundSetVolume,
    MetalAudioGetSettings,
    "Metal audio output"
}; 