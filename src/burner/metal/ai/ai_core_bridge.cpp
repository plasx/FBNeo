#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>
#include <string>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <functional>
#include <stdexcept>

#include "ai_definitions.h"
#include "ai_torch_policy.h"
#include "metal_ai_module.h"
#include "ai_input_frame.h"
#include "ai_output_action.h"
#include "ai_rl_integration.h"
#include "game_memory_mapping.h"

// Forward declare Metal-specific functions (implemented in Objective-C++)
namespace Metal {
    bool InitializeAI();
    void ShutdownAI();
    bool ProcessFrameWithAI(const void* frameData, int width, int height, int pitch);
    bool RenderAIOverlay(void* overlayData, int width, int height, int pitch);
    bool SetAIVisualizationMode(int mode);
    bool LoadAIModel(const char* path);
    bool GetAIModelInfo(AIModelInfo* info);
    float GetAICurrentActionConfidence();
    float GetAIStateValue();
    int GetAITopActionCount();
    void GetAITopActionInfo(int index, char* actionName, float* confidence);
}

// Define the error codes for AI operations
enum AIErrorCode {
    AIE_NONE = 0,
    AIE_INVALID_PARAMETER,
    AIE_ALREADY_INITIALIZED,
    AIE_NOT_INITIALIZED,
    AIE_METAL_INIT_FAILED,
    AIE_MEMORY_ALLOCATION,
    AIE_MODEL_LOAD_FAILED,
    AIE_PYTORCH_ERROR,
    AIE_COREML_ERROR,
    AIE_INTERNAL_ERROR,
    AIE_GAME_NOT_SUPPORTED,
    AIE_FRAME_BUFFER_ERROR,
    AIE_FILE_NOT_FOUND,
    AIE_OUT_OF_BOUNDS,
    AIE_NOT_IMPLEMENTED,
    AIE_MAX
};

// Global AI system state
static bool g_initialized = false;
static bool g_active = false;
static bool g_training = false;
static bool g_debug = false;
static int g_errorCode = AIE_NONE;
static std::string g_errorDescription;
static std::mutex g_stateMutex;
static std::unique_ptr<AITorchPolicy> g_policy;
static std::unique_ptr<fbneo::ai::GameMemoryMapping> g_memoryMapper;
static AIInputFrame g_currentFrame;
static AIOutputAction g_currentAction;
static int g_aiControlledPlayer = 0;
static int g_aiDifficulty = 5;
static std::string g_currentGame;
static int g_frameCount = 0;
static bool g_sessionActive = false;
static float g_sessionReward = 0.0f;
static std::string g_modelPath;

// Error code to description mapping
static std::unordered_map<int, std::string> g_errorDescriptions;

// Initialize error descriptions
static void initErrorDescriptions() {
    if (!g_errorDescriptions.empty()) {
        return;
    }
    
    g_errorDescriptions[AIE_NONE] = "No error";
    g_errorDescriptions[AIE_INVALID_PARAMETER] = "Invalid parameter";
    g_errorDescriptions[AIE_ALREADY_INITIALIZED] = "AI system already initialized";
    g_errorDescriptions[AIE_NOT_INITIALIZED] = "AI system not initialized";
    g_errorDescriptions[AIE_METAL_INIT_FAILED] = "Metal AI initialization failed";
    g_errorDescriptions[AIE_MEMORY_ALLOCATION] = "Memory allocation failed";
    g_errorDescriptions[AIE_MODEL_LOAD_FAILED] = "Model loading failed";
    g_errorDescriptions[AIE_PYTORCH_ERROR] = "PyTorch error";
    g_errorDescriptions[AIE_COREML_ERROR] = "CoreML error";
    g_errorDescriptions[AIE_INTERNAL_ERROR] = "Internal error";
    g_errorDescriptions[AIE_GAME_NOT_SUPPORTED] = "Game not supported";
    g_errorDescriptions[AIE_FRAME_BUFFER_ERROR] = "Frame buffer error";
    g_errorDescriptions[AIE_FILE_NOT_FOUND] = "File not found";
    g_errorDescriptions[AIE_OUT_OF_BOUNDS] = "Index out of bounds";
    g_errorDescriptions[AIE_NOT_IMPLEMENTED] = "Not implemented";
}

