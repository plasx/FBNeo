#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#import <AudioToolbox/AudioToolbox.h>

// Include C-compatible header
#include "metal_audio.h"

// FBNeo core includes
extern "C" {
    #include "burnint.h"
}

// Audio system namespace
namespace MetalAudio {
    // Constants
    const int DEFAULT_SAMPLE_RATE = 44100;
    const int DEFAULT_CHANNELS = 2;
    const int BUFFER_SIZE = 2048;
    const int NUM_BUFFERS = 4;
    
    // Audio state tracking
    static bool initialized = false;
    static float masterVolume = 1.0f;
    static bool isPaused = false;
    static int currentSampleRate = DEFAULT_SAMPLE_RATE;
    static int currentChannels = DEFAULT_CHANNELS;
    
    // Audio engine components
    static AVAudioEngine* audioEngine = nil;
    static AVAudioSourceNode* sourceNode = nil;
    static AVAudioFormat* audioFormat = nil;
    
    // Ring buffer for audio samples
    struct AudioRingBuffer {
        std::vector<float> buffer;
        size_t readPos;
        size_t writePos;
        size_t size;
        std::mutex mutex;
        
        AudioRingBuffer(size_t sizeInSamples) {
            buffer.resize(sizeInSamples);
            readPos = 0;
            writePos = 0;
            size = sizeInSamples;
        }
        
        size_t availableWrite() {
            std::lock_guard<std::mutex> lock(mutex);
            if (readPos <= writePos) {
                return size - (writePos - readPos) - 1;
            } else {
                return readPos - writePos - 1;
            }
        }
        
        size_t availableRead() {
            std::lock_guard<std::mutex> lock(mutex);
            if (readPos <= writePos) {
                return writePos - readPos;
            } else {
                return size - readPos + writePos;
            }
        }
        
        size_t write(const float* data, size_t numSamples) {
            std::lock_guard<std::mutex> lock(mutex);
            size_t available = availableWrite();
            size_t count = std::min(numSamples, available);
            
            for (size_t i = 0; i < count; i++) {
                buffer[writePos] = data[i];
                writePos = (writePos + 1) % size;
            }
            
            return count;
        }
        
        size_t read(float* data, size_t numSamples) {
            std::lock_guard<std::mutex> lock(mutex);
            size_t available = availableRead();
            size_t count = std::min(numSamples, available);
            
            for (size_t i = 0; i < count; i++) {
                data[i] = buffer[readPos];
                readPos = (readPos + 1) % size;
            }
            
            return count;
        }
        
        void reset() {
            std::lock_guard<std::mutex> lock(mutex);
            readPos = 0;
            writePos = 0;
            std::fill(buffer.begin(), buffer.end(), 0.0f);
        }
    };
    
    static std::unique_ptr<AudioRingBuffer> ringBuffer;
    
    // Forward declarations of internal methods
    bool CreateAudioEngine();
    void DestroyAudioEngine();
    static OSStatus AudioRenderCallback(void* inRefCon, 
                                       AudioUnitRenderActionFlags* ioActionFlags,
                                       const AudioTimeStamp* inTimeStamp,
                                       UInt32 inBusNumber,
                                       UInt32 inNumberFrames,
                                       AudioBufferList* ioData);
}

// Create AVAudioEngine and set up audio processing nodes
bool MetalAudio::CreateAudioEngine() {
    // Create buffer
    ringBuffer = std::make_unique<AudioRingBuffer>(BUFFER_SIZE * NUM_BUFFERS * DEFAULT_CHANNELS);
    
    @autoreleasepool {
        // Create engine
        audioEngine = [[AVAudioEngine alloc] init];
        
        // Define audio format
        audioFormat = [[AVAudioFormat alloc] initStandardFormatWithSampleRate:DEFAULT_SAMPLE_RATE 
                                                                      channels:DEFAULT_CHANNELS];
        
        // Create source node
        AVAudioSourceNodeRenderBlock renderBlock = ^OSStatus(BOOL *isSilence, 
                                                           const AudioTimeStamp *timestamp, 
                                                           AVAudioFrameCount frameCount, 
                                                           AudioBufferList *outputData) {
            // Process audio
            float *buffer = (float *)outputData->mBuffers[0].mData;
            
            // Check if we're paused
            if (isPaused) {
                // Output silence
                memset(buffer, 0, frameCount * DEFAULT_CHANNELS * sizeof(float));
                *isSilence = YES;
                return noErr;
            }
            
            // Read from ring buffer
            size_t samplesNeeded = frameCount * DEFAULT_CHANNELS;
            size_t samplesRead = ringBuffer->read(buffer, samplesNeeded);
            
            // Apply volume
            for (size_t i = 0; i < samplesRead; i++) {
                buffer[i] *= masterVolume;
            }
            
            // Fill any remaining space with silence
            if (samplesRead < samplesNeeded) {
                memset(buffer + samplesRead, 0, (samplesNeeded - samplesRead) * sizeof(float));
            }
            
            *isSilence = (samplesRead == 0);
            return noErr;
        };
        
        // Create source node with render block
        sourceNode = [[AVAudioSourceNode alloc] initWithRenderBlock:renderBlock];
        
        // Connect audio nodes
        [audioEngine attachNode:sourceNode];
        [audioEngine connect:sourceNode to:audioEngine.mainMixerNode format:audioFormat];
        
        // Set output volume
        audioEngine.mainMixerNode.outputVolume = masterVolume;
        
        // Start audio engine
        NSError *error = nil;
        if (![audioEngine startAndReturnError:&error]) {
            NSLog(@"Could not start audio engine: %@", error.localizedDescription);
            return false;
        }
        
        NSLog(@"Audio engine started successfully");
        return true;
    }
}

