#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>  // For NULL definition
#include <string.h>  // For memset
#include <stdio.h>   // For printf
#include <math.h>    // For sinf
#include "c_cpp_compatibility.h"
#include "patched_tiles_generic.h"

/*
 * C wrapper functions implementation
 * 
 * This file provides C-compatible implementations of functions
 * that have default arguments or other C++ features that don't
 * work in C mode.
 */

// Type definitions needed for wrapper functions
typedef int32_t INT32;
typedef uint32_t UINT32;
typedef int16_t INT16;
typedef uint16_t UINT16;
typedef int8_t INT8;
typedef uint8_t UINT8;

// We can't directly forward declare the original functions with their C++ default args
// So we declare them with different names that we'll link to the original implementations

// IpsApplyPatches wrapper
void IpsApplyPatches_C(unsigned char* base, char* rom_name, unsigned int rom_crc, bool readonly) {
    // In a real implementation, this would call the original C++ function
    // For now, it's a stub implementation
    // We'll link to the real function in the final build
}

// GenericTilemapDraw wrapper
void GenericTilemapDraw_C(INT32 which, UINT16* Bitmap, INT32 priority, INT32 priority_mask) {
    // In a real implementation, this would call the original C++ function
    // For now, it's a stub implementation
    // We'll link to the real function in the final build
}

// GenericTilemapSetOffsets wrapper with 3 parameters
void GenericTilemapSetOffsets_3Param(INT32 which, INT32 x, INT32 y) {
    // In a real implementation, this would call the original C++ function
    // For now, it's a stub implementation
    // We'll link to the real function in the final build
}

// GenericTilemapSetOffsets wrapper with 5 parameters
void GenericTilemapSetOffsets_5Param(INT32 which, INT32 x, INT32 y, INT32 x_flipped, INT32 y_flipped) {
    // In a real implementation, this would call the original C++ function
    // For now, it's a stub implementation
    // We'll link to the real function in the final build
}

// Stub for BurnBitmapClipDims to avoid the clip_struct tag issue
struct clip_struct* BurnBitmapClipDims(INT32 nBitmapNumber) {
    // Stub implementation - will need to be updated with the real implementation
    return NULL;
}

// Stub for accessing GenericGfxData
static struct GenericTilesGfx gfx_data_stub = {0};
struct GenericTilesGfx GenericGfxData[8] = {
    {0},{0},{0},{0},{0},{0},{0},{0}
};

// Additional key stubs that might be needed
void GenericTilesInit() {}
void GenericTilesExit() {}
void GenericTilesClearScreen() {}
void GenericTilesClearClipRect(INT32 nBitmap) {}
void GenericTileSetClipRect(INT32 nBitmap, INT32 left, INT32 top, INT32 right, INT32 bottom) {}
void GenericTileGetClipRect(INT32 nBitmap, INT32* left, INT32* top, INT32* right, INT32* bottom) {}

/*
 * Audio System C compatibility functions
 * This section provides C-compatible implementations of audio functions
 * that bridge between the C and C++ code.
 */

// Audio system constants
#define AUDIO_MAX_BUFFER_SIZE 8192
#define AUDIO_DEFAULT_SAMPLE_RATE 44100
#define AUDIO_DEFAULT_BUFFER_SIZE 882  // ~20ms at 44.1kHz

// Audio system state
static struct {
    bool initialized;
    int sample_rate;
    int buffer_size;
    UINT16* buffer;
    bool active;
    int current_position;
    int volume;
    float master_volume;
} g_audio_state = {
    .initialized = false,
    .sample_rate = AUDIO_DEFAULT_SAMPLE_RATE,
    .buffer_size = AUDIO_DEFAULT_BUFFER_SIZE,
    .buffer = NULL,
    .active = false,
    .current_position = 0,
    .volume = 100,
    .master_volume = 1.0f
};

