#import <Foundation/Foundation.h>
#import <AudioToolbox/AudioToolbox.h>
#include "burnint.h"
#include "metal_bridge.h"
#include "game_audio.h"

// Audio rendering for FBNeo Metal implementation using Apple's AudioUnit API

// Define the audio format we'll use
#define AUDIO_SAMPLE_RATE      44100
#define AUDIO_BUFFER_SIZE      2048
#define AUDIO_CHANNELS         2
#define AUDIO_BITS_PER_SAMPLE  16

// Forward declare the BurnSoundRender function
extern "C" int BurnSoundRender(short* pSample, int nSamples);

// Audio state
static AudioUnit audioUnit = NULL;
static AudioBufferList *audioBufferList = NULL;
static short *mixBuffer = NULL;
// Make masterVolume accessible from metal_exports.cpp
float masterVolume = 1.0f;
static BOOL audioEnabled = YES;
static int sampleRate = AUDIO_SAMPLE_RATE;
static int bufferSize = AUDIO_BUFFER_SIZE;
static int channels = AUDIO_CHANNELS;

// Audio callback function
static OSStatus AudioRenderCallback(void *inRefCon,
                                   AudioUnitRenderActionFlags *ioActionFlags,
                                   const AudioTimeStamp *inTimeStamp,
                                   UInt32 inBusNumber,
                                   UInt32 inNumberFrames,
                                   AudioBufferList *ioData) {
    // If audio is disabled, return silence
    if (!audioEnabled) {
        for (int i = 0; i < ioData->mNumberBuffers; i++) {
            memset(ioData->mBuffers[i].mData, 0, ioData->mBuffers[i].mDataByteSize);
        }
        return noErr;
    }
    
    // Number of samples to generate (usually stereo, so samples * 2 for L/R channels)
    UInt32 samplesToGenerate = inNumberFrames;
    
    // For debugging
    static int frameCount = 0;
    frameCount++;
    if (frameCount % 300 == 0) {
        NSLog(@"Audio frame %d: requested %d frames", 
              frameCount, inNumberFrames);
    }
    
    // Allocate temporary buffer for 16-bit samples if needed
    if (!mixBuffer || bufferSize < samplesToGenerate) {
        if (mixBuffer) {
            free(mixBuffer);
        }
        bufferSize = samplesToGenerate;
        mixBuffer = (short*)malloc(bufferSize * 2 * sizeof(short)); // *2 for stereo
        if (!mixBuffer) {
            for (int i = 0; i < ioData->mNumberBuffers; i++) {
                memset(ioData->mBuffers[i].mData, 0, ioData->mBuffers[i].mDataByteSize);
            }
            return noErr;
        }
    }
    
    // Clear the buffer
    memset(mixBuffer, 0, samplesToGenerate * 2 * sizeof(short));
    
    // Call BurnSoundRender to get audio samples from the emulator
    int nRet = BurnSoundRender(mixBuffer, samplesToGenerate);
    
    if (nRet != 0) {
        if (frameCount % 300 == 0) {
            NSLog(@"BurnSoundRender returned error: %d", nRet);
        }
        // On error, generate silence
        for (int i = 0; i < ioData->mNumberBuffers; i++) {
            memset(ioData->mBuffers[i].mData, 0, ioData->mBuffers[i].mDataByteSize);
        }
        return noErr;
    }
    
    // Process audio data from the emulator - convert from 16-bit PCM to float
    for (int i = 0; i < ioData->mNumberBuffers; i++) {
        float *outputBuffer = (float*)ioData->mBuffers[i].mData;
        int numSamples = ioData->mBuffers[i].mDataByteSize / sizeof(float);
        
        // Each buffer represents either all channels or one channel
        // Depending on how CoreAudio is configured, adapt accordingly
        if (numSamples == samplesToGenerate) {
            // Buffer is for one channel (mono output)
            for (int j = 0; j < numSamples && j * 2 < samplesToGenerate * 2; j++) {
                // Mix down stereo to mono by averaging left and right channels
                float left = (float)mixBuffer[j * 2] / 32768.0f;
                float right = (float)mixBuffer[j * 2 + 1] / 32768.0f;
                outputBuffer[j] = (left + right) * 0.5f * masterVolume;
            }
        } else if (numSamples == samplesToGenerate * 2) {
            // Buffer is for all channels (interleaved stereo output)
            for (int j = 0; j < numSamples && j < samplesToGenerate * 2; j++) {
                outputBuffer[j] = (float)mixBuffer[j] / 32768.0f * masterVolume;
            }
        }
    }
    
    return noErr;
}

