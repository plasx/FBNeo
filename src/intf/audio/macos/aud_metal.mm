#import <Foundation/Foundation.h>
#import <CoreAudio/CoreAudio.h>
#import <AudioUnit/AudioUnit.h>
#import <AudioToolbox/AudioToolbox.h>
#import "FBAudio.h"

// Core Audio constants
#define MAX_AUDIO_BUFFER_SIZE 8192
#define DEFAULT_SAMPLE_RATE 44100
#define DEFAULT_BUFFER_SIZE 2048
#define RING_BUFFER_SIZE 32768  // 32KB ring buffer
#define NUM_CHANNELS 2

// C-compatible interface
#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
int Metal_AudioInit();
int Metal_AudioExit();
int Metal_AudioFrame();
bool Metal_AudioIsMuted();
void Metal_AudioSetVolume(float volume);
float Metal_AudioGetVolume();
void Metal_AudioMute(bool mute);
int Metal_AudioSetSampleRate(int rate);
int Metal_AudioGetSampleRate();
int Metal_AudioGetBufferSize();
int Metal_AudioSetBufferSize(int size);
void Metal_AudioPause();
void Metal_AudioResume();
float Metal_AudioGetCPUUsage();

// External variables from FBNeo core
extern short* pBurnSoundOut;
extern int nBurnSoundLen;
extern int nBurnSoundRate;
extern int nAudNextSound;

#ifdef __cplusplus
}
#endif

// ============================================================================
// Metal Audio System Implementation
// ============================================================================

// Ring buffer implementation for audio data
typedef struct {
    short* buffer;
    int size;
    int readPos;
    int writePos;
    int available;
    NSLock* lock;
} AudioRingBuffer;

// Audio system state
static struct {
    bool initialized;
    bool muted;
    float volume;
    float cpuUsage;
    int sampleRate;
    int bufferSize;
    int channels;
    bool paused;
    AudioUnit audioUnit;
    AudioRingBuffer ringBuffer;
    short* frameBuffer;
    uint64_t startTime;
    uint64_t totalFrames;
    uint64_t audioTime;
    uint64_t totalTime;
} g_audioState = {
    .initialized = false,
    .muted = false,
    .volume = 1.0f,
    .cpuUsage = 0.0f,
    .sampleRate = DEFAULT_SAMPLE_RATE,
    .bufferSize = DEFAULT_BUFFER_SIZE,
    .channels = NUM_CHANNELS,
    .paused = false,
    .audioUnit = NULL,
    .ringBuffer = {0},
    .frameBuffer = NULL,
    .startTime = 0,
    .totalFrames = 0,
    .audioTime = 0,
    .totalTime = 0
};

// Forward declarations of internal functions
static OSStatus AudioOutputCallback(void* inRefCon, 
                                   AudioUnitRenderActionFlags* ioActionFlags,
                                   const AudioTimeStamp* inTimeStamp,
                                   UInt32 inBusNumber,
                                   UInt32 inNumberFrames,
                                   AudioBufferList* ioData);
static void InitRingBuffer(AudioRingBuffer* rb, int size);
static void FreeRingBuffer(AudioRingBuffer* rb);
static int WriteToRingBuffer(AudioRingBuffer* rb, const short* data, int frames);
static int ReadFromRingBuffer(AudioRingBuffer* rb, short* data, int frames);
static int AvailableInRingBuffer(AudioRingBuffer* rb);
static void FillRingBufferWithSilence(AudioRingBuffer* rb, int frames);

// Initialize the ring buffer
static void InitRingBuffer(AudioRingBuffer* rb, int size) {
    rb->buffer = (short*)malloc(size * sizeof(short));
    rb->size = size;
    rb->readPos = 0;
    rb->writePos = 0;
    rb->available = 0;
    rb->lock = [[NSLock alloc] init];
}

// Free the ring buffer
static void FreeRingBuffer(AudioRingBuffer* rb) {
    if (rb->buffer) {
        free(rb->buffer);
        rb->buffer = NULL;
    }
    rb->lock = nil;
}

