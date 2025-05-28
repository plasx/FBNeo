#ifndef _AI_STUB_TYPES_H_
#define _AI_STUB_TYPES_H_

/*
 * This header contains simplified type definitions for AI integration
 * in the Metal build. These types will be used in stub implementations
 * until the full AI integration is complete.
 */

#include <stdint.h>
#include <stdbool.h>

// Maximum dimension sizes for frame data
#define MAX_FRAME_WIDTH 1024
#define MAX_FRAME_HEIGHT 1024
#define MAX_ACTION_COUNT 32

// Frame data structure passed to AI processing
struct AIFrameData {
    uint8_t* data;           // Pointer to frame pixel data
    uint32_t width;          // Frame width in pixels
    uint32_t height;         // Frame height in pixels
    uint32_t channels;       // Number of channels (e.g., 4 for RGBA)
    uint32_t size;           // Total size in bytes
};

// Action types that can be produced by AI
enum AIActionType {
    AI_ACTION_NONE = 0,
    AI_ACTION_BUTTON = 1,    // Button press/release
    AI_ACTION_JOYSTICK = 2,  // Analog joystick movement
    AI_ACTION_SPECIAL = 3    // Special game-specific action
};

// Individual action produced by AI
struct AIAction {
    enum AIActionType type;  // Type of action
    uint32_t input_id;       // Input identifier (button/axis ID)
    float value;             // Action value (0-1 for buttons, -1 to 1 for joysticks)
    bool active;             // Is this action active?
    float confidence;        // Confidence level (0-1)
    char name[32];           // Action name (like "BUTTON_A", "LEFT", etc.)
};

// Collection of actions produced by AI
struct AIActions {
    struct AIAction actions[MAX_ACTION_COUNT];  // Array of actions
    uint32_t action_count;                      // Number of actions
};

// AI model information
struct AIModelInfo {
    char name[256];          // Model name
    char version[64];        // Model version
    char game_id[64];        // Game identifier this model is trained for
    bool is_game_specific;   // Whether this model is game-specific
    uint32_t input_width;    // Expected input width
    uint32_t input_height;   // Expected input height
    uint32_t input_channels; // Expected input channels
    uint32_t action_count;   // Number of actions
    int model_type;          // Type of model (0 = CNN, 1 = RNN, etc.)
    int compute_backend;     // Compute backend (0 = CPU, 1 = GPU)
    int precision;           // Model precision (0 = FP32, 1 = FP16)
    unsigned int features;   // Feature flags
    int inference_time_ms;   // Typical inference time in milliseconds
    unsigned int memory_usage_kb; // Memory usage in KB
    unsigned int game_genre; // Game genre flags
    unsigned int reserved[4]; // Reserved for future expansion
};

// AI configuration options
struct AIConfig {
    bool enabled;            // Whether AI is active
    uint32_t frame_skip;     // Process every Nth frame (0 = every frame)
    float confidence_threshold; // Minimum confidence for actions (0-1)
    char model_path[1024];   // Path to the model file
    bool visualization;      // Whether to show visualization overlays
    bool debug_mode;         // Enable debug output
};

// Performance metrics
struct AIPerformanceMetrics {
    float inferenceTimeMs;   // Average inference time in milliseconds
    uint32_t modelSizeBytes; // Model size in bytes
    uint32_t memoryUsageKb;  // Memory usage in kilobytes
    uint32_t framesPerSecond; // Average frames processed per second
    float predictionAccuracy; // Prediction accuracy (0-1)
    uint32_t reserved[4];    // Reserved for future use
};

// Function declarations (ensure C/C++ compatibility)
#ifdef __cplusplus
extern "C" {
#endif

// Core AI functions
bool AI_ApplyActions(const struct AIActions* actions);
bool AI_GetModelInfo(struct AIModelInfo* info);
bool CoreML_FindDefaultModels(void);
bool CoreML_GetModelInfo(struct AIModelInfo* info);
bool CoreML_Predict(const void* frameData, struct AIActions* actions);

#ifdef __cplusplus
}
#endif

#endif /* _AI_STUB_TYPES_H_ */ 