// Forward declarations for C++ functions we're bridging to
extern void BurnSoundCheck();
extern void BurnSoundExit();
extern void BurnSoundStop();
extern void BurnSoundPlay();
extern bool BurnSoundGetStatus();
extern int BurnSoundSetVolume(int volume);
extern void BurnSoundRender(INT16* pDest, INT32 nLen);

// Function declarations to fix order dependencies
void Audio_Exit_C(void);
void Audio_Stop_C(void);
void Audio_ExitMetal_C(void);

// Audio system initialization
int Audio_Init_C(int sample_rate, int buffer_size) {
    // If already initialized, clean up first
    if (g_audio_state.initialized) {
        Audio_Exit_C();
    }
    
    printf("Audio_Init_C: Initializing audio system (sample_rate=%d, buffer_size=%d)\n", 
           sample_rate, buffer_size);
    
    // Validate parameters
    if (sample_rate <= 0) {
        sample_rate = AUDIO_DEFAULT_SAMPLE_RATE;
    }
    
    if (buffer_size <= 0 || buffer_size > AUDIO_MAX_BUFFER_SIZE) {
        buffer_size = AUDIO_DEFAULT_BUFFER_SIZE;
    }
    
    // Initialize state
    g_audio_state.sample_rate = sample_rate;
    g_audio_state.buffer_size = buffer_size;
    g_audio_state.current_position = 0;
    g_audio_state.volume = 100;
    g_audio_state.master_volume = 1.0f;
    
    // Allocate buffer (stereo - 2 channels)
    g_audio_state.buffer = (UINT16*)malloc(buffer_size * 2 * sizeof(UINT16));
    if (!g_audio_state.buffer) {
        printf("Audio_Init_C: Failed to allocate audio buffer\n");
        return 0;
    }
    
    // Clear buffer
    memset(g_audio_state.buffer, 0, buffer_size * 2 * sizeof(UINT16));
    
    // Set up global audio variables for the FBNeo core
    // These are declared in burner.h and used by the core
    extern int nBurnSoundRate;
    extern int nBurnSoundLen;
    extern short* pBurnSoundOut;
    
    nBurnSoundRate = sample_rate;
    nBurnSoundLen = buffer_size;
    pBurnSoundOut = (short*)g_audio_state.buffer;
    
    // Mark as initialized
    g_audio_state.initialized = true;
    g_audio_state.active = false;
    
    // Call the BurnSound initialization function
    BurnSoundCheck();
    
    return 1;
}

// Audio system shutdown
void Audio_Exit_C() {
    if (!g_audio_state.initialized) {
        return;
    }
    
    printf("Audio_Exit_C: Shutting down audio system\n");
    
    // If active, stop playback
    if (g_audio_state.active) {
        Audio_Stop_C();
    }
    
    // Free buffer
    if (g_audio_state.buffer) {
        free(g_audio_state.buffer);
        g_audio_state.buffer = NULL;
    }
    
    // Reset state
    g_audio_state.initialized = false;
    g_audio_state.active = false;
    
    // Call the BurnSound exit function
    BurnSoundExit();
}

// Start audio playback
void Audio_Play_C() {
    if (!g_audio_state.initialized) {
        return;
    }
    
    printf("Audio_Play_C: Starting audio playback\n");
    
    g_audio_state.active = true;
    
    // Call the BurnSound play function
    BurnSoundPlay();
}

// Stop audio playback
void Audio_Stop_C() {
    if (!g_audio_state.initialized || !g_audio_state.active) {
        return;
    }
    
    printf("Audio_Stop_C: Stopping audio playback\n");
    
    g_audio_state.active = false;
    
    // Call the BurnSound stop function
    BurnSoundStop();
}

// Check if audio is active
bool Audio_IsActive_C() {
    if (!g_audio_state.initialized) {
        return false;
    }
    
    // Get status from BurnSound
    return BurnSoundGetStatus();
}

// Set audio volume
int Audio_SetVolume_C(int volume) {
    if (!g_audio_state.initialized) {
        return 0;
    }
    
    // Clamp volume to 0-100
    if (volume < 0) volume = 0;
    if (volume > 100) volume = 100;
    
    g_audio_state.volume = volume;
    g_audio_state.master_volume = volume / 100.0f;
    
    // Call the BurnSound volume function
    return BurnSoundSetVolume(volume);
}