// Write data to the ring buffer
static int WriteToRingBuffer(AudioRingBuffer* rb, const short* data, int frames) {
    if (!rb->buffer || !data || frames <= 0) {
        return 0;
    }
    
    [rb->lock lock];
    
    // Calculate available space
    int available = rb->size - rb->available;
    if (available <= 0) {
        [rb->lock unlock];
        return 0;
    }
    
    // Limit frames to available space
    if (frames > available) {
        frames = available;
    }
    
    // Calculate number of samples (stereo = 2 samples per frame)
    int samples = frames * NUM_CHANNELS;
    
    // Write data to buffer
    int writePos = rb->writePos;
    for (int i = 0; i < samples; i++) {
        rb->buffer[writePos] = data[i];
        writePos = (writePos + 1) % rb->size;
    }
    
    // Update write position and available samples
    rb->writePos = writePos;
    rb->available += samples;
    
    [rb->lock unlock];
    
    return frames;
}

// Read data from the ring buffer
static int ReadFromRingBuffer(AudioRingBuffer* rb, short* data, int frames) {
    if (!rb->buffer || !data || frames <= 0) {
        return 0;
    }
    
    [rb->lock lock];
    
    // Calculate available frames
    int availableFrames = rb->available / NUM_CHANNELS;
    if (availableFrames <= 0) {
        [rb->lock unlock];
        return 0;
    }
    
    // Limit frames to available
    if (frames > availableFrames) {
        frames = availableFrames;
    }
    
    // Calculate number of samples (stereo = 2 samples per frame)
    int samples = frames * NUM_CHANNELS;
    
    // Read data from buffer
    int readPos = rb->readPos;
    for (int i = 0; i < samples; i++) {
        data[i] = rb->buffer[readPos];
        readPos = (readPos + 1) % rb->size;
    }
    
    // Update read position and available samples
    rb->readPos = readPos;
    rb->available -= samples;
    
    [rb->lock unlock];
    
    return frames;
}

// Get available frames in the ring buffer
static int AvailableInRingBuffer(AudioRingBuffer* rb) {
    if (!rb->buffer) {
        return 0;
    }
    
    [rb->lock lock];
    int available = rb->available / NUM_CHANNELS;
    [rb->lock unlock];
    
    return available;
}

// Fill ring buffer with silence
static void FillRingBufferWithSilence(AudioRingBuffer* rb, int frames) {
    if (!rb->buffer || frames <= 0) {
        return;
    }
    
    [rb->lock lock];
    
    // Calculate available space
    int available = rb->size - rb->available;
    if (available <= 0) {
        [rb->lock unlock];
        return;
    }
    
    // Limit frames to available space
    if (frames > available / NUM_CHANNELS) {
        frames = available / NUM_CHANNELS;
    }
    
    // Calculate number of samples (stereo = 2 samples per frame)
    int samples = frames * NUM_CHANNELS;
    
    // Write silence to buffer
    int writePos = rb->writePos;
    for (int i = 0; i < samples; i++) {
        rb->buffer[writePos] = 0;
        writePos = (writePos + 1) % rb->size;
    }
    
    // Update write position and available samples
    rb->writePos = writePos;
    rb->available += samples;
    
    [rb->lock unlock];
}

