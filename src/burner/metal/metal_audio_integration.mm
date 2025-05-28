#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#import <AudioToolbox/AudioToolbox.h>
#import <Cocoa/Cocoa.h>

#include "metal_declarations.h"

// External declarations to FBNeo sound system
extern "C" {
    extern INT16* pBurnSoundOut;      // Buffer for audio output
    extern INT32 nBurnSoundLen;       // Number of samples per frame
    extern INT32 nBurnSoundRate;      // Sample rate (usually 48000)
    
    // Burn sound functions - use only what's not already declared in metal_declarations.h
    int BurnSoundInit();
    void BurnSoundExit();
    // Don't redeclare BurnSoundRender, it's in metal_declarations.h
}

// Audio engine interface
@interface FBNeoAudioEngine : NSObject

// Initialize with sample rate and frame size
- (instancetype)initWithSampleRate:(double)sampleRate frameSize:(int)frameSize;

// Start/stop audio playback
- (BOOL)start;
- (void)stop;

// Process one frame of audio
- (void)processAudioFrame;

@end

// Implementation of Audio Engine
@implementation FBNeoAudioEngine {
    AVAudioEngine *_audioEngine;
    AVAudioSourceNode *_sourceNode;
    double _sampleRate;
    int _frameSize;
    int _channelCount;
    BOOL _isRunning;
    
    // Audio buffer
    INT16 *_audioBuffer;
    int _audioBufferSize;
}

// Initialize with sample rate and frame size
- (instancetype)initWithSampleRate:(double)sampleRate frameSize:(int)frameSize {
    self = [super init];
    if (self) {
        _sampleRate = sampleRate;
        _frameSize = frameSize;
        _channelCount = 2; // Stereo
        _isRunning = NO;
        
        // Allocate audio buffer
        _audioBufferSize = _frameSize * _channelCount;
        _audioBuffer = (INT16 *)malloc(_audioBufferSize * sizeof(INT16));
        if (!_audioBuffer) {
            NSLog(@"[AUDIO INIT] Failed to allocate audio buffer!");
            return nil;
        }
        
        // Clear audio buffer
        memset(_audioBuffer, 0, _audioBufferSize * sizeof(INT16));
        
        // Set up AVAudioEngine
        if (![self setupAudioEngine]) {
            NSLog(@"[AUDIO INIT] Failed to setup audio engine!");
            free(_audioBuffer);
            return nil;
        }
        
        NSLog(@"[AUDIO INIT] Audio engine initialized with %d Hz sample rate, %d frame size", 
              (int)_sampleRate, _frameSize);
    }
    return self;
}

// Set up AVAudioEngine
- (BOOL)setupAudioEngine {
    _audioEngine = [[AVAudioEngine alloc] init];
    if (!_audioEngine) {
        NSLog(@"[AUDIO INIT] Failed to create AVAudioEngine");
        return NO;
    }
    
    // Create audio format
    AVAudioFormat *format = [[AVAudioFormat alloc] 
                             initStandardFormatWithSampleRate:_sampleRate 
                             channels:_channelCount];
    
    // Create source node with render block
    _sourceNode = [[AVAudioSourceNode alloc] 
                   initWithRenderBlock:^OSStatus(BOOL *isSilence, 
                                             const AudioTimeStamp *timestamp, 
                                             AVAudioFrameCount frameCount, 
                                             AudioBufferList *outputData) {
        // Get pointer to output buffer
        float *outBufferLeft = (float *)outputData->mBuffers[0].mData;
        float *outBufferRight = NULL;
        
        if (outputData->mNumberBuffers > 1) {
            // Separate buffers for left and right
            outBufferRight = (float *)outputData->mBuffers[1].mData;
        } else {
            // Interleaved stereo
            outBufferRight = outBufferLeft + 1;
        }
        
        // Check if we have valid audio data
        if (!_isRunning || !_audioBuffer) {
            *isSilence = YES;
            return noErr;
        }
        
        *isSilence = NO;
        
        // Copy audio data to output buffer with conversion from INT16 to float
        const float scale = 1.0f / 32768.0f;
        
        for (int i = 0; i < frameCount && i < _frameSize; i++) {
            // Left channel
            if (outputData->mNumberBuffers > 1) {
                outBufferLeft[i] = _audioBuffer[i * 2] * scale;
                outBufferRight[i] = _audioBuffer[i * 2 + 1] * scale;
            } else {
                // Interleaved
                outBufferLeft[i * 2] = _audioBuffer[i * 2] * scale;
                outBufferLeft[i * 2 + 1] = _audioBuffer[i * 2 + 1] * scale;
            }
        }
        
        return noErr;
    }];
    
    // Connect the source node to the main mixer
    [_audioEngine attachNode:_sourceNode];
    [_audioEngine connect:_sourceNode to:_audioEngine.mainMixerNode format:format];
    
    return YES;
}