// Get audio volume
int Audio_GetVolume_C() {
    return g_audio_state.volume;
}

// Reset audio buffer
void Audio_ResetBuffer_C() {
    if (!g_audio_state.initialized || !g_audio_state.buffer) {
        return;
    }
    
    memset(g_audio_state.buffer, 0, g_audio_state.buffer_size * 2 * sizeof(UINT16));
    g_audio_state.current_position = 0;
}

// Process a frame of audio
int Audio_RenderFrame_C(INT16* dest_buffer, INT32 len) {
    if (!g_audio_state.initialized || !g_audio_state.buffer || !dest_buffer || len <= 0) {
        return 0;
    }
    
    // Call the BurnSound render function
    BurnSoundRender(dest_buffer, len);
    
    return 1;
}

// Generate a test tone (useful for debugging)
void Audio_GenerateTestTone_C(INT16* buffer, INT32 len, int frequency, int amplitude) {
    if (!buffer || len <= 0) {
        return;
    }
    
    static int phase = 0;
    const float two_pi = 2.0f * 3.14159265358979323846f;
    const float phase_inc = two_pi * frequency / g_audio_state.sample_rate;
    
    for (int i = 0; i < len; i++) {
        float sample = sinf(phase) * amplitude;
        buffer[i * 2] = (INT16)sample;      // Left channel
        buffer[i * 2 + 1] = (INT16)sample;  // Right channel
        
        phase += phase_inc;
        if (phase >= two_pi) {
            phase -= two_pi;
        }
    }
}

// Get the current audio buffer size
int Audio_GetBufferSize_C() {
    return g_audio_state.buffer_size;
}

// Get the current audio sample rate
int Audio_GetSampleRate_C() {
    return g_audio_state.sample_rate;
}

// Get the audio buffer
INT16* Audio_GetBuffer_C() {
    return (INT16*)g_audio_state.buffer;
}

// Apply volume to a buffer
void Audio_ApplyVolume_C(INT16* buffer, INT32 len, float volume) {
    if (!buffer || len <= 0) {
        return;
    }
    
    for (int i = 0; i < len * 2; i++) {
        float sample = buffer[i] * volume;
        
        // Clamp to INT16 range
        if (sample > 32767.0f) sample = 32767.0f;
        if (sample < -32768.0f) sample = -32768.0f;
        
        buffer[i] = (INT16)sample;
    }
}

// Mix two audio buffers
void Audio_MixBuffers_C(INT16* dest, const INT16* src, INT32 len, float volume) {
    if (!dest || !src || len <= 0) {
        return;
    }
    
    for (int i = 0; i < len * 2; i++) {
        float sample = dest[i] + (src[i] * volume);
        
        // Clamp to INT16 range
        if (sample > 32767.0f) sample = 32767.0f;
        if (sample < -32768.0f) sample = -32768.0f;
        
        dest[i] = (INT16)sample;
    }
}

// Additional CoreAudio integration with Metal
// Forward declarations for Metal CoreAudio bridge functions
extern int MetalAudio_Initialize(int sampleRate, int channelCount);
extern void MetalAudio_Shutdown();
extern void MetalAudio_SetCallback(void (*callback)(void*, int));
extern bool MetalAudio_IsSuspended();
extern int MetalAudio_GetSampleRate();
extern int MetalAudio_GetChannelCount();
extern void MetalAudio_Pause();
extern void MetalAudio_Resume();
extern float MetalAudio_GetCPULoad();
extern void MetalAudio_SetMasterVolume(float volume);
extern float MetalAudio_GetMasterVolume();

// Enhanced Metal audio callback system
typedef void (*AudioCallbackFunc)(INT16* buffer, int samples);
static AudioCallbackFunc g_audio_callback = NULL;
static int g_callback_samples = 0;
static INT16* g_callback_buffer = NULL;
static bool g_audio_suspended = false;
static float g_cpu_load = 0.0f;
static int g_audio_channel_count = 2;
static int g_last_frame_size = 0;