// Core Audio callback function
static OSStatus AudioOutputCallback(void* inRefCon, 
                                   AudioUnitRenderActionFlags* ioActionFlags,
                                   const AudioTimeStamp* inTimeStamp,
                                   UInt32 inBusNumber,
                                   UInt32 inNumberFrames,
                                   AudioBufferList* ioData) {
    // Get timing info for CPU usage calculation
    uint64_t startTime = mach_absolute_time();
    
    // Get audio buffer
    AudioBuffer* buffer = &ioData->mBuffers[0];
    short* outputBuffer = (short*)buffer->mData;
    
    // Check for valid state and buffer
    if (!g_audioState.initialized || !outputBuffer) {
        // Fill buffer with silence
        memset(outputBuffer, 0, inNumberFrames * NUM_CHANNELS * sizeof(short));
        return noErr;
    }
    
    // Check if audio is paused or muted
    if (g_audioState.paused || g_audioState.muted) {
        // Fill buffer with silence
        memset(outputBuffer, 0, inNumberFrames * NUM_CHANNELS * sizeof(short));
        return noErr;
    }
    
    // Read data from ring buffer
    int framesRead = ReadFromRingBuffer(&g_audioState.ringBuffer, outputBuffer, inNumberFrames);
    
    // If we didn't read enough frames, fill the rest with silence
    if (framesRead < inNumberFrames) {
        int remainingFrames = inNumberFrames - framesRead;
        short* remainingBuffer = outputBuffer + (framesRead * NUM_CHANNELS);
        memset(remainingBuffer, 0, remainingFrames * NUM_CHANNELS * sizeof(short));
    }
    
    // Apply volume
    if (g_audioState.volume != 1.0f) {
        for (int i = 0; i < inNumberFrames * NUM_CHANNELS; i++) {
            float sample = outputBuffer[i] * g_audioState.volume;
            
            // Clamp to avoid overflow
            if (sample > 32767.0f) sample = 32767.0f;
            if (sample < -32768.0f) sample = -32768.0f;
            
            outputBuffer[i] = (short)sample;
        }
    }
    
    // Update stats
    g_audioState.totalFrames += inNumberFrames;
    
    // Calculate CPU usage
    uint64_t endTime = mach_absolute_time();
    uint64_t duration = endTime - startTime;
    
    // Update CPU usage calculation (moving average)
    static const float SMOOTHING = 0.95f;
    g_audioState.audioTime = (duration + g_audioState.audioTime * 99) / 100;
    g_audioState.totalTime = ((endTime - g_audioState.startTime) + g_audioState.totalTime * 999) / 1000;
    g_audioState.cpuUsage = SMOOTHING * g_audioState.cpuUsage + 
                           (1.0f - SMOOTHING) * ((float)g_audioState.audioTime / (float)g_audioState.totalTime);
    
    return noErr;
}

