#import <Cocoa/Cocoa.h>
#import <AVFoundation/AVFoundation.h>
#import <AudioToolbox/AudioToolbox.h>
#include <thread>
#include <mutex>
#include <vector>
#include <atomic>
#include <algorithm>
#include <memory>
#include <mach/mach_time.h>

// Include headers needed for integration with FBNeo
#include "burnint.h"
#include "metal_audio.h"

// Extern variables from FBNeo core
extern INT16* pBurnSoundOut;  // Sound output buffer
extern INT32 nBurnSoundLen;   // Sound buffer length
extern INT32 nBurnSoundRate;  // Sound sample rate

// === Metal ARM64 Audio Implementation ===

// Constants
static const int kDefaultSampleRate = 44100;
static const int kDefaultBufferSize = 1024;
static const int kDefaultChannels = 2;
static const int kRingBufferSize = 16384;
static const float kSmoothingFactor = 0.95f;

// Audio state namespace
namespace MetalAudio {
    // Ring buffer for audio data
    class RingBuffer {
    private:
        std::vector<float> buffer;
        std::size_t capacity;
        std::size_t readPos;
        std::size_t writePos;
        bool isEmpty;
        bool isFull;
        std::mutex mutex;
        
    public:
        RingBuffer(std::size_t size) : 
            buffer(size), capacity(size), readPos(0), writePos(0), isEmpty(true), isFull(false) {
        }
        
        void reset() {
            std::lock_guard<std::mutex> lock(mutex);
            readPos = 0;
            writePos = 0;
            isEmpty = true;
            isFull = false;
        }
        
        std::size_t available() const {
            if (isEmpty) return 0;
            if (isFull) return capacity;
            
            if (writePos >= readPos) {
                return writePos - readPos;
            } else {
                return capacity - (readPos - writePos);
            }
        }
        
        std::size_t space() const {
            if (isEmpty) return capacity;
            if (isFull) return 0;
            
            return capacity - available();
        }
        
        std::size_t write(const float* data, std::size_t size) {
            std::lock_guard<std::mutex> lock(mutex);
            
            if (isFull) return 0;
            if (size == 0) return 0;
            
            std::size_t available = space();
            std::size_t toWrite = std::min(size, available);
            
            // First portion (up to buffer end)
            std::size_t firstPortion = std::min(toWrite, capacity - writePos);
            std::copy(data, data + firstPortion, buffer.begin() + writePos);
            
            // Second portion (wrapped around)
            if (firstPortion < toWrite) {
                std::copy(data + firstPortion, data + toWrite, buffer.begin());
                writePos = toWrite - firstPortion;
            } else {
                writePos = (writePos + firstPortion) % capacity;
            }
            
            isEmpty = false;
            isFull = (readPos == writePos);
            
            return toWrite;
        }
        
        std::size_t read(float* data, std::size_t size) {
            std::lock_guard<std::mutex> lock(mutex);
            
            if (isEmpty) return 0;
            if (size == 0) return 0;
            
            std::size_t dataAvailable = available();
            std::size_t toRead = std::min(size, dataAvailable);
            
            // First portion (up to buffer end)
            std::size_t firstPortion = std::min(toRead, capacity - readPos);
            std::copy(buffer.begin() + readPos, buffer.begin() + readPos + firstPortion, data);
            
            // Second portion (wrapped around)
            if (firstPortion < toRead) {
                std::copy(buffer.begin(), buffer.begin() + (toRead - firstPortion), data + firstPortion);
                readPos = toRead - firstPortion;
            } else {
                readPos = (readPos + firstPortion) % capacity;
            }
            
            isFull = false;
            isEmpty = (readPos == writePos);
            
            return toRead;
        }
    };
    
    // Audio state tracking
    static struct {
        bool initialized = false;
        bool paused = false;
        float masterVolume = 1.0f;
        float cpuUsage = 0.0f;
        int sampleRate = kDefaultSampleRate;
        int bufferSize = kDefaultBufferSize;
        int channels = kDefaultChannels;
        
        // CPU load tracking
        uint64_t totalAudioTime = 0;
        uint64_t totalTime = 0;
        uint64_t lastUpdateTime = 0;
        