int GameAudio_Init(int sampleRate, int bufferSize, int channels) {
    NSLog(@"GameAudio_Init(%d, %d, %d)", sampleRate, bufferSize, channels);
    
    // Store audio configuration
    ::sampleRate = sampleRate;
    ::bufferSize = bufferSize;
    ::channels = channels;
    
    // Initialize FBNeo audio system
    BurnSoundInit();
    
    // Allocate mix buffer (always stereo for FBNeo)
    mixBuffer = (short*)malloc(bufferSize * 2 * sizeof(short));
    if (!mixBuffer) {
        NSLog(@"Failed to allocate audio mix buffer");
        return 1;
    }
    memset(mixBuffer, 0, bufferSize * 2 * sizeof(short));
    
    // Setup the audio format
    AudioComponentDescription desc;
    desc.componentType = kAudioUnitType_Output;
    desc.componentSubType = kAudioUnitSubType_DefaultOutput;
    desc.componentManufacturer = kAudioUnitManufacturer_Apple;
    desc.componentFlags = 0;
    desc.componentFlagsMask = 0;
    
    // Find the default audio component
    AudioComponent component = AudioComponentFindNext(NULL, &desc);
    if (!component) {
        NSLog(@"Failed to find default audio component");
        return 1;
    }
    
    // Create an instance of the default audio component
    OSStatus status = AudioComponentInstanceNew(component, &audioUnit);
    if (status != noErr) {
        NSLog(@"Failed to create audio component instance: %d", (int)status);
        return 1;
    }
    
    // Set the audio format for output
    AudioStreamBasicDescription audioFormat;
    memset(&audioFormat, 0, sizeof(audioFormat));
    audioFormat.mSampleRate = sampleRate;
    audioFormat.mFormatID = kAudioFormatLinearPCM;
    audioFormat.mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked;
    audioFormat.mBitsPerChannel = 32;
    audioFormat.mChannelsPerFrame = channels;
    audioFormat.mFramesPerPacket = 1;
    audioFormat.mBytesPerFrame = channels * sizeof(float);
    audioFormat.mBytesPerPacket = channels * sizeof(float);
    
    status = AudioUnitSetProperty(audioUnit,
                                 kAudioUnitProperty_StreamFormat,
                                 kAudioUnitScope_Input,
                                 0,
                                 &audioFormat,
                                 sizeof(audioFormat));
    
    if (status != noErr) {
        NSLog(@"Failed to set audio unit format: %d", (int)status);
        return 1;
    }
    
    // Set the render callback
    AURenderCallbackStruct callbackStruct;
    callbackStruct.inputProc = AudioRenderCallback;
    callbackStruct.inputProcRefCon = NULL;
    
    status = AudioUnitSetProperty(audioUnit,
                                 kAudioUnitProperty_SetRenderCallback,
                                 kAudioUnitScope_Input,
                                 0,
                                 &callbackStruct,
                                 sizeof(callbackStruct));
    
    if (status != noErr) {
        NSLog(@"Failed to set audio unit render callback: %d", (int)status);
        return 1;
    }
    
    // Initialize the audio unit
    status = AudioUnitInitialize(audioUnit);
    if (status != noErr) {
        NSLog(@"Failed to initialize audio unit: %d", (int)status);
        return 1;
    }
    
    // Start the audio output
    status = AudioOutputUnitStart(audioUnit);
    if (status != noErr) {
        NSLog(@"Failed to start audio output unit: %d", (int)status);
        return 1;
    }
    
    NSLog(@"Audio system initialized: %dHz, %d samples per buffer, %d channels",
         sampleRate, bufferSize, channels);
    
    return 0;
}