// Initialize the audio system
int Metal_AudioInit() {
    // If already initialized, return success
    if (g_audioState.initialized) {
        return 0;
    }
    
    NSLog(@"Metal_AudioInit: Initializing Metal audio system");
    
    // Initialize state
    g_audioState.initialized = false;
    g_audioState.muted = false;
    g_audioState.volume = 1.0f;
    g_audioState.cpuUsage = 0.0f;
    g_audioState.sampleRate = DEFAULT_SAMPLE_RATE;
    g_audioState.bufferSize = DEFAULT_BUFFER_SIZE;
    g_audioState.channels = NUM_CHANNELS;
    g_audioState.paused = false;
    g_audioState.frameBuffer = NULL;
    g_audioState.startTime = mach_absolute_time();
    g_audioState.totalFrames = 0;
    g_audioState.audioTime = 0;
    g_audioState.totalTime = 0;
    
    // Initialize ring buffer
    InitRingBuffer(&g_audioState.ringBuffer, RING_BUFFER_SIZE);
    
    // Allocate frame buffer
    g_audioState.frameBuffer = (short*)malloc(MAX_AUDIO_BUFFER_SIZE * NUM_CHANNELS * sizeof(short));
    if (!g_audioState.frameBuffer) {
        NSLog(@"Metal_AudioInit: Failed to allocate frame buffer");
        return 1;
    }
    
    // Clear frame buffer
    memset(g_audioState.frameBuffer, 0, MAX_AUDIO_BUFFER_SIZE * NUM_CHANNELS * sizeof(short));
    
    // Set up audio component description
    AudioComponentDescription desc;
    desc.componentType = kAudioUnitType_Output;
#if TARGET_OS_IPHONE
    desc.componentSubType = kAudioUnitSubType_RemoteIO;
#else
    desc.componentSubType = kAudioUnitSubType_DefaultOutput;
#endif
    desc.componentManufacturer = kAudioUnitManufacturer_Apple;
    desc.componentFlags = 0;
    desc.componentFlagsMask = 0;
    
    // Find audio component
    AudioComponent component = AudioComponentFindNext(NULL, &desc);
    if (!component) {
        NSLog(@"Metal_AudioInit: Failed to find audio component");
        Metal_AudioExit();
        return 1;
    }
    
    // Create audio unit
    OSStatus status = AudioComponentInstanceNew(component, &g_audioState.audioUnit);
    if (status != noErr) {
        NSLog(@"Metal_AudioInit: Failed to create audio unit (status: %d)", (int)status);
        Metal_AudioExit();
        return 1;
    }
    
    // Set up audio format
    AudioStreamBasicDescription format;
    format.mSampleRate = g_audioState.sampleRate;
    format.mFormatID = kAudioFormatLinearPCM;
    format.mFormatFlags = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
    format.mBitsPerChannel = 16;
    format.mChannelsPerFrame = NUM_CHANNELS;
    format.mFramesPerPacket = 1;
    format.mBytesPerFrame = format.mBitsPerChannel * format.mChannelsPerFrame / 8;
    format.mBytesPerPacket = format.mBytesPerFrame * format.mFramesPerPacket;
    format.mReserved = 0;
    
    // Set audio format
    status = AudioUnitSetProperty(g_audioState.audioUnit,
                                 kAudioUnitProperty_StreamFormat,
                                 kAudioUnitScope_Input,
                                 0,
                                 &format,
                                 sizeof(format));
    if (status != noErr) {
        NSLog(@"Metal_AudioInit: Failed to set audio format (status: %d)", (int)status);
        Metal_AudioExit();
        return 1;
    }
    
    // Set up callback
    AURenderCallbackStruct callback;
    callback.inputProc = AudioOutputCallback;
    callback.inputProcRefCon = NULL;
    
    status = AudioUnitSetProperty(g_audioState.audioUnit,
                                 kAudioUnitProperty_SetRenderCallback,
                                 kAudioUnitScope_Input,
                                 0,
                                 &callback,
                                 sizeof(callback));
    if (status != noErr) {
        NSLog(@"Metal_AudioInit: Failed to set audio callback (status: %d)", (int)status);
        Metal_AudioExit();
        return 1;
    }
    
    // Initialize audio unit
    status = AudioUnitInitialize(g_audioState.audioUnit);
    if (status != noErr) {
        NSLog(@"Metal_AudioInit: Failed to initialize audio unit (status: %d)", (int)status);
        Metal_AudioExit();
        return 1;
    }
    
    // Start audio unit
    status = AudioOutputUnitStart(g_audioState.audioUnit);
    if (status != noErr) {
        NSLog(@"Metal_AudioInit: Failed to start audio unit (status: %d)", (int)status);
        Metal_AudioExit();
        return 1;
    }
    
    // Initialize FBNeo core audio variables
    nBurnSoundRate = g_audioState.sampleRate;
    nBurnSoundLen = g_audioState.bufferSize;
    
    // Mark as initialized
    g_audioState.initialized = true;
    
    NSLog(@"Metal_AudioInit: Audio system initialized successfully");
    return 0;
}

// Shutdown the audio system
int Metal_AudioExit() {
    // If not initialized, do nothing
    if (!g_audioState.initialized) {
        return 0;
    }
    
    NSLog(@"Metal_AudioExit: Shutting down Metal audio system");
    
    // Stop audio unit
    if (g_audioState.audioUnit) {
        AudioOutputUnitStop(g_audioState.audioUnit);
        AudioUnitUninitialize(g_audioState.audioUnit);
        AudioComponentInstanceDispose(g_audioState.audioUnit);
        g_audioState.audioUnit = NULL;
    }
    
    // Free ring buffer
    FreeRingBuffer(&g_audioState.ringBuffer);
    
    // Free frame buffer
    if (g_audioState.frameBuffer) {
        free(g_audioState.frameBuffer);
        g_audioState.frameBuffer = NULL;
    }
    
    // Reset state
    g_audioState.initialized = false;
    
    NSLog(@"Metal_AudioExit: Audio system shutdown complete");
    return 0;
}