        // Audio objects
        AVAudioEngine* audioEngine = nil;
        AVAudioSourceNode* sourceNode = nil;
        AVAudioFormat* audioFormat = nil;
        
        // Ring buffer for async audio processing
        std::unique_ptr<RingBuffer> ringBuffer;
        
        // Debug counters
        uint64_t totalFramesProcessed = 0;
        uint64_t bufferUnderflows = 0;
        uint64_t bufferOverflows = 0;
        uint64_t updateCalls = 0;
    } audioState;
    
    // Time conversion for CPU load tracking
    static mach_timebase_info_data_t timebaseInfo;
    static bool timebaseInitialized = false;
    
    // Get elapsed nanoseconds from mach absolute time
    uint64_t getElapsedNanos(uint64_t start, uint64_t end) {
        if (!timebaseInitialized) {
            mach_timebase_info(&timebaseInfo);
            timebaseInitialized = true;
        }
        
        return ((end - start) * timebaseInfo.numer) / timebaseInfo.denom;
    }
    
    // Convert PCM samples to float
    void convertSamplesToFloat(const INT16* input, float* output, size_t numSamples) {
        static const float scale = 1.0f / 32768.0f;
        
        for (size_t i = 0; i < numSamples; i++) {
            output[i] = input[i] * scale;
        }
    }
    
    // AVAudioEngine render callback
    AVAudioSourceNodeRenderBlock createRenderBlock() {
        return ^OSStatus(BOOL *isSilence, 
                        const AudioTimeStamp *timestamp, 
                        AVAudioFrameCount frameCount, 
                        AudioBufferList *outputData) {
            uint64_t startTime = mach_absolute_time();
            float *outputBuffer = (float *)outputData->mBuffers[0].mData;
            int channels = audioState.channels;
            
            // If paused, output silence
            if (audioState.paused) {
                for (AVAudioFrameCount i = 0; i < frameCount * channels; i++) {
                    outputBuffer[i] = 0.0f;
                }
                *isSilence = YES;
                return noErr;
            }
            
            // Read from ring buffer
            size_t samplesNeeded = frameCount * channels;
            size_t samplesRead = audioState.ringBuffer->read(outputBuffer, samplesNeeded);
            
            // Apply volume and fill with silence if needed
            bool bufferUnderrun = (samplesRead < samplesNeeded);
            
            if (bufferUnderrun) {
                audioState.bufferUnderflows++;
                // Fill remaining with silence
                for (size_t i = samplesRead; i < samplesNeeded; i++) {
                    outputBuffer[i] = 0.0f;
                }
            }
            
            // Apply master volume
            for (size_t i = 0; i < samplesNeeded; i++) {
                outputBuffer[i] *= audioState.masterVolume;
            }
            
            // Track CPU usage
            uint64_t endTime = mach_absolute_time();
            uint64_t audioTimeNanos = getElapsedNanos(startTime, endTime);
            audioState.totalAudioTime += audioTimeNanos;
            audioState.totalTime += getElapsedNanos(audioState.lastUpdateTime, endTime);
            audioState.lastUpdateTime = endTime;
            
            // Smooth CPU usage calculation
            if (audioState.totalTime > 0) {
                float instantCpuUsage = (float)audioState.totalAudioTime / (float)audioState.totalTime;
                audioState.cpuUsage = audioState.cpuUsage * kSmoothingFactor + 
                                     instantCpuUsage * (1.0f - kSmoothingFactor);
            }
            
            // Increment performance counters
            audioState.totalFramesProcessed += frameCount;
            
            *isSilence = (samplesRead == 0);
            return noErr;
        };
    }
    