// Start audio playback
- (BOOL)start {
    if (_isRunning) {
        return YES;  // Already running
    }
    
    NSError *error = nil;
    
    // Start the audio engine
    if (![_audioEngine startAndReturnError:&error]) {
        NSLog(@"[AUDIO LOOP] Failed to start audio engine: %@", error);
        return NO;
    }
    
    _isRunning = YES;
    NSLog(@"[AUDIO LOOP] Audio engine started");
    return YES;
}

// Stop audio playback
- (void)stop {
    if (!_isRunning) {
        return;  // Already stopped
    }
    
    [_audioEngine stop];
    _isRunning = NO;
    NSLog(@"[AUDIO LOOP] Audio engine stopped");
}

// Process one frame of audio
- (void)processAudioFrame {
    if (!_isRunning || !pBurnSoundOut) {
        return;
    }
    
    // Render audio from FBNeo to our buffer
    BurnSoundRender(pBurnSoundOut, nBurnSoundLen);
    
    // Copy the rendered audio to our buffer
    memcpy(_audioBuffer, pBurnSoundOut, _audioBufferSize * sizeof(INT16));
}

- (void)dealloc {
    [self stop];
    
    if (_audioBuffer) {
        free(_audioBuffer);
        _audioBuffer = NULL;
    }
}

@end

// Global audio engine instance
static FBNeoAudioEngine *g_audioEngine = nil;