// Audio callback handler for CoreAudio integration
static void Audio_CallbackHandler(void* buffer, int frames) {
    // Skip processing if audio is suspended
    if (g_audio_suspended || !g_audio_state.initialized) {
        // Fill buffer with silence
        memset(buffer, 0, frames * g_audio_channel_count * sizeof(INT16));
        return;
    }
    
    // If we have a user callback, call it
    if (g_audio_callback && g_callback_buffer) {
        // Call user callback to fill temp buffer
        g_audio_callback(g_callback_buffer, frames);
        
        // Apply master volume
        if (g_audio_state.master_volume != 1.0f) {
            Audio_ApplyVolume_C(g_callback_buffer, frames, g_audio_state.master_volume);
        }
        
        // Copy to output buffer
        memcpy(buffer, g_callback_buffer, frames * g_audio_channel_count * sizeof(INT16));
    } else {
        // No callback, try to call BurnSoundRender directly
        INT16* dest = (INT16*)buffer;
        BurnSoundRender(dest, frames);
        
        // Apply master volume
        if (g_audio_state.master_volume != 1.0f) {
            Audio_ApplyVolume_C(dest, frames, g_audio_state.master_volume);
        }
    }
    
    // Store frame size for diagnostics
    g_last_frame_size = frames;
}

// Initialize Metal CoreAudio integration
int Audio_InitMetal_C(int sample_rate, int channels, int buffer_frames) {
    printf("Audio_InitMetal_C: Initializing Metal CoreAudio (sample_rate=%d, channels=%d, buffer_frames=%d)\n", 
           sample_rate, channels, buffer_frames);
    
    // If already initialized, clean up first
    if (g_audio_state.initialized) {
        Audio_ExitMetal_C();
    }
    
    // Set up local state
    g_audio_state.sample_rate = sample_rate;
    g_audio_state.buffer_size = buffer_frames;
    g_audio_channel_count = channels;
    g_audio_state.volume = 100;
    g_audio_state.master_volume = 1.0f;
    g_audio_suspended = false;
    
    // Allocate callback buffer if needed (double size for safety)
    g_callback_buffer = (INT16*)malloc(buffer_frames * channels * sizeof(INT16) * 2);
    if (!g_callback_buffer) {
        printf("Audio_InitMetal_C: Failed to allocate callback buffer\n");
        return 0;
    }
    
    // Set up global audio variables for the FBNeo core
    extern int nBurnSoundRate;
    extern int nBurnSoundLen;
    extern short* pBurnSoundOut;
    
    nBurnSoundRate = sample_rate;
    nBurnSoundLen = buffer_frames;
    pBurnSoundOut = g_callback_buffer;
    
    // Initialize Metal CoreAudio
    int result = MetalAudio_Initialize(sample_rate, channels);
    if (result == 0) {
        printf("Audio_InitMetal_C: Failed to initialize Metal CoreAudio\n");
        free(g_callback_buffer);
        g_callback_buffer = NULL;
        return 0;
    }
    
    // Set callback
    MetalAudio_SetCallback(Audio_CallbackHandler);
    
    // Mark as initialized
    g_audio_state.initialized = true;
    g_audio_state.active = true;
    
    // Initialize BurnSound
    BurnSoundCheck();
    
    return 1;
}

// Shutdown Metal CoreAudio integration
void Audio_ExitMetal_C() {
    if (!g_audio_state.initialized) {
        return;
    }
    
    printf("Audio_ExitMetal_C: Shutting down Metal CoreAudio\n");
    
    // Shutdown Metal CoreAudio
    MetalAudio_Shutdown();
    
    // Free callback buffer
    if (g_callback_buffer) {
        free(g_callback_buffer);
        g_callback_buffer = NULL;
    }
    
    // Reset state
    g_audio_state.initialized = false;
    g_audio_state.active = false;
    g_audio_callback = NULL;
    g_callback_samples = 0;
    
    // Shutdown BurnSound
    BurnSoundExit();
}