    // Initialize the audio engine
    bool initAudioEngine() {
        @autoreleasepool {
            // Create and setup the AVAudioEngine
            audioState.audioEngine = [[AVAudioEngine alloc] init];
            
            // Create audio format
            audioState.audioFormat = [[AVAudioFormat alloc] 
                                  initStandardFormatWithSampleRate:audioState.sampleRate 
                                  channels:audioState.channels];
            
            if (!audioState.audioFormat) {
                NSLog(@"Failed to create audio format");
                return false;
            }
            
            // Create source node
            audioState.sourceNode = [[AVAudioSourceNode alloc] 
                                 initWithRenderBlock:createRenderBlock()];
            
            // Attach and connect source node to engine
            [audioState.audioEngine attachNode:audioState.sourceNode];
            [audioState.audioEngine connect:audioState.sourceNode 
                                        to:audioState.audioEngine.mainMixerNode 
                                    format:audioState.audioFormat];
            
            // Set initial volume
            audioState.audioEngine.mainMixerNode.outputVolume = audioState.masterVolume;
            
            // Start the engine
            NSError *error = nil;
            if (![audioState.audioEngine startAndReturnError:&error]) {
                NSLog(@"Failed to start audio engine: %@", error.localizedDescription);
                return false;
            }
            
            NSLog(@"Audio engine started successfully (sample rate: %d, channels: %d)", 
                 audioState.sampleRate, audioState.channels);
            return true;
        }
    }
    
    // Cleanup the audio engine
    void cleanupAudioEngine() {
        @autoreleasepool {
            if (audioState.audioEngine) {
                [audioState.audioEngine stop];
                [audioState.audioEngine reset];
                audioState.sourceNode = nil;
                audioState.audioFormat = nil;
                audioState.audioEngine = nil;
            }
        }
    }
}