// Process audio for the current frame
int Metal_AudioFrame() {
    // If not initialized, return failure
    if (!g_audioState.initialized) {
        return 1;
    }
    
    // If paused or muted, do nothing
    if (g_audioState.paused) {
        return 0;
    }
    
    // Get audio data from FBNeo core
    if (pBurnSoundOut) {
        // Copy data to frame buffer
        memcpy(g_audioState.frameBuffer, pBurnSoundOut, nBurnSoundLen * NUM_CHANNELS * sizeof(short));
        
        // Write to ring buffer
        WriteToRingBuffer(&g_audioState.ringBuffer, g_audioState.frameBuffer, nBurnSoundLen);
    }
    
    return 0;
}

// Check if audio is muted
bool Metal_AudioIsMuted() {
    return g_audioState.muted;
}

// Set audio volume
void Metal_AudioSetVolume(float volume) {
    // Clamp volume to 0.0 - 1.0
    if (volume < 0.0f) volume = 0.0f;
    if (volume > 1.0f) volume = 1.0f;
    
    g_audioState.volume = volume;
}

// Get audio volume
float Metal_AudioGetVolume() {
    return g_audioState.volume;
}

// Mute/unmute audio
void Metal_AudioMute(bool mute) {
    g_audioState.muted = mute;
}

// Set sample rate
int Metal_AudioSetSampleRate(int rate) {
    // Check for valid rate
    if (rate <= 0) {
        return 1;
    }
    
    // If already initialized, we need to reinitialize
    if (g_audioState.initialized) {
        // Store rate
        g_audioState.sampleRate = rate;
        
        // Reinitialize
        Metal_AudioExit();
        return Metal_AudioInit();
    } else {
        // Just store the rate
        g_audioState.sampleRate = rate;
        return 0;
    }
}

// Get sample rate
int Metal_AudioGetSampleRate() {
    return g_audioState.sampleRate;
}

// Get buffer size
int Metal_AudioGetBufferSize() {
    return g_audioState.bufferSize;
}

// Set buffer size
int Metal_AudioSetBufferSize(int size) {
    // Check for valid size
    if (size <= 0 || size > MAX_AUDIO_BUFFER_SIZE) {
        return 1;
    }
    
    // If already initialized, we need to reinitialize
    if (g_audioState.initialized) {
        // Store size
        g_audioState.bufferSize = size;
        
        // Reinitialize
        Metal_AudioExit();
        return Metal_AudioInit();
    } else {
        // Just store the size
        g_audioState.bufferSize = size;
        return 0;
    }
}

// Pause audio
void Metal_AudioPause() {
    g_audioState.paused = true;
}

// Resume audio
void Metal_AudioResume() {
    g_audioState.paused = false;
}

// Get CPU usage
float Metal_AudioGetCPUUsage() {
    return g_audioState.cpuUsage;
}

// Additional helper functions

// Get available frames in the buffer
int Metal_AudioGetBufferAvailable() {
    return AvailableInRingBuffer(&g_audioState.ringBuffer);
}

// Get buffer fill percentage
float Metal_AudioGetBufferFillPercent() {
    if (!g_audioState.initialized) {
        return 0.0f;
    }
    
    int available = AvailableInRingBuffer(&g_audioState.ringBuffer);
    return (float)available / (float)(RING_BUFFER_SIZE / NUM_CHANNELS);
}

// Reset audio buffer
void Metal_AudioResetBuffer() {
    if (!g_audioState.initialized) {
        return;
    }
    
    // Clear ring buffer
    g_audioState.ringBuffer.readPos = 0;
    g_audioState.ringBuffer.writePos = 0;
    g_audioState.ringBuffer.available = 0;
}

// Fill buffer with silence
void Metal_AudioFillWithSilence(int milliseconds) {
    if (!g_audioState.initialized) {
        return;
    }
    
    // Calculate frames from milliseconds
    int frames = (g_audioState.sampleRate * milliseconds) / 1000;
    
    // Fill ring buffer with silence
    FillRingBufferWithSilence(&g_audioState.ringBuffer, frames);
} 