// Set error code and description
static void setError(int code, const std::string& description = "") {
    g_errorCode = code;
    
    if (!description.empty()) {
        g_errorDescription = description;
    } else {
        auto it = g_errorDescriptions.find(code);
        if (it != g_errorDescriptions.end()) {
            g_errorDescription = it->second;
        } else {
            g_errorDescription = "Unknown error";
        }
    }
}

// Reset error
static void resetError() {
    g_errorCode = AIE_NONE;
    g_errorDescription = "";
}

// Get current error code
int AI_GetErrorCode() {
    return g_errorCode;
}

// Get current error description
const char* AI_GetErrorDescription() {
    return g_errorDescription.c_str();
}

// Check if a game is supported by the AI system
bool isGameSupported(const std::string& gameId) {
    // This would be extended with a more comprehensive database
    static const std::vector<std::string> supportedGames = {
        "mvsc", "sfa3", "xmvsf", "ssf2t", "vsav", "sfiii3", "kof98", "mshvsf", "dstlk"
    };
    
    for (const auto& id : supportedGames) {
        if (id == gameId) {
            return true;
        }
    }
    
    return false;
}

// Create a memory mapping for the current game
bool createMemoryMapping(const std::string& gameId) {
    if (!g_memoryMapper) {
        g_memoryMapper = std::make_unique<fbneo::ai::GameMemoryMapping>();
    }
    
    return g_memoryMapper->initialize(gameId);
}