// Clean up audio engine resources
void MetalAudio::DestroyAudioEngine() {
    @autoreleasepool {
        if (audioEngine) {
            [audioEngine stop];
            sourceNode = nil;
            audioFormat = nil;
            audioEngine = nil;
        }
    }
    
    ringBuffer.reset();
}

// Initialize Metal audio system
int Metal_InitAudio() {
    if (MetalAudio::initialized) {
        NSLog(@"Metal_InitAudio: Audio system already initialized");
        return 0;
    }
    
    NSLog(@"Metal_InitAudio: Initializing audio system");
    
    // Configure and activate audio session
    NSError *error = nil;
    AVAudioSession *session = [AVAudioSession sharedInstance];
    
    if (![session setCategory:AVAudioSessionCategoryPlayback error:&error]) {
        NSLog(@"Failed to set audio session category: %@", error.localizedDescription);
        return 1;
    }
    
    if (![session setActive:YES error:&error]) {
        NSLog(@"Failed to activate audio session: %@", error.localizedDescription);
        return 1;
    }
    
    // Create audio engine
    if (!MetalAudio::CreateAudioEngine()) {
        NSLog(@"Metal_InitAudio: Failed to create audio engine");
        return 1;
    }
    
    // Set default values
    MetalAudio::masterVolume = 1.0f;
    MetalAudio::isPaused = false;
    
    // Mark as initialized
    MetalAudio::initialized = true;
    NSLog(@"Metal_InitAudio: Audio system initialized successfully");
    
    return 0;
}

// Update audio with new samples
void Metal_UpdateAudio(const void* samples, int numSamples, int channels, int sampleRate) {
    if (!MetalAudio::initialized || !samples || numSamples <= 0) {
        return;
    }
    
    // Check if audio format has changed
    if (sampleRate != MetalAudio::currentSampleRate || channels != MetalAudio::currentChannels) {
        NSLog(@"Audio format changed: %d Hz, %d channels", sampleRate, channels);
        MetalAudio::currentSampleRate = sampleRate;
        MetalAudio::currentChannels = channels;
        
        // We would ideally recreate the audio engine with new format
        // For simplicity, we'll just note it - a real implementation should handle this
    }
    
    // Convert samples to float format if needed
    const int16_t* pcmSamples = static_cast<const int16_t*>(samples);
    std::vector<float> floatSamples(numSamples * channels);
    
    // Convert 16-bit PCM to float (-1.0f to 1.0f)
    for (int i = 0; i < numSamples * channels; i++) {
        floatSamples[i] = pcmSamples[i] / 32768.0f;
    }
    
    // Write to ring buffer
    size_t written = MetalAudio::ringBuffer->write(floatSamples.data(), floatSamples.size());
    
    // If we couldn't write all samples, log a warning (buffer overflow)
    if (written < floatSamples.size()) {
        NSLog(@"Audio buffer overflow: %zu samples dropped", floatSamples.size() - written);
    }
}

// Shutdown audio system
void Metal_ShutdownAudio() {
    if (!MetalAudio::initialized) {
        return;
    }
    
    NSLog(@"Metal_ShutdownAudio: Shutting down audio system");
    
    // Stop and clean up audio engine
    MetalAudio::DestroyAudioEngine();
    
    // Deactivate audio session
    NSError *error = nil;
    if (![[AVAudioSession sharedInstance] setActive:NO error:&error]) {
        NSLog(@"Failed to deactivate audio session: %@", error.localizedDescription);
    }
    
    // Mark as uninitialized
    MetalAudio::initialized = false;
    
    NSLog(@"Metal_ShutdownAudio: Audio system shut down");
}

// Pause/resume audio
void Metal_PauseAudio(int pause) {
    if (!MetalAudio::initialized) {
        return;
    }
    
    MetalAudio::isPaused = (pause != 0);
    NSLog(@"Metal_PauseAudio: Audio %s", MetalAudio::isPaused ? "paused" : "resumed");
}

// Set audio volume
void Metal_SetAudioVolume(float volume) {
    if (!MetalAudio::initialized) {
        return;
    }
    
    // Clamp volume to valid range
    volume = (volume < 0.0f) ? 0.0f : ((volume > 1.0f) ? 1.0f : volume);
    
    MetalAudio::masterVolume = volume;
    
    // Update mixer volume
    @autoreleasepool {
        if (MetalAudio::audioEngine) {
            MetalAudio::audioEngine.mainMixerNode.outputVolume = volume;
        }
    }
    
    NSLog(@"Metal_SetAudioVolume: Volume set to %.2f", volume);
} 