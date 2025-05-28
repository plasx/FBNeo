#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <array>
#include <unordered_map>
#include "ai_input_frame.h"
#include "ai_output_action.h"

// Define MAX_PLAYERS if not defined
#ifndef MAX_PLAYERS
#define MAX_PLAYERS 4
#endif

namespace fbneo {
namespace ai {

// FBNeo AI Integration Definitions
    
// AI Feature Flags
#define FBNEO_AI_FEATURE_PLAYER_ASSIST      0x0001  // AI assists player gameplay
#define FBNEO_AI_FEATURE_CPU_ENHANCEMENT    0x0002  // AI enhances CPU opponents
#define FBNEO_AI_FEATURE_SELF_PLAY          0x0004  // AI can play against itself
#define FBNEO_AI_FEATURE_TRAINING           0x0008  // AI can be trained
#define FBNEO_AI_FEATURE_UPSCALING          0x0010  // AI for graphical upscaling
#define FBNEO_AI_FEATURE_PREDICTION         0x0020  // AI predicts game state
#define FBNEO_AI_FEATURE_ANALYTICS          0x0040  // AI analyzes gameplay
#define FBNEO_AI_FEATURE_CONTENT_GEN        0x0080  // AI generates game content

// AI Model Types
typedef enum {
    FBNEO_AI_MODEL_TYPE_UNKNOWN = 0,
    FBNEO_AI_MODEL_TYPE_COREML = 1,
    FBNEO_AI_MODEL_TYPE_PYTORCH = 2,
    FBNEO_AI_MODEL_TYPE_ONNX = 3,
    FBNEO_AI_MODEL_TYPE_TENSORFLOW_LITE = 4,
    FBNEO_AI_MODEL_TYPE_METAL_GRAPH = 5,
    FBNEO_AI_MODEL_TYPE_CUSTOM = 99
} FBNeoAIModelType;

// AI Compute Units
typedef enum {
    FBNEO_AI_COMPUTE_CPU_ONLY = 0,
    FBNEO_AI_COMPUTE_GPU_ONLY = 1,
    FBNEO_AI_COMPUTE_ANE_ONLY = 2,  // Apple Neural Engine only
    FBNEO_AI_COMPUTE_CPU_GPU = 3,   // CPU + GPU
    FBNEO_AI_COMPUTE_CPU_ANE = 4,   // CPU + ANE
    FBNEO_AI_COMPUTE_GPU_ANE = 5,   // GPU + ANE
    FBNEO_AI_COMPUTE_ALL = 6        // CPU + GPU + ANE
} FBNeoAIComputeUnits;

// AI Precision Mode
typedef enum {
    FBNEO_AI_PRECISION_FP32 = 0,    // 32-bit floating point
    FBNEO_AI_PRECISION_FP16 = 1,    // 16-bit floating point
    FBNEO_AI_PRECISION_INT8 = 2,    // 8-bit integer quantization
    FBNEO_AI_PRECISION_INT4 = 3,    // 4-bit integer quantization
    FBNEO_AI_PRECISION_MIXED = 4    // Mixed precision (auto-select)
} FBNeoAIPrecision;

// AI Game Genres - helps select appropriate models
typedef enum {
    FBNEO_AI_GENRE_FIGHTING = 0,
    FBNEO_AI_GENRE_SHMUP = 1,       // Shoot 'em up
    FBNEO_AI_GENRE_PLATFORMER = 2,
    FBNEO_AI_GENRE_PUZZLE = 3,
    FBNEO_AI_GENRE_RACING = 4,
    FBNEO_AI_GENRE_SPORTS = 5,
    FBNEO_AI_GENRE_BEAT_EM_UP = 6,
    FBNEO_AI_GENRE_RUN_AND_GUN = 7,
    FBNEO_AI_GENRE_OTHER = 99
} FBNeoAIGameGenre;

// AI Operational Mode
typedef enum {
    FBNEO_AI_MODE_DISABLED = 0,     // AI features disabled
    FBNEO_AI_MODE_ANALYSIS = 1,     // Analyze gameplay but don't interfere
    FBNEO_AI_MODE_ASSIST = 2,       // Provide gameplay assistance
    FBNEO_AI_MODE_OPPONENT = 3,     // Act as CPU opponent
    FBNEO_AI_MODE_PLAYER = 4,       // Play the game fully
    FBNEO_AI_MODE_TRAINING = 5,     // Training mode for AI improvement
    FBNEO_AI_MODE_DEMO = 6          // AI demonstration mode
} FBNeoAIMode;

// AI Player Assistance Level
typedef enum {
    FBNEO_AI_ASSIST_NONE = 0,       // No assistance
    FBNEO_AI_ASSIST_HINTS = 1,      // Provide hints/suggestions
    FBNEO_AI_ASSIST_REACTIVE = 2,   // Help with reactions (blocking, dodging)
    FBNEO_AI_ASSIST_COMBOS = 3,     // Help with combo execution
    FBNEO_AI_ASSIST_PARTIAL = 4,    // Take partial control
    FBNEO_AI_ASSIST_FULL = 5        // Take full control when needed
} FBNeoAIAssistLevel;

// AI Difficulty Level (for CPU opponents)
typedef enum {
    FBNEO_AI_DIFFICULTY_BEGINNER = 0,
    FBNEO_AI_DIFFICULTY_EASY = 1,
    FBNEO_AI_DIFFICULTY_MEDIUM = 2,
    FBNEO_AI_DIFFICULTY_HARD = 3,
    FBNEO_AI_DIFFICULTY_EXPERT = 4,
    FBNEO_AI_DIFFICULTY_DYNAMIC = 5  // Dynamically adjusts to player skill
} FBNeoAIDifficulty;

// AI Status Codes
typedef enum {
    FBNEO_AI_STATUS_SUCCESS = 0,
    FBNEO_AI_STATUS_ERROR_INIT = -1,
    FBNEO_AI_STATUS_ERROR_MODEL = -2,
    FBNEO_AI_STATUS_ERROR_COMPUTE = -3,
    FBNEO_AI_STATUS_ERROR_MEMORY = -4,
    FBNEO_AI_STATUS_ERROR_IO = -5,
    FBNEO_AI_STATUS_ERROR_PARAMETER = -6,
    FBNEO_AI_STATUS_ERROR_UNSUPPORTED = -7,
    FBNEO_AI_STATUS_ERROR_UNKNOWN = -99
} FBNeoAIStatus;

// AI Error Codes (for C++ API)
typedef enum {
    AIE_SUCCESS = 0,
    AIE_UNKNOWN_ERROR = -1,
    AIE_NOT_INITIALIZED = -2,
    AIE_ALREADY_INITIALIZED = -3,
    AIE_INVALID_PARAMETER = -4,
    AIE_FILE_NOT_FOUND = -5,
    AIE_UNSUPPORTED_FEATURE = -6,
    AIE_OUT_OF_MEMORY = -7,
    AIE_MODEL_LOAD_FAILED = -8,
    AIE_MODEL_NOT_LOADED = -9,
    AIE_INFERENCE_FAILED = -10,
    AIE_TRAINING_FAILED = -11,
    AIE_RESOURCE_EXHAUSTED = -12,
    AIE_METAL_INIT_FAILED = -13,
    AIE_INCOMPATIBLE_GAME = -14,
    AIE_COREML_ERROR = -15,
    AIE_PYTORCH_ERROR = -16,
    AIE_INVALID_FORMAT = -17,
    AIE_PERMISSION_DENIED = -18,
    AIE_TIMEOUT = -19,
    AIE_NOT_READY = -20
} AIError;

// AI Configuration Structure
typedef struct {
    FBNeoAIModelType modelType;     // Type of AI model
    FBNeoAIComputeUnits computeUnits; // Compute units to use
    FBNeoAIPrecision precision;     // Precision mode
    FBNeoAIMode operationMode;      // AI operation mode
    FBNeoAIAssistLevel assistLevel; // Player assistance level
    FBNeoAIDifficulty difficulty;   // AI difficulty level
    
    // Privacy settings
    int enableDifferentialPrivacy;  // 1 = enabled, 0 = disabled
    float privacyNoiseScale;        // 0.0 to 1.0, higher = more privacy
    
    // Performance settings
    int batchSize;                  // Batch size for inference
    int lowLatencyMode;             // 1 = optimize for latency, 0 = optimize for throughput
    int powerSavingMode;            // 1 = enable power saving, 0 = maximum performance
    
    // Game-specific settings
    FBNeoAIGameGenre gameGenre;     // Game genre for model selection
    const char* gameDriverName;     // FBNeo driver name
    int useGameSpecificModel;       // 1 = use game-specific model if available
    
    // Miscellaneous settings
    int enableLogging;              // 1 = enable AI logging
    int enableMetrics;              // 1 = collect performance metrics
    int enableAIVisualizations;     // 1 = show AI activity visualizations
    const char* modelPath;          // Custom model path (or NULL for default)
} FBNeoAIConfig;

// AI Input Frame Structure - game state information passed to AI
typedef struct {
    // Raw screen data
    unsigned char* screenData;      // Raw screen pixel data (RGBA format)
    int screenWidth;                // Screen width in pixels
    int screenHeight;               // Screen height in pixels
    int screenPitch;                // Screen pitch (bytes per row)
    
    // Game state information
    int playerHealth;               // Current player health
    int opponentHealth;             // Current opponent health
    int playerX;                    // Player X position
    int playerY;                    // Player Y position
    int opponentX;                  // Opponent X position
    int opponentY;                  // Opponent Y position
    int gameStage;                  // Current game stage/level
    int gameScore;                  // Current game score
    int frameNumber;                // Current frame number
    
    // Game-specific state (used by game-specific models)
    unsigned char gameState[256];   // Game-specific state data
    
    // Timing information
    unsigned long long timestamp;   // Timestamp in microseconds
} FBNeoAIInputFrame;

// AI Output Action Structure - actions to take based on AI decision
typedef struct {
    // Button state (1 = pressed, 0 = released)
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
    
    // Confidence levels (0.0 to 1.0) for each action
    float confidenceUp;
    float confidenceDown;
    float confidenceLeft;
    float confidenceRight;
    float confidence1;
    float confidence2;
    float confidence3;
    float confidence4;
    float confidence5;
    float confidence6;
    float confidenceStart;
    float confidenceCoin;
    
    // Action metadata
    int actionType;                 // Type of action (game-specific)
    int actionPriority;             // Priority of action (higher = more important)
    int actionDuration;             // Suggested duration in frames
    
    // Timing information
    unsigned long long timestamp;   // Timestamp in microseconds
    int latencyMicroseconds;        // Processing latency in microseconds
} FBNeoAIOutputAction;

// AI Model Information Structure
typedef struct {
    char modelName[256];            // Model name
    char modelVersion[64];          // Model version
    char modelAuthor[128];          // Model author
    char modelDescription[512];     // Model description
    char modelLicense[256];         // Model license
    
    FBNeoAIModelType modelType;     // Model type
    int modelSizeBytes;             // Model size in bytes
    
    // Model capabilities
    unsigned int supportedFeatures; // Bitfield of FBNEO_AI_FEATURE_* flags
    FBNeoAIGameGenre supportedGenres; // Supported game genres
    int minBatchSize;               // Minimum batch size
    int maxBatchSize;               // Maximum batch size
    
    // Model compatibility
    int requiresNeuralEngine;       // 1 = requires Neural Engine
    int requiresGPU;                // 1 = requires GPU
    int supportsCPUOnly;            // 1 = can run on CPU only
    
    // Model performance
    int averageLatencyMicroseconds; // Average inference latency
    int peakMemoryUsageBytes;       // Peak memory usage
} FBNeoAIModelInfo;

// AI System Information Structure
typedef struct {
    int isNeuralEngineAvailable;    // 1 = Neural Engine is available
    int neuralEngineCores;          // Number of Neural Engine cores (0 if not available)
    int metalFeatureSet;            // Metal feature set version
    int maxComputeUnits;            // Maximum compute units available
    char deviceName[128];           // Device name
    char osVersion[64];             // OS version
    int memoryBudgetBytes;          // Memory budget for AI in bytes
    
    // Performance metrics
    float averageCPUUsage;          // Average CPU usage (0.0 to 1.0)
    float averageGPUUsage;          // Average GPU usage (0.0 to 1.0)
    float averageANEUsage;          // Average ANE usage (0.0 to 1.0)
    int averageLatencyMicroseconds; // Average inference latency
    
    // Model information
    int modelCount;                 // Number of available models
    FBNeoAIModelInfo currentModel;  // Information about the current model
} FBNeoAISystemInfo;

// Simplified C-compatible interface for FBNeo

// AI Player Control
typedef enum {
    AI_PLAYER_NONE = 0,
    AI_PLAYER_1 = 1,
    AI_PLAYER_2 = 2,
    AI_PLAYER_BOTH = 3
} AIPlayerControl;

// AI Difficulty Levels
typedef enum {
    AI_DIFFICULTY_BEGINNER = 0,
    AI_DIFFICULTY_EASY = 1,
    AI_DIFFICULTY_EASY_MEDIUM = 2,
    AI_DIFFICULTY_MEDIUM_LOW = 3,
    AI_DIFFICULTY_MEDIUM = 5,
    AI_DIFFICULTY_MEDIUM_HIGH = 7,
    AI_DIFFICULTY_HARD = 8,
    AI_DIFFICULTY_EXPERT = 10
} AIDifficultyLevel;

// Simplified input frame for C interface
typedef struct {
    unsigned int frameNumber;
    int player;
    int buttons;
    int joystick;
    int specialMove;
} C_AIInputFrame;

// Simplified output action for C interface
typedef struct {
    int player;
    int button_press;
    int button_release;
    int joystick;
    float confidence;
} C_AIOutputAction;

// AI Settings structure for C interface
typedef struct {
    int enabled;
    int controlledPlayer;
    int difficulty;
    int trainingMode;
    int debugOverlay;
    char modelPath[1024];
} AISettings;

// AI Debug settings for C interface
typedef struct {
    int showHitboxes;
    int showFrameData;
    int showInputDisplay;
    int showGameState;
} AIDebugSettings;

// Helper types for the C++ API

enum AIAlgorithmType {
    ALGORITHM_PPO,    // Proximal Policy Optimization
    ALGORITHM_A3C,    // Asynchronous Advantage Actor-Critic
    ALGORITHM_DQN,    // Deep Q-Network
    ALGORITHM_RAINBOW // Rainbow DQN
};

enum GameType {
    GAME_UNKNOWN,
    GAME_FIGHTING,
    GAME_PLATFORMER,
    GAME_PUZZLE,
    GAME_SHOOTER,
    GAME_RACING,
    GAME_SPORTS
};

enum PolicyArchitecture {
    ARCHITECTURE_CNN,         // Convolutional Neural Network
    ARCHITECTURE_MLP,         // Multi-Layer Perceptron
    ARCHITECTURE_LSTM,        // Long Short-Term Memory
    ARCHITECTURE_TRANSFORMER  // Transformer
};

enum EngineType {
    ENGINE_NONE,      // No engine
    ENGINE_LIBTORCH,  // LibTorch (PyTorch C++ API)
    ENGINE_COREML,    // Apple CoreML
    ENGINE_MPS        // Metal Performance Shaders
};

// String conversion functions for enum types
std::string algorithmTypeToString(AIAlgorithmType type) {
    switch (type) {
        case ALGORITHM_PPO: return "PPO";
        case ALGORITHM_A3C: return "A3C";
        case ALGORITHM_DQN: return "DQN";
        case ALGORITHM_RAINBOW: return "Rainbow";
        default: return "Unknown";
    }
}

std::string gameTypeToString(GameType type) {
    switch (type) {
        case GAME_FIGHTING: return "Fighting";
        case GAME_PLATFORMER: return "Platformer";
        case GAME_PUZZLE: return "Puzzle";
        case GAME_SHOOTER: return "Shooter";
        case GAME_RACING: return "Racing";
        case GAME_SPORTS: return "Sports";
        default: return "Unknown";
    }
}

std::string architectureToString(PolicyArchitecture arch) {
    switch (arch) {
        case ARCHITECTURE_CNN: return "CNN";
        case ARCHITECTURE_MLP: return "MLP";
        case ARCHITECTURE_LSTM: return "LSTM";
        case ARCHITECTURE_TRANSFORMER: return "Transformer";
        default: return "Unknown";
    }
}

std::string engineTypeToString(EngineType engine) {
    switch (engine) {
        case ENGINE_LIBTORCH: return "LibTorch";
        case ENGINE_COREML: return "CoreML";
        case ENGINE_MPS: return "MPS";
        default: return "None";
    }
}

// Game state representation
struct PlayerState {
    int health;                  // Current health
    int maxHealth;               // Maximum health
    int positionX;               // X position on screen
    int positionY;               // Y position on screen
    int facing;                  // Direction facing (1 = right, -1 = left)
    int state;                   // Current animation state
    int stateTimer;              // Frames remaining in current state
    int attackPower;             // Current attack power
    int comboCounter;            // Current combo count
    char stateName[32];          // Name of current state
    char characterName[32];      // Character name
    