// C API Implementation
extern "C" {
    // Initialize the AI system
    int AI_Init(const char* configPath) {
        std::lock_guard<std::mutex> lock(g_stateMutex);
        
        // Initialize error descriptions on first call
        initErrorDescriptions();
        
        if (g_initialized) {
            setError(AIE_ALREADY_INITIALIZED);
            return 1; // Already initialized (but not an error for client code)
        }
        
        resetError();
        printf("AI_Init: Initializing AI system\n");
        
        try {
            // Check parameters
            if (!configPath || strlen(configPath) == 0) {
                setError(AIE_INVALID_PARAMETER, "Invalid config path provided");
                return 0;
            }
            
            // Initialize Metal AI integration
            if (!Metal::InitializeAI()) {
                setError(AIE_METAL_INIT_FAILED);
                return 0;
            }
            
            // Create policy model with error handling
            try {
                g_policy = std::make_unique<AITorchPolicy>();
                if (!g_policy) {
                    setError(AIE_MEMORY_ALLOCATION, "Failed to allocate policy model");
                    Metal::ShutdownAI();
                    return 0;
                }
                
                if (!g_policy->initialize()) {
                    setError(AIE_MODEL_LOAD_FAILED, "Failed to initialize policy model");
                    Metal::ShutdownAI();
                    return 0;
                }
            } catch (const std::runtime_error& e) {
                setError(AIE_PYTORCH_ERROR, std::string("Policy initialization error: ") + e.what());
                Metal::ShutdownAI();
                return 0;
            }
            
            // Initialize game memory mapping
            g_memoryMapper = std::make_unique<fbneo::ai::GameMemoryMapping>();
            if (!g_memoryMapper) {
                setError(AIE_MEMORY_ALLOCATION, "Failed to allocate memory mapper");
                Metal::ShutdownAI();
                return 0;
            }
            
            // Initialize input frame buffer
            g_currentFrame.frameBuffer = nullptr;
            g_currentFrame.width = 0;
            g_currentFrame.height = 0;
            g_currentFrame.pitch = 0;
            g_currentFrame.format = AI_PIXEL_FORMAT_RGBA;
            
            // Initialize action buffer
            g_currentAction.clear();
            
            // Set default parameters
            g_aiControlledPlayer = 0;  // Player 1
            g_aiDifficulty = 5;       // Medium difficulty
            g_active = false;
            g_training = false;
            g_debug = false;
            g_frameCount = 0;
            g_sessionActive = false;
            g_sessionReward = 0.0f;
            
            // Set initialized flag
            g_initialized = true;
            
            printf("AI_Init: Successfully initialized AI system\n");
            return 1;
        } catch (const std::exception& e) {
            setError(AIE_INTERNAL_ERROR, std::string("Initialization error: ") + e.what());
            g_initialized = false;
            return 0;
        }
    }
    
    // Shutdown the AI system
    void AI_Exit() {
        std::lock_guard<std::mutex> lock(g_stateMutex);
        
        if (!g_initialized) {
            return;
        }
        
        printf("AI_Exit: Shutting down AI system\n");
        
        // Shutdown Metal AI integration
        Metal::ShutdownAI();
        
        // Reset policy model
        g_policy.reset();
        
        // Reset memory mapper
        g_memoryMapper.reset();
        
        // Reset state
        g_initialized = false;
        g_active = false;
        g_training = false;
        g_currentGame = "";
        g_frameCount = 0;
        g_sessionActive = false;
        g_sessionReward = 0.0f;
        
        resetError();
    }
    
    // Check if AI system is initialized
    bool AI_IsInitialized() {
        return g_initialized;
    }
    
    // Set active state
    void AI_SetActive(int enable) {
        std::lock_guard<std::mutex> lock(g_stateMutex);
        
        if (!g_initialized) {
            return;
        }
        
        g_active = (enable != 0);
        
        if (g_active) {
            printf("AI_SetActive: AI control enabled\n");
        } else {
            printf("AI_SetActive: AI control disabled\n");
        }
    }
    
    // Get active state
    int AI_IsActive() {
        return g_active ? 1 : 0;
    }
    
    // Set training mode
    void AI_SetTraining(int enable) {
        std::lock_guard<std::mutex> lock(g_stateMutex);
        
        if (!g_initialized) {
            return;
        }
        
        g_training = (enable != 0);
        
        if (g_training) {
            printf("AI_SetTraining: Training mode enabled\n");
        } else {
            printf("AI_SetTraining: Training mode disabled\n");
        }
    }
    
    // Get training mode
    int AI_IsTraining() {
        return g_training ? 1 : 0;
    }
    
    // Process a frame buffer from the emulator
    void AI_ProcessFrameBuffer(const void* data, int width, int height, int pitch) {
        std::lock_guard<std::mutex> lock(g_stateMutex);
        
        if (!g_initialized || !g_active || !data || width <= 0 || height <= 0 || pitch <= 0) {
            return;
        }
        
        // Update the frame counter
        g_frameCount++;
        
        // Configure input frame
        g_currentFrame.frameBuffer = data;
        g_currentFrame.width = width;
        g_currentFrame.height = height;
        g_currentFrame.pitch = pitch;
        
        // Get game state from memory
        if (g_memoryMapper && !g_currentGame.empty()) {
            g_memoryMapper->updateGameState();
            
            // Extract player states
            AIPlayerState player1, player2;
            g_memoryMapper->getPlayerState(0, player1);
            g_memoryMapper->getPlayerState(1, player2);
            
            // Update input frame with game state information
            g_currentFrame.players[0] = player1;
            g_currentFrame.players[1] = player2;
            g_currentFrame.frameNumber = g_frameCount;
        }
        
        // Process frame with AI
        bool success = Metal::ProcessFrameWithAI(data, width, height, pitch);
        
        if (success) {
            // Run AI policy to get action
            if (g_policy) {
                g_policy->predict(g_currentFrame, g_currentAction, !g_training);
            }
            
            // If we're in training mode, update with game state feedback
            if (g_training && g_memoryMapper) {
                float reward = g_memoryMapper->calculateReward(g_aiControlledPlayer);
                g_sessionReward += reward;
                
                // TODO: Implement training update logic here
            }
        }
    }
    
    // Start a training session
    void AI_StartSession() {
        std::lock_guard<std::mutex> lock(g_stateMutex);
        
        if (!g_initialized || !g_training) {
            return;
        }
        
        printf("AI_StartSession: Starting new training session\n");
        
        g_sessionActive = true;
        g_sessionReward = 0.0f;
        g_frameCount = 0;
    }
    
    // End a training session and return the total reward
    float AI_EndSession(int success) {
        std::lock_guard<std::mutex> lock(g_stateMutex);
        
        if (!g_initialized || !g_training || !g_sessionActive) {
            return 0.0f;
        }
        
        printf("AI_EndSession: Ending training session, success=%d, total reward=%.2f\n", 
              success, g_sessionReward);
        
        float finalReward = g_sessionReward;
        
        // Add final reward based on session success or failure
        if (success) {
            finalReward += 10.0f;
        }
        
        g_sessionActive = false;
        g_sessionReward = 0.0f;
        
        return finalReward;
    }
    
    // Save emulator state
    int AI_SaveState(const char* path) {
        if (!g_initialized) {
            setError(AIE_NOT_INITIALIZED);
            return 0;
        }
        
        if (!path || strlen(path) == 0) {
            setError(AIE_INVALID_PARAMETER);
            return 0;
        }
        
        printf("AI_SaveState: Saving state to %s\n", path);
        
        // TODO: Implement state saving logic here
        // This would need to hook into FBNeo state management
        
        return 1; // Success
    }
    
    // Load emulator state
    int AI_LoadState(const char* path) {
        if (!g_initialized) {
            setError(AIE_NOT_INITIALIZED);
            return 0;
        }
        
        if (!path || strlen(path) == 0) {
            setError(AIE_INVALID_PARAMETER);
            return 0;
        }
        
        printf("AI_LoadState: Loading state from %s\n", path);
        
        // TODO: Implement state loading logic here
        // This would need to hook into FBNeo state management
        
        return 1; // Success
    }
    
    // Set the current game
    int AI_SetGame(const char* gameId) {
        std::lock_guard<std::mutex> lock(g_stateMutex);
        
        if (!g_initialized) {
            setError(AIE_NOT_INITIALIZED);
            return 0;
        }
        
        if (!gameId || strlen(gameId) == 0) {
            setError(AIE_INVALID_PARAMETER);
            return 0;
        }
        
        // Check if game is supported
        std::string gameIdStr = gameId;
        if (!isGameSupported(gameIdStr)) {
            setError(AIE_GAME_NOT_SUPPORTED);
            return 0;
        }
        
        printf("AI_SetGame: Setting current game to %s\n", gameId);
        
        // Update current game and create memory mapping
        g_currentGame = gameIdStr;
        
        // Create memory mapping for this game
        if (!createMemoryMapping(gameIdStr)) {
            setError(AIE_GAME_NOT_SUPPORTED, "Failed to create memory mapping for game");
            return 0;
        }
        
        // Reset session state
        g_frameCount = 0;
        g_sessionActive = false;
        g_sessionReward = 0.0f;
        
        return 1; // Success
    }
    
    // Load an AI model
    int AI_LoadModel(const char* modelPath) {
        std::lock_guard<std::mutex> lock(g_stateMutex);
        
        if (!g_initialized) {
            setError(AIE_NOT_INITIALIZED);
            return 0;
        }
        
        if (!modelPath || strlen(modelPath) == 0) {
            setError(AIE_INVALID_PARAMETER);
            return 0;
        }
        
        printf("AI_LoadModel: Loading model from %s\n", modelPath);
        
        // Check if file exists
        FILE* file = fopen(modelPath, "rb");
        if (!file) {
            setError(AIE_FILE_NOT_FOUND);
            return 0;
        }
        fclose(file);
        
        // Try to load the model with Metal
        bool success = Metal::LoadAIModel(modelPath);
        if (!success) {
            setError(AIE_MODEL_LOAD_FAILED);
            return 0;
        }
        
        // Also load into our policy if it exists
        if (g_policy) {
            success = g_policy->load(modelPath);
            if (!success) {
                setError(AIE_MODEL_LOAD_FAILED, "Failed to load model into policy");
                return 0;
            }
        }
        
        // Store the model path
        g_modelPath = modelPath;
        
        return 1; // Success
    }
    
    // Set the AI-controlled player
    void AI_SetControlledPlayer(int playerIndex) {
        std::lock_guard<std::mutex> lock(g_stateMutex);
        
        if (!g_initialized) {
            return;
        }
        
        // Ensure valid player index (0 or 1)
        g_aiControlledPlayer = (playerIndex == 0 || playerIndex == 1) ? playerIndex : 0;
        
        printf("AI_SetControlledPlayer: Set AI-controlled player to P%d\n", g_aiControlledPlayer + 1);
    }
    
    // Set AI difficulty level
    void AI_SetDifficulty(int level) {
        std::lock_guard<std::mutex> lock(g_stateMutex);
        
        if (!g_initialized) {
            return;
        }
        
        // Clamp difficulty to range 1-10
        g_aiDifficulty = (level < 1) ? 1 : ((level > 10) ? 10 : level);
        
        printf("AI_SetDifficulty: Set AI difficulty to %d\n", g_aiDifficulty);
        
        // Update policy difficulty if available
        if (g_policy) {
            g_policy->SetDifficulty(g_aiDifficulty);
        }
    }
    
    // Enable/disable debug overlay
    void AI_EnableDebugOverlay(int enable) {
        std::lock_guard<std::mutex> lock(g_stateMutex);
        
        if (!g_initialized) {
            return;
        }
        
        g_debug = (enable != 0);
        
        // Configure Metal debug overlay
        Metal::SetAIVisualizationMode(g_debug ? 1 : 0);
        
        printf("AI_EnableDebugOverlay: Debug overlay %s\n", g_debug ? "enabled" : "disabled");
    }
    
    // Get AI prediction confidence
    float AI_GetConfidence() {
        if (!g_initialized || !g_active) {
            return 0.0f;
        }
        
        return Metal::GetAICurrentActionConfidence();
    }
    
    // Get current AI state value
    float AI_GetStateValue() {
        if (!g_initialized || !g_active) {
            return 0.0f;
        }
        
        return Metal::GetAIStateValue();
    }
    
    // Get number of top actions
    int AI_GetTopActionCount() {
        if (!g_initialized || !g_active) {
            return 0;
        }
        
        return Metal::GetAITopActionCount();
    }
    
    // Get info about a top action
    void AI_GetTopActionInfo(int index, char* actionName, float* confidence) {
        if (!g_initialized || !g_active || index < 0 || !actionName || !confidence) {
            return;
        }
        
        Metal::GetAITopActionInfo(index, actionName, confidence);
    }
    
    // Get the current AI action
    int AI_GetCurrentAction(AIOutputAction* action) {
        std::lock_guard<std::mutex> lock(g_stateMutex);
        
        if (!g_initialized || !g_active || !action) {
            return 0;
        }
        
        // Copy the current action
        *action = g_currentAction;
        
        return 1; // Success
    }
    
    // Get memory-mapped game state
    int AI_GetGameState(void* stateBuffer, int bufferSize) {
        std::lock_guard<std::mutex> lock(g_stateMutex);
        
        if (!g_initialized || !g_memoryMapper || !stateBuffer || bufferSize <= 0) {
            return 0;
        }
        
        // Copy game state to buffer
        size_t stateSize = g_memoryMapper->copyGameState(stateBuffer, bufferSize);
        
        return static_cast<int>(stateSize); // Return size of state data
    }
}

// C++ API implementation for integration with other C++ components
namespace fbneo {
namespace ai {

// Get the policy model
AITorchPolicy* GetPolicyModel() {
    return g_policy.get();
}

// Get the memory mapper
GameMemoryMapping* GetMemoryMapper() {
    return g_memoryMapper.get();
}

// Get the current frame
const AIInputFrame& GetCurrentFrame() {
    return g_currentFrame;
}

// Get the current action
const AIOutputAction& GetCurrentAction() {
    return g_currentAction;
}

// Check if AI is active
bool IsAIActive() {
    return g_active;
}

// Check if AI is in training mode
bool IsAITraining() {
    return g_training;
}

// Get the AI-controlled player index
int GetAIControlledPlayer() {
    return g_aiControlledPlayer;
}

// Get the AI difficulty level
int GetAIDifficulty() {
    return g_aiDifficulty;
}

// Get the current game ID
std::string GetCurrentGame() {
    return g_currentGame;
}

} // namespace ai
} // namespace fbneo 