// C interface for Metal integration
extern "C" {

// Audio state tracking
typedef struct {
    BOOL initialized;
    BOOL running;
    float volume;
    int sampleRate;
    int frameSize;
    int cpuUsage;
} AudioState;

static AudioState audioState = {
    .initialized = NO,
    .running = NO,
    .volume = 1.0f,
    .sampleRate = 48000,
    .frameSize = 1024,
    .cpuUsage = 0
};

// Initialize Metal audio system
int Metal_InitAudio_UNUSED() {
    NSLog(@"[AUDIO] Initializing Metal audio system");
    
    if (audioState.initialized) {
        NSLog(@"[AUDIO] Audio system already initialized");
        return 0;
    }
    
    // Get sample rate from FBNeo
    if (nBurnSoundRate > 0) {
        audioState.sampleRate = nBurnSoundRate;
    }
    
    // Get frame size from FBNeo
    if (nBurnSoundLen > 0) {
        audioState.frameSize = nBurnSoundLen;
    }
    
    // Create audio engine
    g_audioEngine = [[FBNeoAudioEngine alloc] 
                      initWithSampleRate:audioState.sampleRate 
                      frameSize:audioState.frameSize];
    
    if (!g_audioEngine) {
        NSLog(@"[AUDIO] Failed to create audio engine");
        return -1;
    }
    
    // Start audio engine
    if (![g_audioEngine start]) {
        NSLog(@"[AUDIO] Failed to start audio engine");
        return -1;
    }
    
    audioState.initialized = YES;
    audioState.running = YES;
    
    NSLog(@"[AUDIO] Metal audio system initialized");
    return 0;
}

// Set audio volume (0.0 - 1.0)
void Metal_SetAudioVolume(float volume) {
    audioState.volume = volume;
    NSLog(@"[AUDIO] Set volume to %.2f", volume);
}

// Process one frame of audio
int Metal_AudioFrame() {
    if (!audioState.initialized || !g_audioEngine) {
        return -1;
    }
    
    [g_audioEngine processAudioFrame];
    return 0;
}

// Shutdown audio system
void Metal_ShutdownAudio_UNUSED() {
    if (!audioState.initialized) {
        return;
    }
    
    if (g_audioEngine) {
        [g_audioEngine stop];
        g_audioEngine = nil;
    }
    
    audioState.initialized = NO;
    audioState.running = NO;
    
    NSLog(@"[AUDIO] Metal audio system shutdown");
}

// Pause/resume audio
void Metal_PauseAudio(BOOL pause) {
    if (!audioState.initialized || !g_audioEngine) {
        return;
    }
    
    if (pause && audioState.running) {
        [g_audioEngine stop];
        audioState.running = NO;
        NSLog(@"[AUDIO] Audio paused");
    } else if (!pause && !audioState.running) {
        [g_audioEngine start];
        audioState.running = YES;
        NSLog(@"[AUDIO] Audio resumed");
    }
}

// Get CPU usage percentage of audio thread (0-100)
float Metal_GetAudioCPUUsage() {
    return audioState.cpuUsage / 100.0f;
}

// Get audio buffer fill level (0.0 - 1.0)
float Metal_GetBufferFillLevel() {
    return 0.5f; // Default 50% fill level
}

// FBNeo interface functions
int FBNeo_AudioInit() {
    int result = Metal_InitAudio();
    
    if (result == 0) {
        // Set initial volume
        Metal_SetAudioVolume(audioState.volume);
    }
    
    return result;
}

int FBNeo_AudioFrame() {
    return Metal_AudioFrame();
}

void FBNeo_AudioExit() {
    Metal_ShutdownAudio();
}

void FBNeo_SetAudioVolume(float volume) {
    Metal_SetAudioVolume(volume);
}

// Add missing audio function implementations
void FBNeo_AudioPause(int pause) {
    if (!audioState.initialized) {
        return;
    }
    
    Metal_PauseAudio(pause ? YES : NO);
}

// Helper to convert 0-100 integer volume to float (0.0-1.0)
void FBNeo_AudioSetVolumePercent(int volumePercent) {
    // Clamp to 0-100 range
    if (volumePercent < 0) volumePercent = 0;
    if (volumePercent > 100) volumePercent = 100;
    
    // Convert to float (0.0-1.0)
    float volume = (float)volumePercent / 100.0f;
    
    Metal_SetAudioVolume(volume);
}

// Legacy function wrappers for compatibility with existing code

// For compatibility with BurnSoundCheck from burn.cpp
extern "C" int AudSoundInit() {
    return FBNeo_AudioInit();
}

// For compatibility with BurnSoundExit from burn.cpp
extern "C" int AudSoundExit() {
    FBNeo_AudioExit();
    return 0;
}

// For compatibility with BurnSoundPlay from burn.cpp
extern "C" int AudSoundPlay() {
    FBNeo_AudioPause(0); // Resume
    return 0;
}

// For compatibility with BurnSoundStop from burn.cpp
extern "C" int AudSoundStop() {
    FBNeo_AudioPause(1); // Pause
    return 0;
}

// For compatibility with BurnSound functions
extern "C" int AudSoundSetVolume(int nVolume) {
    FBNeo_AudioSetVolumePercent(nVolume);
    return 0;
}

// Wrapper for callback compatibility
extern "C" int AudSetCallback(int (*pCallback)(int)) {
    // In our implementation, we don't use callbacks directly
    // The integration is done via Metal_AudioFrame
    return 0;
}
} 