int GameAudio_Exit() {
    NSLog(@"GameAudio_Exit()");
    
    // Stop the audio output
    if (audioUnit) {
        AudioOutputUnitStop(audioUnit);
        AudioUnitUninitialize(audioUnit);
        AudioComponentInstanceDispose(audioUnit);
        audioUnit = NULL;
    }
    
    // Free the mix buffer
    if (mixBuffer) {
        free(mixBuffer);
        mixBuffer = NULL;
    }
    
    // Shut down FBNeo audio
    // Just call BurnSoundInit with no parameters to reset audio instead of using BurnSoundExit
    BurnSoundInit();
    
    return 0;
}

// Make sure this matches the header declaration exactly - extern "C" for explicit linkage
extern "C" void GameAudio_SetVolume(float volume) {
    printf("GameAudio_SetVolume(%f)\n", volume);
    // Normalize volume to 0.0-1.0 range
    if (volume < 0.0f) volume = 0.0f;
    if (volume > 100.0f) volume = 100.0f;
    float normalizedVolume = volume / 100.0f;
    
    // Set master volume
    masterVolume = normalizedVolume;
}

int GameAudio_GetVolume() {
    return (int)(masterVolume * 100.0f); // Convert to percentage
}

int GameAudio_SetEnabled(bool enabled) {
    audioEnabled = enabled ? YES : NO;
    return 0;
}

// Reset audio state
void GameAudio_Reset() {
    // Reset the audio state
    if (mixBuffer) {
        memset(mixBuffer, 0, bufferSize * 2 * sizeof(short));
    }
    
    // Reset FBNeo audio system by re-initializing it
    BurnSoundInit();
}

// Called to get the next chunk of audio data from the emulator
void GameAudio_GetBuffer(short *buffer, int samplesRequested) {
    if (!buffer || !audioEnabled) {
        return;
    }
    
    // Clear the buffer
    memset(buffer, 0, samplesRequested * 2 * sizeof(short));
    
    // Get audio samples from the emulator
    BurnSoundRender(buffer, samplesRequested);
}

// Update the audio buffer with new data
int GameAudio_UpdateBuffer(const short* data, int size) {
    if (!data || size <= 0 || !audioEnabled) {
        return 1;
    }
    
    // Copy data to the mix buffer if it's smaller than our buffer size
    if (size <= bufferSize * 2) {
        memcpy(mixBuffer, data, size * sizeof(short));
        return 0;
    }
    
    // Otherwise, resize our buffer
    short* newBuffer = (short*)realloc(mixBuffer, size * sizeof(short));
    if (!newBuffer) {
        return 1;
    }
    
    mixBuffer = newBuffer;
    bufferSize = size / 2; // Adjust buffer size (assuming stereo)
    memcpy(mixBuffer, data, size * sizeof(short));
    
    return 0;
}

// Set new sample rate and buffer size
void GameAudio_Reconfigure(int newSampleRate, int newBufferSize) {
    if (sampleRate == newSampleRate && bufferSize == newBufferSize) {
        return; // No change
    }
    
    // Stop audio
    if (audioUnit) {
        AudioOutputUnitStop(audioUnit);
    }
    
    // Update parameters
    sampleRate = newSampleRate;
    bufferSize = newBufferSize;
    
    // Recreate mix buffer
    if (mixBuffer) {
        free(mixBuffer);
    }
    mixBuffer = (short*)malloc(bufferSize * 2 * sizeof(short));
    if (mixBuffer) {
        memset(mixBuffer, 0, bufferSize * 2 * sizeof(short));
    }
    
    // Update the FBNeo sound configuration
    // Just reinitialize the audio system
    BurnSoundInit();
    
    // Restart audio
    if (audioUnit) {
        AudioOutputUnitStart(audioUnit);
    }
} 