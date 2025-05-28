#pragma once

#include "ai_definitions.h"

#ifdef __cplusplus
extern "C" {
#endif

// CoreML initialization and shutdown
void* CoreML_Initialize();
void CoreML_Shutdown(void* handle);

// Model management
int CoreML_LoadModel(void* handle, const char* path, int enablePrivacy, float noiseScale);
int CoreML_IsModelReady(void* handle);
int CoreML_UnloadModel(void* handle);
const char* CoreML_GetModelInfo(void* handle);

// Inference
float* CoreML_PredictBatch(void* handle, float* inputData, int inputSize, int batchSize, int* outputSize);
int CoreML_PredictWithFrameBuffer(void* handle, unsigned char* frameBuffer, int width, int height, 
                                 float* outputData, int* outputSize);
void CoreML_FreeResult(float* data);

// Model optimization and quantization
int CoreML_OptimizeModel(void* handle, const char* inputPath, const char* outputPath, int precision);
int CoreML_ExportModelForDevice(void* handle, const char* inputPath, const char* outputPath);

// Hardware acceleration settings
void CoreML_EnableHardwareAcceleration(void* handle, int enable);
void CoreML_SetComputeUnits(void* handle, int units);
void CoreML_EnableLowPrecisionAccumulation(void* handle, int enable);

// Model privacy settings
void CoreML_SetDifferentialPrivacyLevel(void* handle, float noiseScale);
void CoreML_EnableModelEncryption(void* handle, int enable);

// Performance monitoring
float CoreML_GetLastInferenceTime(void* handle);
float CoreML_GetAverageInferenceTime(void* handle);
int CoreML_GetPeakMemoryUsage(void* handle);

// Error handling
const char* CoreML_GetLastError(void* handle);

// Advanced PyTorch integration
int CoreML_ConvertPyTorchModel(const char* torchModelPath, const char* coremlOutputPath, 
                              const int* inputShape, int shapeLen, int useNeuralEngine);

// Multi-model management
int CoreML_CreateModelCollection(void* handle);
int CoreML_AddModelToCollection(void* handle, int collectionId, const char* modelPath);
int CoreML_SwitchToModel(void* handle, int collectionId, int modelIndex);
int CoreML_GetModelCount(void* handle, int collectionId);

// Advanced inference options
typedef struct CoreML_InferenceOptions {
    int useCompiler;           // 1 = use compiled model, 0 = interpret
    int forceGPUEvaluation;    // 1 = force GPU, 0 = follow model config
    int allowFallback;         // 1 = allow fallback to CPU, 0 = fail if GPU/ANE not available
    int timeoutMs;             // Timeout in milliseconds, 0 = no timeout
    int useCache;              // 1 = enable prediction cache, 0 = disable cache
    const char* cacheDirectory; // Directory for cache, NULL = default
} CoreML_InferenceOptions;

int CoreML_SetInferenceOptions(void* handle, const CoreML_InferenceOptions* options);

// High-level AI controller
typedef struct CoreML_GameState {
    unsigned char* screenBuffer;  // RGBA screen buffer
    int screenWidth;
    int screenHeight;
    int playerHealth;
    int opponentHealth;
    int playerX;
    int playerY;
    int opponentX;
    int opponentY;
    int frameNumber;
    int gameMode;                 // Current game mode/state
    void* gameSpecificData;       // Game-specific data (optional)
} CoreML_GameState;

typedef struct CoreML_AIAction {
    int buttonUp;
    int buttonDown;
    int buttonLeft;
    int buttonRight;
    int button1;
    int button2;
    int button3;
    int button4;
    int button5;
    int button6;
    int buttonStart;
    int buttonCoin;
    float confidenceLevel;       // Overall confidence 0.0-1.0
    int specialMoveId;           // Game-specific special move ID
    int suggestedHoldFrames;     // Suggested frames to hold input
} CoreML_AIAction;

int CoreML_ProcessGameState(void* handle, const CoreML_GameState* gameState, CoreML_AIAction* action);

#ifdef __cplusplus
}
#endif 