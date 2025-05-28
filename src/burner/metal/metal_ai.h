#pragma once

#include <stdbool.h>
#include "metal_declarations.h"
#include "fixes/ai_stub_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// Basic AI function declarations for FBNeo Metal implementation

// AI Module Initialization and Control
int Metal_InitAI(void);
int Metal_ShutdownAI(void);
int Metal_InitAIForGame(const char* gameId);
int Metal_StartAI(void);
int Metal_StopAI(void);
int Metal_UpdateAI(void);

// AI Status Functions - using bool return type to match implementation
bool Metal_IsAIModuleLoaded(void);
bool Metal_IsAIActive(void);

// AI Settings
int Metal_SetAIDifficulty(int level);
int Metal_SetAIPlayer(int player);
int Metal_EnableAITrainingMode(int enable);
int Metal_EnableAIDebugOverlay(int enable);

// AI Model Management
int Metal_LoadAIModel(const char* modelPath);
int Metal_SaveAIModel(const char* modelPath);
int Metal_GetAIModelInfo(char* infoBuffer, int bufferSize);

// Game State Integration
int Metal_ProvideGameState(void* gameState, int size);
int Metal_ProcessAIFrame(int frameNumber);
int Metal_GetAIAction(void* actionBuffer, int bufferSize);

// AI Utilities
int Metal_SaveFrameData(const char* filename);
int Metal_SetAILogLevel(int level);
int Metal_PerformAIFramebufferAnalysis(void* frameData, int width, int height);

// CoreML Integration - these are defined in coreml_bridge.mm but forward declared here for C code
bool CoreML_Initialize(void);
void CoreML_Shutdown(void);
bool CoreML_LoadModel(const char* path);
bool CoreML_GetModelInfo(struct AIModelInfo* info);
bool CoreML_ProcessFrame(const void* frameData, int width, int height, int pitch, float* results, int resultSize);
bool CoreML_RenderVisualization(void* overlayData, int width, int height, int pitch, int visualizationType);

#ifdef __cplusplus
}
#endif 