// Register a callback for audio processing
void Audio_SetCallback_C(AudioCallbackFunc callback, int callback_buffer_size) {
    printf("Audio_SetCallback_C: Setting audio callback (buffer_size=%d)\n", callback_buffer_size);
    
    g_audio_callback = callback;
    g_callback_samples = callback_buffer_size;
}

// Suspend/resume audio processing
void Audio_Suspend_C(bool suspend) {
    printf("Audio_Suspend_C: %s audio\n", suspend ? "Suspending" : "Resuming");
    
    g_audio_suspended = suspend;
    
    if (suspend) {
        MetalAudio_Pause();
    } else {
        MetalAudio_Resume();
    }
}

// Get the CPU load from the audio thread
float Audio_GetCPULoad_C() {
    if (!g_audio_state.initialized) {
        return 0.0f;
    }
    
    return MetalAudio_GetCPULoad();
}

// Get the latency in milliseconds
float Audio_GetLatency_C() {
    if (!g_audio_state.initialized) {
        return 0.0f;
    }
    
    // Calculate latency based on buffer size
    return (float)g_last_frame_size * 1000.0f / (float)g_audio_state.sample_rate;
}

// Set the channel count
void Audio_SetChannelCount_C(int channels) {
    if (channels < 1 || channels > 8) {
        return;
    }
    
    g_audio_channel_count = channels;
}

// Get the channel count
int Audio_GetChannelCount_C() {
    return g_audio_channel_count;
}

// Process a specific number of frames with a custom callback
int Audio_ProcessFrames_C(INT16* buffer, int frames, void (*custom_callback)(INT16*, int, void*), void* user_data) {
    if (!buffer || frames <= 0 || !custom_callback) {
        return 0;
    }
    
    // Call the custom callback with user data
    custom_callback(buffer, frames, user_data);
    
    // Apply master volume if needed
    if (g_audio_state.master_volume != 1.0f) {
        Audio_ApplyVolume_C(buffer, frames, g_audio_state.master_volume);
    }
    
    return frames;
}

// Enhanced audio format conversion for 8/16/24/32-bit samples
void Audio_ConvertFormat_C(void* dest, const void* src, int samples, int src_format, int dest_format) {
    if (!dest || !src || samples <= 0) {
        return;
    }
    
    // Format codes: 0=8-bit, 1=16-bit, 2=24-bit, 3=32-bit int, 4=32-bit float
    
    // Special case: same format, just copy the data
    if (src_format == dest_format) {
        int bytes_per_sample = 0;
        switch (src_format) {
            case 0: bytes_per_sample = 1; break; // 8-bit
            case 1: bytes_per_sample = 2; break; // 16-bit
            case 2: bytes_per_sample = 3; break; // 24-bit
            case 3: case 4: bytes_per_sample = 4; break; // 32-bit
            default: return;
        }
        
        memcpy(dest, src, samples * bytes_per_sample);
        return;
    }
    
    // Implementation for other conversions would go here
    // This is a complex task that would require extensive code
    // For now, we only implement a few common conversions
    
    // 8-bit to 16-bit conversion
    if (src_format == 0 && dest_format == 1) {
        const uint8_t* src8 = (const uint8_t*)src;
        int16_t* dest16 = (int16_t*)dest;
        
        for (int i = 0; i < samples; i++) {
            // Convert 8-bit unsigned to 16-bit signed
            dest16[i] = ((int16_t)src8[i] - 128) << 8;
        }
        return;
    }
    
    // 16-bit to 8-bit conversion
    if (src_format == 1 && dest_format == 0) {
        const int16_t* src16 = (const int16_t*)src;
        uint8_t* dest8 = (uint8_t*)dest;
        
        for (int i = 0; i < samples; i++) {
            // Convert 16-bit signed to 8-bit unsigned
            dest8[i] = (uint8_t)((src16[i] >> 8) + 128);
        }
        return;
    }
    
    // 16-bit to 32-bit float conversion
    if (src_format == 1 && dest_format == 4) {
        const int16_t* src16 = (const int16_t*)src;
        float* dest32f = (float*)dest;
        
        for (int i = 0; i < samples; i++) {
            // Convert 16-bit signed to 32-bit float (-1.0 to 1.0)
            dest32f[i] = src16[i] / 32768.0f;
        }
        return;
    }
    
    // 32-bit float to 16-bit conversion
    if (src_format == 4 && dest_format == 1) {
        const float* src32f = (const float*)src;
        int16_t* dest16 = (int16_t*)dest;
        
        for (int i = 0; i < samples; i++) {
            // Convert 32-bit float (-1.0 to 1.0) to 16-bit signed
            float sample = src32f[i] * 32768.0f;
            
            // Clamp to INT16 range
            if (sample > 32767.0f) sample = 32767.0f;
            if (sample < -32768.0f) sample = -32768.0f;
            
            dest16[i] = (int16_t)sample;
        }
        return;
    }
    
    // For unsupported conversions, just fill with zeros
    printf("Audio_ConvertFormat_C: Unsupported format conversion from %d to %d\n", 
           src_format, dest_format);
           
    int dest_bytes = 0;
    switch (dest_format) {
        case 0: dest_bytes = samples; break; // 8-bit
        case 1: dest_bytes = samples * 2; break; // 16-bit
        case 2: dest_bytes = samples * 3; break; // 24-bit
        case 3: case 4: dest_bytes = samples * 4; break; // 32-bit
        default: return;
    }
    
    memset(dest, 0, dest_bytes);
}