    // Hit boxes
    struct Box {
        int x, y, width, height;
    };
    std::vector<Box> collisionBoxes;    // Collision detection boxes
    std::vector<Box> attackBoxes;       // Attack hit boxes
    std::vector<Box> vulnerableBoxes;   // Vulnerable hit boxes
    
    // Reset to defaults
    void Reset() {
        health = 100;
        maxHealth = 100;
        positionX = 0;
        positionY = 0;
        facing = 1;
        state = 0;
        stateTimer = 0;
        attackPower = 0;
        comboCounter = 0;
        strcpy(stateName, "standing");
        strcpy(characterName, "unknown");
        collisionBoxes.clear();
        attackBoxes.clear();
        vulnerableBoxes.clear();
    }
};

struct GameState {
    int playerCount;                             // Number of players
    std::array<PlayerState, MAX_PLAYERS> players;  // Player states
    int timeRemaining;                           // Time left in current round
    int currentRound;                            // Current round number
    int maxRounds;                               // Max rounds in match
    int stage;                                   // Current stage ID
    char stageName[32];                          // Stage name
    
    // Game-specific variables
    std::unordered_map<std::string, int> gameVars;
    
    // Reset to defaults
    void Reset() {
        playerCount = 2;
        for (auto& player : players) {
            player.Reset();
        }
        timeRemaining = 99;
        currentRound = 1;
        maxRounds = 3;
        stage = 0;
        strcpy(stageName, "unknown");
        gameVars.clear();
    }
};

// Memory mapping for game variables
struct MemoryMapping {
    std::string name;        // Name of variable
    uint32_t address;        // Memory address
    uint32_t size;           // Size in bytes
    std::string type;        // Data type (int8, int16, int32, float, etc)
    float scale;             // Scaling factor
    float offset;            // Offset value
    std::string description; // Description of what this memory location contains
};

// Model information for C interface
typedef struct {
    char name[64];
    char version[16];
    int input_width;
    int input_height;
    int input_channels;
    int action_count;
    FBNeoAIModelType model_type;
    FBNeoAIComputeUnits compute_backend;
    FBNeoAIPrecision precision;
    uint32_t features;
    int inference_time_ms;
    uint32_t memory_usage_kb;
    char game_id[16];
    uint32_t game_genre;
    uint32_t reserved[4];  // For future expansion
} AIModelInfo;

// Game state for C interface
typedef struct {
    int player1_health;
    int player2_health;
    int player1_x;
    int player1_y;
    int player2_x;
    int player2_y;
    int timer;
    int score;
    int combo_counter;
    int stage;
    int game_state;  // 0=menu, 1=playing, 2=game over, etc.
    uint32_t reserved[12];  // For future expansion
} GameStateSimple;

// Memory mapping for C interface
typedef struct {
    const char* game_id;
    struct {
        const char* name;
        uint32_t address;
        uint32_t size;
        uint32_t type;  // 0=byte, 1=word, 2=dword, 3=float
    } variables[32];
    int variable_count;
} AIMemoryMapping;

// C function declarations

#ifdef __cplusplus
extern "C" {
#endif

// Core AI Module Functions
void AI_Initialize();
void AI_Shutdown();
void AI_LoadModel(const char* modelPath);
C_AIOutputAction AI_ProcessFrame(void* gameState, int frameNumber);
void AI_SetControlledPlayer(int playerIndex);
void AI_SetDifficulty(int level);
void AI_EnableTrainingMode(int enable);
void AI_EnableDebugOverlay(int enable);
void AI_SaveFrameData(const char* filename);

// CoreML Functions
bool CoreML_Initialize();
void CoreML_Shutdown();
bool CoreML_LoadModel(const char* path);
bool CoreML_GetModelInfo(AIModelInfo* info);
bool CoreML_ProcessFrame(const void* frameData, int width, int height, int pitch, float* results, int resultSize);
bool CoreML_RenderVisualization(void* overlayData, int width, int height, int pitch, int visualizationType);

#ifdef __cplusplus
}
#endif

} // namespace ai
} // namespace fbneo 