// API Implementation
extern "C" {

// Initialize the audio system with the FBNeo core
int Metal_AudioInit() {
    if (MetalAudio::audioState.initialized) {
        NSLog(@"Audio system already initialized");
        return 0;
    }
    
    NSLog(@"Initializing macOS Metal audio system");
    
    // Setup audio parameters for macOS
    @autoreleasepool {
        // For macOS, we don't use AVAudioSession as it's iOS-only
        // Instead, we configure audio using AVAudioEngine directly
        
        // Check if we have a preferred sample rate from FBNeo
        if (nBurnSoundRate > 0) {
            MetalAudio::audioState.sampleRate = nBurnSoundRate;
        } else {
            // Use default sample rate
            MetalAudio::audioState.sampleRate = kDefaultSampleRate;
        }
        
        // Set buffer size based on sample rate
        if (nBurnSoundLen > 0) {
            MetalAudio::audioState.bufferSize = nBurnSoundLen;
        } else {
            MetalAudio::audioState.bufferSize = (MetalAudio::audioState.sampleRate >= 48000) ? 
                                               2048 : 1024;
        }
        
        // Use stereo channels
        MetalAudio::audioState.channels = 2;
        
        NSLog(@"Configured audio with sample rate: %d Hz, buffer size: %d samples, channels: %d", 
              MetalAudio::audioState.sampleRate,
              MetalAudio::audioState.bufferSize,
              MetalAudio::audioState.channels);
    }
    
    // Create ring buffer
    MetalAudio::audioState.ringBuffer = std::make_unique<MetalAudio::RingBuffer>(kRingBufferSize);
    
    // Initialize the audio engine
    if (!MetalAudio::initAudioEngine()) {
        NSLog(@"Failed to initialize audio engine");
        return 1;
    }
    
    // Initialize the FBNeo core audio settings
    nBurnSoundRate = MetalAudio::audioState.sampleRate;
    nBurnSoundLen = MetalAudio::audioState.bufferSize;
    
    // Reset performance counters
    MetalAudio::audioState.totalFramesProcessed = 0;
    MetalAudio::audioState.bufferUnderflows = 0;
    MetalAudio::audioState.bufferOverflows = 0;
    MetalAudio::audioState.updateCalls = 0;
    MetalAudio::audioState.lastUpdateTime = mach_absolute_time();
    
    // Mark as initialized
    MetalAudio::audioState.initialized = true;
    
    NSLog(@"macOS Metal audio system initialized successfully");
    return 0;
}

// Shutdown audio system
void Metal_ShutdownAudio() {
    if (!MetalAudio::audioState.initialized) {
        return;
    }
    
    NSLog(@"Shutting down macOS Metal audio system");
    
    // Clean up audio engine
    MetalAudio::cleanupAudioEngine();
    
    // Clean up ring buffer
    MetalAudio::audioState.ringBuffer.reset();
    
    // Log performance statistics
    NSLog(@"Audio statistics: %llu frames processed, %llu buffer underflows, %llu buffer overflows, %llu updates",
         MetalAudio::audioState.totalFramesProcessed,
         MetalAudio::audioState.bufferUnderflows,
         MetalAudio::audioState.bufferOverflows,
         MetalAudio::audioState.updateCalls);
    
    // Mark as uninitialized
    MetalAudio::audioState.initialized = false;
    
    NSLog(@"macOS Metal audio system shutdown complete");
}

// Update with new audio samples (called by the emulator core)
void Metal_UpdateAudio(const void* samples, int numSamples, int channels, int sampleRate) {
    if (!MetalAudio::audioState.initialized || !samples || numSamples <= 0) {
        return;
    }
    
    MetalAudio::audioState.updateCalls++;
    
    // Check for audio format changes
    bool formatChanged = false;
    if (sampleRate != MetalAudio::audioState.sampleRate || channels != MetalAudio::audioState.channels) {
        NSLog(@"Audio format changed: %d Hz, %d channels -> %d Hz, %d channels",
             MetalAudio::audioState.sampleRate, MetalAudio::audioState.channels,
             sampleRate, channels);
        
        formatChanged = true;
        MetalAudio::audioState.sampleRate = sampleRate;
        MetalAudio::audioState.channels = channels;
    }
    
    // Convert samples from 16-bit PCM to float
    const INT16* pcmSamples = static_cast<const INT16*>(samples);
    
    // Allocate buffer for float samples - use static to avoid reallocations
    static std::vector<float> floatSamples;
    floatSamples.resize(numSamples * channels);
    
    // Convert to float (-1.0 to 1.0 range)
    MetalAudio::convertSamplesToFloat(pcmSamples, floatSamples.data(), numSamples * channels);
    
    // Write to ring buffer
    size_t bytesWritten = MetalAudio::audioState.ringBuffer->write(
        floatSamples.data(), floatSamples.size());
    
    // Check for buffer overflow
    if (bytesWritten < floatSamples.size()) {
        MetalAudio::audioState.bufferOverflows++;
    }
}

// Pause/resume audio playback
void Metal_PauseAudio(int pause) {
    if (!MetalAudio::audioState.initialized) {
        return;
    }
    
    bool shouldPause = (pause != 0);
    
    // Only take action if state is changing
    if (shouldPause != MetalAudio::audioState.paused) {
        MetalAudio::audioState.paused = shouldPause;
        
        NSLog(@"Metal audio %s", shouldPause ? "paused" : "resumed");
        
        if (shouldPause) {
            // Clear the buffer when pausing to avoid old sound playing when resumed
            if (MetalAudio::audioState.ringBuffer) {
                MetalAudio::audioState.ringBuffer->reset();
            }
        }
    }
}

// Set volume level (0.0 - 1.0)
void Metal_SetAudioVolume(float volume) {
    if (!MetalAudio::audioState.initialized) {
        return;
    }
    
    // Clamp volume to valid range
    if (volume < 0.0f) volume = 0.0f;
    if (volume > 1.0f) volume = 1.0f;
    
    // Only update if changed
    if (volume != MetalAudio::audioState.masterVolume) {
        MetalAudio::audioState.masterVolume = volume;
        
        // Update mixer node volume
        @autoreleasepool {
            if (MetalAudio::audioState.audioEngine) {
                MetalAudio::audioState.audioEngine.mainMixerNode.outputVolume = volume;
            }
        }
        
        NSLog(@"Metal audio volume set to %.2f", volume);
    }
}

// Get current audio CPU usage (0.0 - 1.0)
float Metal_GetAudioCPUUsage() {
    return MetalAudio::audioState.initialized ? MetalAudio::audioState.cpuUsage : 0.0f;
}

// Get current buffer fill level (0.0 - 1.0)
float Metal_GetBufferFillLevel() {
    if (!MetalAudio::audioState.initialized || !MetalAudio::audioState.ringBuffer) {
        return 0.0f;
    }
    
    size_t available = MetalAudio::audioState.ringBuffer->available();
    return (float)available / (float)kRingBufferSize;
}

// Integrations for core FBNeo (required for callback interface)

// Process a frame's worth of audio data from FBNeo
int Metal_AudioFrame() {
    if (!MetalAudio::audioState.initialized) {
        return 1;
    }
    
    // If we have audio data from the core, update it
    if (pBurnSoundOut && nBurnSoundLen > 0) {
        Metal_UpdateAudio(pBurnSoundOut, nBurnSoundLen, 2, nBurnSoundRate);
        return 0;
    }
    
    return 1;
}

// Process audio samples for rendering
// This is called directly by BurnSoundRender via the bridge
int Metal_ProcessAudioFrame(short* pSoundBuf, int nSegmentLength) {
    if (!MetalAudio::audioState.initialized || !pSoundBuf || nSegmentLength <= 0) {
        return 1;
    }
    
    // Update audio system with the provided samples
    Metal_UpdateAudio(pSoundBuf, nSegmentLength, MetalAudio::audioState.channels, 
                      MetalAudio::audioState.sampleRate);
    
    return 0;
}

// Calculate audio latency in milliseconds based on buffer fill and sample rate
float Metal_GetAudioLatency() {
    if (!MetalAudio::audioState.initialized || !MetalAudio::audioState.ringBuffer) {
        return 0.0f;
    }
    
    // Calculate how many samples are in the buffer
    size_t samplesAvailable = MetalAudio::audioState.ringBuffer->available();
    
    // Convert to milliseconds based on sample rate and channels
    float latencyMs = (float)samplesAvailable / 
                    (MetalAudio::audioState.channels * MetalAudio::audioState.sampleRate) * 1000.0f;
    
    return latencyMs;
}

// Set sample rate for audio synchronization
void Metal_AudioSetSampleRate(int sampleRate) {
    // Only change if necessary and initialized
    if (!MetalAudio::audioState.initialized || sampleRate <= 0 || 
        sampleRate == MetalAudio::audioState.sampleRate) {
        return;
    }
    
    NSLog(@"Changing audio sample rate from %d to %d Hz", 
          MetalAudio::audioState.sampleRate, sampleRate);
    
    // Save current state
    bool wasPaused = MetalAudio::audioState.paused;
    
    // Pause audio while we change settings
    Metal_PauseAudio(1);
    
    // Clean up current engine
    MetalAudio::cleanupAudioEngine();
    
    // Set new sample rate
    MetalAudio::audioState.sampleRate = sampleRate;
    nBurnSoundRate = sampleRate;
    
    // Clear ring buffer
    if (MetalAudio::audioState.ringBuffer) {
        MetalAudio::audioState.ringBuffer->reset();
    }
    
    // Reinitialize engine with new rate
    if (!MetalAudio::initAudioEngine()) {
        NSLog(@"Failed to reinitialize audio engine after sample rate change");
    }
    
    // Restore pause state
    if (!wasPaused) {
        Metal_PauseAudio(0);
    }
}

// Set buffer size for audio synchronization
void Metal_AudioSetBufferSize(int bufferSize) {
    // Only change if necessary and initialized
    if (!MetalAudio::audioState.initialized || bufferSize <= 0 || 
        bufferSize == MetalAudio::audioState.bufferSize) {
        return;
    }
    
    NSLog(@"Changing audio buffer size from %d to %d samples", 
          MetalAudio::audioState.bufferSize, bufferSize);
    
    // Update buffer size
    MetalAudio::audioState.bufferSize = bufferSize;
    nBurnSoundLen = bufferSize;
}

// Audio synchronization settings
static int audioSyncMode = 0; // 0: disabled, 1: light, 2: full

// Set audio synchronization mode
void Metal_SetAudioSync(int syncMode) {
    // Clamp to valid range
    if (syncMode < 0) syncMode = 0;
    if (syncMode > 2) syncMode = 2;
    
    audioSyncMode = syncMode;
}

// Get current audio synchronization mode
int Metal_GetAudioSync() {
    return audioSyncMode;
}

} // extern "C" 