// Set master volume with decibel value
void Audio_SetVolumeDB_C(float decibels) {
    // Convert dB to linear scale
    // 0 dB = 1.0, -6 dB = 0.5, -12 dB = 0.25, etc.
    if (decibels <= -96.0f) {
        // Silence
        g_audio_state.master_volume = 0.0f;
    } else {
        g_audio_state.master_volume = powf(10.0f, decibels / 20.0f);
    }
    
    // Update Metal audio volume
    MetalAudio_SetMasterVolume(g_audio_state.master_volume);
    
    // Calculate integer volume (0-100)
    g_audio_state.volume = (int)(g_audio_state.master_volume * 100.0f);
    if (g_audio_state.volume > 100) g_audio_state.volume = 100;
}

// Get master volume in decibels
float Audio_GetVolumeDB_C() {
    if (g_audio_state.master_volume <= 0.0001f) {
        return -96.0f;  // Silence
    }
    
    return 20.0f * log10f(g_audio_state.master_volume);
}

// Metal-specific audio integration functions

// Method to update audio from a game frame
void Metal_AudioUpdate_C(INT16* game_audio, int frames) {
    if (!g_audio_state.initialized || !game_audio || frames <= 0) {
        return;
    }
    
    // If we have a callback buffer, copy the game audio to it
    if (g_callback_buffer) {
        int copy_size = frames * g_audio_channel_count * sizeof(INT16);
        if (copy_size > g_audio_state.buffer_size * g_audio_channel_count * sizeof(INT16) * 2) {
            // Safety check - the buffer shouldn't be this large
            copy_size = g_audio_state.buffer_size * g_audio_channel_count * sizeof(INT16) * 2;
        }
        
        memcpy(g_callback_buffer, game_audio, copy_size);
    }
}

// FBNeo core bridge for Metal audio
int FBNeo_AudioInit_C(int sample_rate, int buffer_size) {
    return Audio_InitMetal_C(sample_rate, 2, buffer_size);
}

void FBNeo_AudioExit_C() {
    Audio_ExitMetal_C();
}

// Update function called from FBNeo core
void FBNeo_AudioUpdate_C(INT16* buffer, int frames) {
    Metal_AudioUpdate_C(buffer, frames);
}

// Metal Debug Audio Functions

