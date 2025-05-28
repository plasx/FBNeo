#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "../metal_declarations.h"
#include "../metal_ai.h"
#include "ai_stub_types.h"

// CoreML stub functions for Metal build

// Initialize CoreML
bool CoreML_Initialize(void) {
    printf("CoreML_Initialize: Initializing CoreML subsystem\n");
    return true;
}

// Shutdown CoreML
void CoreML_Shutdown(void) {
    printf("CoreML_Shutdown: Shutting down CoreML subsystem\n");
}

// Load a CoreML model
bool CoreML_LoadModel(const char* path) {
    printf("CoreML_LoadModel: Loading model from %s\n", path ? path : "default");
    return true;
}

// Get information about the loaded model
bool CoreML_GetModelInfo(struct AIModelInfo* info) {
    if (!info) {
        printf("CoreML_GetModelInfo: Invalid pointer\n");
        return false;
    }
    
    // Fill with sample data
    strcpy(info->name, "FBNeo Default AI Model");
    strcpy(info->version, "1.0.0");
    info->input_width = 384;
    info->input_height = 224;
    info->input_channels = 3;
    info->action_count = 12;
    info->model_type = 0;
    info->compute_backend = 1;
    info->precision = 0;
    info->features = 1;
    info->inference_time_ms = 5;
    info->memory_usage_kb = 15360;
    strcpy(info->game_id, "generic");
    info->game_genre = 0;
    info->is_game_specific = false;
    
    return true;
}

// Process a frame with CoreML
bool CoreML_ProcessFrame(const void* frameData, int width, int height, int pitch, float* results, int resultSize) {
    printf("CoreML_ProcessFrame: Processing %dx%d frame (pitch: %d)\n", width, height, pitch);
    
    // Fill results with dummy data
    if (results && resultSize > 0) {
        // Generate some random-like values for testing
        for (int i = 0; i < resultSize; i++) {
            results[i] = ((float)(i % 10)) / 10.0f;
        }
    }
    
    return true;
}

// Render visualization overlay
bool CoreML_RenderVisualization(void* overlayData, int width, int height, int pitch, int visualizationType) {
    printf("CoreML_RenderVisualization() called\n");
    return overlayData != NULL;
}

// FBNeo core variables (used by audio code)
INT32 nBurnSoundRate = 44100;
INT32 nBurnSoundLen = 1024;
INT16* pBurnSoundOut = NULL;

bool CoreML_FindDefaultModels() {
    printf("CoreML_FindDefaultModels: Looking for default models\n");
    return true;
}

bool CoreML_ConvertFrameToInput(const void* frameData, int width, int height, int pitch, float* inputTensor) {
    printf("CoreML_ConvertFrameToInput: Converting %dx%d frame to tensor\n", width, height);
    
    // This would normalize the pixel data and convert to the format required by CoreML
    if (inputTensor && frameData) {
        const unsigned char* pixels = (const unsigned char*)frameData;
        
        // Simple stub implementation - just normalize pixel values to 0-1 range
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int pixelOffset = y * pitch + x * 4; // Assuming RGBA format
                
                // RGB channels (normalized to 0-1)
                inputTensor[(y * width + x) * 3 + 0] = pixels[pixelOffset + 0] / 255.0f;
                inputTensor[(y * width + x) * 3 + 1] = pixels[pixelOffset + 1] / 255.0f;
                inputTensor[(y * width + x) * 3 + 2] = pixels[pixelOffset + 2] / 255.0f;
            }
        }
    }
    
    return true;
} 