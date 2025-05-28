/**
 * @file ai_system_test_mock.cpp
 * @brief Mock implementations of external functions for the AI system test
 */

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include "../fixes/ai_stub_types.h"

// Mock implementations of the external C functions used by ai_stubs.c

extern "C" {

// Mock C++ bridge functions
int AI_Init(const char* configPath) {
    std::cout << "MOCK: AI_Init called with path: " << (configPath ? configPath : "NULL") << std::endl;
    return 1;
}

void AI_Exit() {
    std::cout << "MOCK: AI_Exit called" << std::endl;
}

void AI_SetActive(int enable) {
    std::cout << "MOCK: AI_SetActive(" << enable << ") called" << std::endl;
}

int AI_IsActive() {
    return 1; // Always active for testing
}

void AI_SetTraining(int enable) {
    std::cout << "MOCK: AI_SetTraining(" << enable << ") called" << std::endl;
}

int AI_IsTraining() {
    return 0; // Not in training mode
}

void AI_ProcessFrameBuffer(const void* data, int width, int height, int pitch) {
    std::cout << "MOCK: AI_ProcessFrameBuffer called with " << width << "x" << height << " frame" << std::endl;
}

void AI_StartSession() {
    std::cout << "MOCK: AI_StartSession called" << std::endl;
}

float AI_EndSession(int success) {
    std::cout << "MOCK: AI_EndSession(" << success << ") called" << std::endl;
    return 1.0f;
}

int AI_SaveState(const char* path) {
    std::cout << "MOCK: AI_SaveState(" << path << ") called" << std::endl;
    return 1;
}

int AI_LoadState(const char* path) {
    std::cout << "MOCK: AI_LoadState(" << path << ") called" << std::endl;
    return 1;
}

float AI_GetCurrentActionConfidence() {
    return 0.8f; // Reasonable confidence for testing
}

float AI_GetStateValue() {
    return 0.5f;
}

int AI_GetTopActionCount() {
    return 3;
}

void AI_GetTopActionInfo(int index, char* actionName, float* confidence) {
    const char* actions[] = {"Action 1", "Action 2", "Action 3"};
    const float confs[] = {0.9f, 0.7f, 0.5f};
    
    if (actionName && index >= 0 && index < 3) {
        strcpy(actionName, actions[index]);
    }
    
    if (confidence && index >= 0 && index < 3) {
        *confidence = confs[index];
    }
}

// Mock Metal functions
void* Metal_GetFrameBuffer() {
    static uint8_t buffer[320 * 240 * 4];
    // Initialize with some pattern for testing
    for (int i = 0; i < sizeof(buffer); i++) {
        buffer[i] = i % 256;
    }
    return buffer;
}

int Metal_GetFrameWidth() {
    return 320;
}

int Metal_GetFrameHeight() {
    return 240;
}

// Mock AI functions that aren't explicitly defined in the stubs
bool AI_ApplyActions(const struct AIActions* actions) {
    std::cout << "MOCK: AI_ApplyActions called with " 
              << (actions ? actions->action_count : 0) 
              << " actions" << std::endl;
    return true;
}

struct AIFrameData* AI_CaptureFrame(void) {
    std::cout << "MOCK: AI_CaptureFrame called" << std::endl;
    // Allocate and return a frame buffer
    static uint8_t frame[320 * 240 * 4];
    static struct AIFrameData frameData;
    
    frameData.data = frame;
    frameData.width = 320;
    frameData.height = 240;
    frameData.channels = 4;
    frameData.size = 320 * 240 * 4;
    
    return &frameData;
}

// Mock game state functions
bool AI_GetModelInfo(struct AIModelInfo* info) {
    if (!info) return false;
    
    strcpy(info->name, "Mock AI Model");
    strcpy(info->version, "1.0");
    strcpy(info->game_id, "test_game");
    info->is_game_specific = true;
    info->input_width = 320;
    info->input_height = 240;
    
    return true;
}

// Implementations of system functions
bool AI_InitializeSystem() {
    std::cout << "MOCK: AI_InitializeSystem called" << std::endl;
    return true;
}

void AI_ShutdownSystem() {
    std::cout << "MOCK: AI_ShutdownSystem called" << std::endl;
}

bool AI_LoadModelFile(const char* path) {
    std::cout << "MOCK: AI_LoadModelFile called with path: " << path << std::endl;
    return true;
}

bool AI_Configure(const struct AIConfig* config) {
    std::cout << "MOCK: AI_Configure called" << std::endl;
    
    // Save the configuration so it can be retrieved later
    static struct AIConfig savedConfig;
    
    if (config) {
        memcpy(&savedConfig, config, sizeof(struct AIConfig));
    }
    
    return true;
}

bool AI_GetConfiguration(struct AIConfig* config) {
    if (!config) return false;
    
    // Use the saved configuration if available, otherwise use defaults
    static struct AIConfig savedConfig = {
        true,  // enabled
        2,     // frame_skip
        0.7f,  // confidence_threshold
        "",    // model_path (empty)
        false, // visualization
        true   // debug_mode
    };
    
    // Fill with values
    memcpy(config, &savedConfig, sizeof(struct AIConfig));
    
    return true;
}

void AI_SetEnabled(bool enabled) {
    std::cout << "MOCK: AI_SetEnabled(" << (enabled ? "true" : "false") << ") called" << std::endl;
}

void AI_ConfigureGameMemoryMapping(int gameType, const char* gameId) {
    std::cout << "MOCK: AI_ConfigureGameMemoryMapping(" << gameType << ", " << gameId << ") called" << std::endl;
}

void* AI_GetGameObservation() {
    static uint8_t buffer[1024];
    return buffer;
}

void AI_SetControlledPlayer(int playerIndex) {
    std::cout << "MOCK: AI_SetControlledPlayer(" << playerIndex << ") called" << std::endl;
}

void AI_SetDifficulty(int level) {
    std::cout << "MOCK: AI_SetDifficulty(" << level << ") called" << std::endl;
}

void AI_EnableTrainingMode(int enable) {
    std::cout << "MOCK: AI_EnableTrainingMode(" << enable << ") called" << std::endl;
}

void AI_EnableDebugOverlay(int enable) {
    std::cout << "MOCK: AI_EnableDebugOverlay(" << enable << ") called" << std::endl;
}

void AI_SaveFrameData(const char* filename) {
    std::cout << "MOCK: AI_SaveFrameData(" << filename << ") called" << std::endl;
}

void AI_ProcessFrame() {
    std::cout << "MOCK: AI_ProcessFrame called" << std::endl;
}

bool AI_Predict(const struct AIFrameData* frame_data, struct AIActions* actions) {
    std::cout << "MOCK: AI_Predict called" << std::endl;
    
    if (!actions) return false;
    
    // Fill in a mock action
    actions->action_count = 1;
    actions->actions[0].type = AI_ACTION_BUTTON;
    actions->actions[0].input_id = 0;
    actions->actions[0].value = 1.0f;
    
    return true;
}

} // extern "C" 