// Generate a sweep tone (useful for testing audio system)
void Audio_GenerateSweepTone_C(INT16* buffer, int frames, float start_freq, float end_freq, float amplitude) {
    if (!buffer || frames <= 0) {
        return;
    }
    
    static float phase = 0.0f;
    const float two_pi = 2.0f * 3.14159265358979323846f;
    
    // Calculate phase increment for sweep
    float freq_range = end_freq - start_freq;
    float freq_inc = freq_range / frames;
    float current_freq = start_freq;
    
    for (int i = 0; i < frames; i++) {
        float sample = sinf(phase) * amplitude;
        
        // Fill stereo channels
        buffer[i * 2] = (INT16)sample;
        buffer[i * 2 + 1] = (INT16)sample;
        
        // Update frequency for sweep
        current_freq += freq_inc;
        
        // Calculate phase increment for current frequency
        float phase_inc = two_pi * current_freq / g_audio_state.sample_rate;
        
        // Update phase
        phase += phase_inc;
        if (phase >= two_pi) {
            phase -= two_pi;
        }
    }
}

// Generate a noise burst (useful for testing audio system)
void Audio_GenerateNoise_C(INT16* buffer, int frames, float amplitude) {
    if (!buffer || frames <= 0) {
        return;
    }
    
    for (int i = 0; i < frames; i++) {
        // Generate white noise
        float noise = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
        INT16 sample = (INT16)(noise * amplitude);
        
        // Fill stereo channels
        buffer[i * 2] = sample;
        buffer[i * 2 + 1] = sample;
    }
}

// Apply low-pass filter to audio buffer
void Audio_ApplyLowPassFilter_C(INT16* buffer, int frames, float cutoff) {
    if (!buffer || frames <= 0) {
        return;
    }
    
    // Simple one-pole low-pass filter
    static float prev_left = 0.0f;
    static float prev_right = 0.0f;
    
    // Calculate filter coefficient
    float sample_rate = (float)g_audio_state.sample_rate;
    float rc = 1.0f / (2.0f * 3.14159265358979323846f * cutoff);
    float dt = 1.0f / sample_rate;
    float alpha = dt / (rc + dt);
    
    for (int i = 0; i < frames; i++) {
        // Get input samples
        float left = (float)buffer[i * 2];
        float right = (float)buffer[i * 2 + 1];
        
        // Apply filter
        prev_left = prev_left + alpha * (left - prev_left);
        prev_right = prev_right + alpha * (right - prev_right);
        
        // Write filtered samples back to buffer
        buffer[i * 2] = (INT16)prev_left;
        buffer[i * 2 + 1] = (INT16)prev_right;
    }
}

// Apply high-pass filter to audio buffer
void Audio_ApplyHighPassFilter_C(INT16* buffer, int frames, float cutoff) {
    if (!buffer || frames <= 0) {
        return;
    }
    
    // Simple one-pole high-pass filter
    static float prev_left_in = 0.0f;
    static float prev_left_out = 0.0f;
    static float prev_right_in = 0.0f;
    static float prev_right_out = 0.0f;
    
    // Calculate filter coefficient
    float sample_rate = (float)g_audio_state.sample_rate;
    float rc = 1.0f / (2.0f * 3.14159265358979323846f * cutoff);
    float dt = 1.0f / sample_rate;
    float alpha = rc / (rc + dt);
    
    for (int i = 0; i < frames; i++) {
        // Get input samples
        float left = (float)buffer[i * 2];
        float right = (float)buffer[i * 2 + 1];
        
        // Apply filter
        float left_out = alpha * (prev_left_out + left - prev_left_in);
        float right_out = alpha * (prev_right_out + right - prev_right_in);
        
        // Update state
        prev_left_in = left;
        prev_left_out = left_out;
        prev_right_in = right;
        prev_right_out = right_out;
        
        // Write filtered samples back to buffer
        buffer[i * 2] = (INT16)left_out;
        buffer[i * 2 + 1] = (INT16)right_out;
    }
}

// Additional C wrapper functions can be added here as needed 