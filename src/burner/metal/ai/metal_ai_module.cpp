#include "ai_rl_integration.h"
#include "ai_torch_policy.h"
#include "pytorch_to_coreml.h"
#include "../metal_declarations.h"
#include "../metal_intf.h"
#include <string>
#include <iostream>
#include <memory>
#include <fstream>
#include <chrono>
#include <unordered_map>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>
#include <mutex>
#include <thread>
#include <atomic>
#include <functional>

// Forward declarations
extern "C" {
    // AI core bridge functions
    extern int AI_Init(const char* configPath);
    extern void AI_Exit();
    extern void AI_SetActive(int enable);
    extern int AI_IsActive();
    extern void AI_SetTraining(int enable);
    extern int AI_IsTraining();
    extern void AI_ProcessFrameBuffer(const void* data, int width, int height, int pitch);
    extern void AI_StartSession();
    extern float AI_EndSession(int success);
    extern int AI_SaveState(const char* path);
    extern int AI_LoadState(const char* path);
    
    // FBNeo core functions
    extern INT32 BurnDrvGetIndex(char* szName);
    extern char* BurnDrvGetTextA(UINT32 i);
    extern INT32 BurnDrvGetVisibleSize(INT32* pnWidth, INT32* pnHeight);
}

namespace fbneo {
namespace metal {
namespace ai {

// Module state
static bool g_initialized = false;
static bool g_aiEnabled = false;
static bool g_trainingMode = false;
static std::string g_modelPath = "models/";
static std::string g_gameType = "unknown";
static std::string g_gameName = "";
static std::chrono::time_point<std::chrono::steady_clock> g_frameTime;

// Forward declarations for Objective-C++ functionality
extern "C" {
    bool CoreML_Initialize();
    void CoreML_Shutdown();
    bool CoreML_LoadModel(const char* path);
    bool CoreML_GetModelInfo(AIModelInfo* info);
    bool CoreML_ProcessFrame(const void* frameData, int width, int height, int pitch, float* results, int resultSize);
    bool CoreML_RenderVisualization(void* overlayData, int width, int height, int pitch, int visualizationType);
}

// MetalAIModule implementation
namespace {
    // Module state
    std::mutex g_moduleMutex;
    bool g_modelLoaded = false;
    std::atomic<bool> g_processingActive{false};
    std::atomic<bool> g_visualizationActive{false};
    
    // Processing thread
    std::thread g_processingThread;
    std::atomic<bool> g_threadRunning{false};
    
    // Current frame and results
    struct {
        std::vector<uint8_t> frameData;
        int width = 0;
        int height = 0;
        int pitch = 0;
        bool updated = false;
        std::mutex mutex;
    } g_currentFrame;
    
    // Processing results
    struct {
        std::vector<float> data;
        std::vector<std::string> actionNames;
        std::vector<float> actionConfidences;
        float stateValue = 0.0f;
        int topActionCount = 0;
        std::mutex mutex;
    } g_results;
    
    // Visualization settings
    struct {
        int type = 0;  // 0=none, 1=heatmap, 2=attention, 3=action probs
        float opacity = 0.5f;
    } g_visualization;
    
    // Current model info
    AIModelInfo g_modelInfo;
    
    // Frame processing thread function
    void ProcessingThreadFunc() {
        printf("MetalAIModule: Processing thread started\n");
        
        g_threadRunning = true;
        
        // Processing loop
        while (g_threadRunning) {
            // Check if we have a new frame and processing is active
            bool hasFrame = false;
            std::vector<uint8_t> frameCopy;
            int width = 0, height = 0, pitch = 0;
            
            {
                std::lock_guard<std::mutex> lock(g_currentFrame.mutex);
                if (g_currentFrame.updated && g_processingActive) {
                    // Make a copy of the frame data for processing
                    frameCopy = g_currentFrame.frameData;
                    width = g_currentFrame.width;
                    height = g_currentFrame.height;
                    pitch = g_currentFrame.pitch;
                    g_currentFrame.updated = false;
                    hasFrame = true;
                }
            }
            
            // Process the frame if we have one
            if (hasFrame) {
                // Prepare results buffer
                std::vector<float> results(256); // Assume max 256 result values
                
                // Process frame using CoreML
                bool success = CoreML_ProcessFrame(
                    frameCopy.data(), width, height, pitch,
                    results.data(), static_cast<int>(results.size())
                );
                
                if (success) {
                    // Update results
                    std::lock_guard<std::mutex> lock(g_results.mutex);
                    g_results.data = results;
                    
                    // Parse result values (this would depend on the specific model format)
                    // For this implementation, we'll assume:
                    // - First value is the state value
                    // - Next N values are action probabilities
                    g_results.stateValue = results[0];
                    
                    // Clear previous action data
                    g_results.actionConfidences.clear();
                    
                    // Parse action probabilities
                    int actionCount = g_modelInfo.action_count;
                    if (actionCount > 0 && actionCount < 50) { // Sanity check
                        for (int i = 0; i < actionCount && i + 1 < results.size(); i++) {
                            g_results.actionConfidences.push_back(results[i + 1]);
                        }
                        g_results.topActionCount = actionCount;
                    }
                }
            }
            
            // Avoid busy waiting
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        
        printf("MetalAIModule: Processing thread stopped\n");
    }
}

// MetalAIModule interface implementation
namespace MetalAIModule {
    bool Initialize() {
        std::lock_guard<std::mutex> lock(g_moduleMutex);
        
        if (g_initialized) {
            return true; // Already initialized
        }
        
        printf("MetalAIModule::Initialize: Initializing Metal AI module\n");
        
        // Initialize CoreML integration
        if (!CoreML_Initialize()) {
            printf("MetalAIModule::Initialize: Failed to initialize CoreML\n");
            return false;
        }
        
        // Set up default model info
        strcpy(g_modelInfo.name, "No Model Loaded");
        strcpy(g_modelInfo.version, "0.0.0");
        g_modelInfo.input_width = 224;
        g_modelInfo.input_height = 224;
        g_modelInfo.input_channels = 3;
        g_modelInfo.action_count = 0;
        
        // Initialize processing state
        g_processingActive = false;
        g_visualizationActive = false;
        g_modelLoaded = false;
        
        // Set up default visualization
        g_visualization.type = 0;
        g_visualization.opacity = 0.5f;
        
        // Initialize result data
        g_results.stateValue = 0.0f;
        g_results.topActionCount = 0;
        g_results.actionNames = {
            "UP", "DOWN", "LEFT", "RIGHT", 
            "BUTTON1", "BUTTON2", "BUTTON3", "BUTTON4", "BUTTON5", "BUTTON6"
        };
        g_results.actionConfidences.resize(g_results.actionNames.size(), 0.0f);
        
        // Start processing thread
        g_threadRunning = true;
        g_processingThread = std::thread(ProcessingThreadFunc);
        
        // Mark as initialized
        g_initialized = true;
        
        printf("MetalAIModule::Initialize: Metal AI module initialized successfully\n");
        return true;
    }
    
    void Shutdown() {
        std::lock_guard<std::mutex> lock(g_moduleMutex);
        
        if (!g_initialized) {
            return;
        }
        
        printf("MetalAIModule::Shutdown: Shutting down Metal AI module\n");
        
        // Stop processing thread
        g_threadRunning = false;
        if (g_processingThread.joinable()) {
            g_processingThread.join();
        }
        
        // Shutdown CoreML
        CoreML_Shutdown();
        
        // Reset state
        g_initialized = false;
        g_processingActive = false;
        g_visualizationActive = false;
        g_modelLoaded = false;
        
        printf("MetalAIModule::Shutdown: Metal AI module shutdown complete\n");
    }
    
    bool LoadModel(const char* path) {
        std::lock_guard<std::mutex> lock(g_moduleMutex);
        
        if (!g_initialized) {
            return false;
        }
        
        printf("MetalAIModule::LoadModel: Loading model from %s\n", path);
        
        // Load model using CoreML
        if (!CoreML_LoadModel(path)) {
            printf("MetalAIModule::LoadModel: Failed to load model\n");
            return false;
        }
        
        // Get model info
        if (!CoreML_GetModelInfo(&g_modelInfo)) {
            printf("MetalAIModule::LoadModel: Failed to get model info\n");
            return false;
        }
        
        // Update state
        g_modelLoaded = true;
        
        printf("MetalAIModule::LoadModel: Model loaded successfully\n");
        printf("  Name: %s\n", g_modelInfo.name);
        printf("  Version: %s\n", g_modelInfo.version);
        printf("  Input: %dx%dx%d\n", g_modelInfo.input_width, g_modelInfo.input_height, g_modelInfo.input_channels);
        printf("  Actions: %d\n", g_modelInfo.action_count);
        
        return true;
    }
    
    bool ProcessFrame(const void* frameData, int width, int height, int pitch) {
        if (!g_initialized || !frameData || width <= 0 || height <= 0 || pitch <= 0) {
            return false;
        }
        
        // Skip if processing is not active
        if (!g_processingActive) {
            return true;
        }
        
        // Update current frame
        {
            std::lock_guard<std::mutex> lock(g_currentFrame.mutex);
            
            // Resize frame buffer if needed
            size_t frameSize = height * pitch;
            if (g_currentFrame.frameData.size() < frameSize) {
                g_currentFrame.frameData.resize(frameSize);
            }
            
            // Copy frame data
            memcpy(g_currentFrame.frameData.data(), frameData, frameSize);
            g_currentFrame.width = width;
            g_currentFrame.height = height;
            g_currentFrame.pitch = pitch;
            g_currentFrame.updated = true;
        }
        
        return true;
    }
    
    bool RenderVisualization(void* overlayData, int width, int height, int pitch) {
        if (!g_initialized || !overlayData || width <= 0 || height <= 0 || pitch <= 0) {
            return false;
        }
        
        // Skip if visualization is not active or no visualization type is selected
        if (!g_visualizationActive || g_visualization.type == 0) {
            return true;
        }
        
        // Use CoreML to render visualization
        return CoreML_RenderVisualization(overlayData, width, height, pitch, g_visualization.type);
    }
    
    void SetActive(bool active) {
        if (!g_initialized) {
            return;
        }
        
        g_processingActive = active;
        printf("MetalAIModule::SetActive: Processing is now %s\n", active ? "active" : "inactive");
    }
    
    bool IsActive() {
        return g_initialized && g_processingActive;
    }
    
    void SetVisualizationActive(bool active) {
        if (!g_initialized) {
            return;
        }
        
        g_visualizationActive = active;
        printf("MetalAIModule::SetVisualizationActive: Visualization is now %s\n", 
               active ? "active" : "inactive");
    }
    
    void SetVisualizationType(int type) {
        if (!g_initialized) {
            return;
        }
        
        g_visualization.type = type;
        printf("MetalAIModule::SetVisualizationType: Visualization type set to %d\n", type);
    }
    
    void SetVisualizationOpacity(float opacity) {
        if (!g_initialized) {
            return;
        }
        
        // Clamp opacity to 0.0-1.0
        if (opacity < 0.0f) opacity = 0.0f;
        if (opacity > 1.0f) opacity = 1.0f;
        
        g_visualization.opacity = opacity;
    }
    
    bool GetModelInfo(AIModelInfo* info) {
        if (!g_initialized || !g_modelLoaded || !info) {
            return false;
        }
        
        // Copy model info
        memcpy(info, &g_modelInfo, sizeof(AIModelInfo));
        return true;
    }
    
    float GetCurrentActionConfidence() {
        if (!g_initialized || !g_modelLoaded) {
            return 0.0f;
        }
        
        // Get top action confidence
        std::lock_guard<std::mutex> lock(g_results.mutex);
        if (g_results.actionConfidences.empty()) {
            return 0.0f;
        }
        
        // Find max confidence
        float maxConfidence = 0.0f;
        for (float conf : g_results.actionConfidences) {
            if (conf > maxConfidence) {
                maxConfidence = conf;
            }
        }
        
        return maxConfidence;
    }
    
    float GetStateValue() {
        if (!g_initialized || !g_modelLoaded) {
            return 0.0f;
        }
        
        std::lock_guard<std::mutex> lock(g_results.mutex);
        return g_results.stateValue;
    }
    
    int GetTopActionCount() {
        if (!g_initialized || !g_modelLoaded) {
            return 0;
        }
        
        std::lock_guard<std::mutex> lock(g_results.mutex);
        return g_results.topActionCount;
    }
    
    void GetTopActionInfo(int index, char* actionName, float* confidence) {
        if (!g_initialized || !g_modelLoaded || !actionName || !confidence) {
            if (actionName) {
                actionName[0] = '\0';
            }
            if (confidence) {
                *confidence = 0.0f;
            }
            return;
        }
        
        std::lock_guard<std::mutex> lock(g_results.mutex);
        
        // Check index bounds
        if (index < 0 || index >= g_results.actionNames.size() || 
            index >= g_results.actionConfidences.size()) {
            actionName[0] = '\0';
            *confidence = 0.0f;
            return;
        }
        
        // Copy action name and confidence
        strcpy(actionName, g_results.actionNames[index].c_str());
        *confidence = g_results.actionConfidences[index];
    }
    
    AIOutputAction GetCurrentAction() {
        if (!g_initialized || !g_modelLoaded) {
            return AIOutputAction();
        }
        
        std::lock_guard<std::mutex> lock(g_results.mutex);
        
        // Create action from current results
        AIOutputAction action;
        action.setValue(g_results.stateValue);
        
        // Find top action
        int topActionIndex = -1;
        float topConfidence = 0.0f;
        
        for (size_t i = 0; i < g_results.actionConfidences.size(); i++) {
            if (g_results.actionConfidences[i] > topConfidence) {
                topConfidence = g_results.actionConfidences[i];
                topActionIndex = static_cast<int>(i);
            }
        }
        
        // Set action data
        if (topActionIndex >= 0) {
            action.setAction(topActionIndex);
            action.setConfidence(topConfidence);
            action.setName(g_results.actionNames[topActionIndex]);
        }
        
        return action;
    }
}

/**
 * @brief Initialize the AI module for Metal backend
 * @param configPath Path to configuration file (optional)
 * @return True if initialization was successful
 */
bool initialize(const char* configPath) {
    if (g_initialized) {
        std::cout << "AI module already initialized" << std::endl;
        return true;
    }
    
    std::cout << "Initializing Metal AI module..." << std::endl;
    
    // Initialize AI core
    if (!AI_Init(configPath)) {
        std::cerr << "Failed to initialize AI core" << std::endl;
        return false;
    }
    
    // Initialize PyTorch to CoreML conversion system
    fbneo::ai::initializePyTorchToCoreMLSystem();
    
    // Create model directory if it doesn't exist
    system("mkdir -p models");
    
    // Get current game information
    char gameName[256] = {0};
    strncpy(gameName, BurnDrvGetTextA(DRV_NAME), sizeof(gameName) - 1);
    g_gameName = gameName;
    
    // Determine game type based on name/genre
    if (strstr(BurnDrvGetTextA(DRV_FULLNAME), "Fighting") ||
        strstr(g_gameName.c_str(), "sf") || 
        strstr(g_gameName.c_str(), "kof") ||
        strstr(g_gameName.c_str(), "marvel") ||
        strstr(g_gameName.c_str(), "vs")) {
        g_gameType = "fighting";
    }
    else if (strstr(BurnDrvGetTextA(DRV_FULLNAME), "Platformer") ||
             strstr(g_gameName.c_str(), "mario") ||
             strstr(g_gameName.c_str(), "sonic")) {
        g_gameType = "platformer";
    }
    else if (strstr(BurnDrvGetTextA(DRV_FULLNAME), "Puzzle") ||
             strstr(g_gameName.c_str(), "puzzle") ||
             strstr(g_gameName.c_str(), "tetris")) {
        g_gameType = "puzzle";
    }
    else if (strstr(BurnDrvGetTextA(DRV_FULLNAME), "Shooter") ||
             strstr(g_gameName.c_str(), "shoot") ||
             strstr(g_gameName.c_str(), "gun")) {
        g_gameType = "shooter";
    }
    
    std::cout << "Game detected as type: " << g_gameType << std::endl;
    
    // Try to load model for this game if it exists
    std::string modelPath = g_modelPath + g_gameName + ".model";
    if (std::ifstream(modelPath)) {
        std::cout << "Loading model for " << g_gameName << " from " << modelPath << std::endl;
        if (AI_LoadState(modelPath.c_str())) {
            std::cout << "Model loaded successfully" << std::endl;
        } else {
            std::cerr << "Failed to load model" << std::endl;
        }
    } else {
        std::cout << "No pre-existing model found for " << g_gameName << std::endl;
    }
    
    g_initialized = true;
    g_frameTime = std::chrono::steady_clock::now();
    
    return true;
}

/**
 * @brief Shutdown the AI module
 */
void shutdown() {
    if (!g_initialized) {
        return;
    }
    
    std::cout << "Shutting down Metal AI module..." << std::endl;
    
    // Save model if one was created
    if (g_aiEnabled) {
        std::string modelPath = g_modelPath + g_gameName + ".model";
        std::cout << "Saving model to " << modelPath << std::endl;
        AI_SaveState(modelPath.c_str());
    }
    
    // Shutdown AI core
    AI_Exit();
    
    g_initialized = false;
    g_aiEnabled = false;
    g_trainingMode = false;
}

/**
 * @brief Enable or disable AI
 * @param enable Whether to enable AI
 */
void setEnabled(bool enable) {
    if (g_aiEnabled == enable) {
        return;
    }
    
    g_aiEnabled = enable;
    AI_SetActive(enable ? 1 : 0);
    
    std::cout << "AI " << (enable ? "enabled" : "disabled") << std::endl;
    
    // Start a new session if enabling
    if (enable) {
        AI_StartSession();
    } else {
        // End session if disabling
        AI_EndSession(0);
    }
}

/**
 * @brief Check if AI is enabled
 * @return True if AI is enabled
 */
bool isEnabled() {
    return g_aiEnabled;
}

/**
 * @brief Enable or disable training mode
 * @param enable Whether to enable training mode
 */
void setTrainingMode(bool enable) {
    if (g_trainingMode == enable) {
        return;
    }
    
    g_trainingMode = enable;
    AI_SetTraining(enable ? 1 : 0);
    
    std::cout << "AI training mode " << (enable ? "enabled" : "disabled") << std::endl;
}

/**
 * @brief Check if training mode is enabled
 * @return True if training mode is enabled
 */
bool isTrainingMode() {
    return g_trainingMode;
}

/**
 * @brief Process a frame from the Metal renderer
 * @param frameData Frame buffer data
 * @param width Frame width
 * @param height Frame height
 * @param pitch Frame pitch (bytes per row)
 */
void processFrame(const void* frameData, int width, int height, int pitch) {
    if (!g_initialized || !g_aiEnabled || !frameData) {
        return;
    }
    
    // Calculate frame time
    auto now = std::chrono::steady_clock::now();
    float deltaTime = std::chrono::duration<float>(now - g_frameTime).count();
    g_frameTime = now;
    
    // Process frame in AI core
    AI_ProcessFrameBuffer(frameData, width, height, pitch);
}

/**
 * @brief Save the current AI model
 * @param path Path to save to (optional, uses default if not provided)
 * @return True if successful
 */
bool saveModel(const char* path) {
    if (!g_initialized) {
        return false;
    }
    
    std::string savePath;
    if (path && strlen(path) > 0) {
        savePath = path;
    } else {
        savePath = g_modelPath + g_gameName + ".model";
    }
    
    std::cout << "Saving AI model to " << savePath << std::endl;
    return AI_SaveState(savePath.c_str()) != 0;
}

/**
 * @brief Load an AI model
 * @param path Path to load from (optional, uses default if not provided)
 * @return True if successful
 */
bool loadModel(const char* path) {
    if (!g_initialized) {
        return false;
    }
    
    std::string loadPath;
    if (path && strlen(path) > 0) {
        loadPath = path;
    } else {
        loadPath = g_modelPath + g_gameName + ".model";
    }
    
    std::cout << "Loading AI model from " << loadPath << std::endl;
    return AI_LoadState(loadPath.c_str()) != 0;
}

/**
 * @brief Start a training session
 */
void startTrainingSession() {
    if (!g_initialized || !g_aiEnabled) {
        return;
    }
    
    AI_StartSession();
    std::cout << "AI training session started" << std::endl;
}

/**
 * @brief End the current training session
 * @param success Whether the session was successful
 * @return The total reward for the session
 */
float endTrainingSession(bool success) {
    if (!g_initialized || !g_aiEnabled) {
        return 0.0f;
    }
    
    float reward = AI_EndSession(success ? 1 : 0);
    std::cout << "AI training session ended with " << (success ? "success" : "failure")
             << ", total reward: " << reward << std::endl;
    
    return reward;
}

/**
 * @brief Get the game type for the current game
 * @return Game type string
 */
const char* getGameType() {
    return g_gameType.c_str();
}

/**
 * @brief Export the current AI model to CoreML format
 * @param path Path to save to (optional, uses default if not provided)
 * @return True if successful
 */
bool exportToCoreML(const char* path) {
    if (!g_initialized) {
        return false;
    }
    
    std::string exportPath;
    if (path && strlen(path) > 0) {
        exportPath = path;
    } else {
        exportPath = g_modelPath + g_gameName + ".mlmodel";
    }
    
    // First save the AI model to a temporary PyTorch file
    std::string tempPath = g_modelPath + g_gameName + ".tmp.pt";
    if (!saveModel(tempPath.c_str())) {
        std::cerr << "Failed to save temporary PyTorch model" << std::endl;
        return false;
    }
    
    // Define input shape based on typical game frameSize
    INT32 width = 0, height = 0;
    BurnDrvGetVisibleSize(&width, &height);
    
    // Adjust for typical model input size
    width = 84;  // Common downsampled size
    height = 84; // Common downsampled size
    
    // Convert shape to array for CoreML conversion
    int inputShape[] = {1, 4, height, width}; // Batch size, channels (RGBA), height, width
    int shapeLen = 4;
    
    // Convert to CoreML
    if (FBNEO_PyTorch_ToCoreML_Convert(
            tempPath.c_str(),
            exportPath.c_str(),
            inputShape,
            shapeLen,
            1, // Use Neural Engine
            1  // Quantize model
        )) {
        std::cout << "Model exported to CoreML format: " << exportPath << std::endl;
        
        // Optimize for device
        std::string optimizedPath = exportPath + ".optimized.mlmodel";
        if (FBNEO_PyTorch_ToCoreML_Optimize(
                exportPath.c_str(),
                optimizedPath.c_str(),
                "ANE" // Target Apple Neural Engine
            )) {
            std::cout << "Model optimized for device: " << optimizedPath << std::endl;
        }
        
        return true;
    } else {
        std::cerr << "Failed to export model to CoreML format" << std::endl;
        return false;
    }
}

/**
 * @brief Configure distributed training settings
 * @param numWorkers Number of worker threads
 * @param syncInterval Synchronization interval in frames
 * @param learningRate Learning rate for training
 * @return True if successful
 */
bool configureDistributedTraining(int numWorkers, int syncInterval, float learningRate) {
    if (!g_initialized || !g_aiEnabled) {
        return false;
    }
    
    std::cout << "Configuring distributed training with " << numWorkers << " workers" << std::endl;
    std::cout << "Sync interval: " << syncInterval << " frames" << std::endl;
    std::cout << "Learning rate: " << learningRate << std::endl;
    
    try {
        // Get RL integration instance
        fbneo::ai::RLIntegration& rl = fbneo::ai::RLIntegration::getInstance();
        
        // Set hyperparameters
        std::unordered_map<std::string, float> params;
        params["learning_rate"] = learningRate;
        params["num_workers"] = static_cast<float>(numWorkers);
        params["sync_interval"] = static_cast<float>(syncInterval);
        rl.setHyperparameters(params);
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error configuring distributed training: " << e.what() << std::endl;
        return false;
    }
}

/**
 * @brief Get memory address for game-specific state variable
 * @param gameName Name of the game
 * @param varName Name of the variable
 * @return Memory address or 0 if not found
 */
uint32_t getGameMemoryAddress(const std::string& gameName, const std::string& varName) {
    // Game-specific memory mappings
    static const std::unordered_map<std::string, std::unordered_map<std::string, uint32_t>> memoryMap = {
        // Street Fighter II (various versions)
        {"sf2", {
            {"p1_health", 0xFF8390},
            {"p2_health", 0xFF8790},
            {"round_timer", 0xFF8AB4},
            {"p1_x", 0xFF8450},
            {"p1_y", 0xFF8452},
            {"p2_x", 0xFF8850},
            {"p2_y", 0xFF8852}
        }},
        // King of Fighters '98
        {"kof98", {
            {"p1_health", 0x108DA0},
            {"p2_health", 0x108FA0},
            {"timer", 0x10D902},
            {"p1_x", 0x108DD8},
            {"p1_y", 0x108DDC},
            {"p2_x", 0x108FD8},
            {"p2_y", 0x108FDC}
        }},
        // Metal Slug
        {"mslug", {
            {"p1_lives", 0x10E489},
            {"p1_bombs", 0x10E02A},
            {"p1_score", 0x10E416},
            {"p1_x", 0x10E010},
            {"p1_y", 0x10E014}
        }},
        // Puzzle Bobble / Bust-A-Move
        {"pbobble", {
            {"p1_score", 0xFF8856},
            {"p2_score", 0xFF8858},
            {"bubbles_left", 0xFF82FA},
            {"level", 0xFF8318}
        }},
        // Pac-Man
        {"pacman", {
            {"lives", 0x4E0E},
            {"score", 0x4E00},
            {"level", 0x4E13},
            {"pac_x", 0x4E0A},
            {"pac_y", 0x4E0B}
        }}
    };
    
    // Look up the game
    auto gameIt = memoryMap.find(gameName);
    if (gameIt != memoryMap.end()) {
        // Look up the variable
        auto varIt = gameIt->second.find(varName);
        if (varIt != gameIt->second.end()) {
            return varIt->second;
        }
    }
    
    return 0;
}

/**
 * @brief Optimize CoreML model for specific Apple hardware
 * @param inputPath Input CoreML model path
 * @param outputPath Output optimized model path
 * @param targetDevice Target device (CPU, GPU, ANE, ALL)
 * @return True if optimization was successful
 */
bool optimizeCoreMLForDevice(const std::string& inputPath, 
                             const std::string& outputPath,
                             const std::string& targetDevice) {
    std::cout << "Optimizing CoreML model for " << targetDevice << "..." << std::endl;
    
    // Validate input path
    if (!std::ifstream(inputPath)) {
        std::cerr << "Input CoreML model not found: " << inputPath << std::endl;
        return false;
    }
    
    // Create optimization command
    std::string cmd = "xcrun coremlcompiler optimize " + inputPath + " " + outputPath;
    
    // Add target device parameter
    if (targetDevice == "CPU") {
        cmd += " --cpu-only";
    } else if (targetDevice == "GPU") {
        cmd += " --gpu-only";
    } else if (targetDevice == "ANE") {
        cmd += " --ane-only";
    } else {
        // Default to ALL compute units
        cmd += " --all-compute-units";
    }
    
    // Add neuralnetwork option (required for Metal)
    cmd += " --minimum-deployment-target 14.0";
    
    // Execute the command
    int result = system(cmd.c_str());
    if (result != 0) {
        std::cerr << "Failed to optimize CoreML model. Error code: " << result << std::endl;
        return false;
    }
    
    std::cout << "CoreML model optimized successfully: " << outputPath << std::endl;
    return true;
}

/**
 * @brief Create a memory mapping for the current game
 * @return True if mapping was created successfully
 */
bool createGameMemoryMapping() {
    if (g_gameName.empty()) {
        return false;
    }
    
    std::cout << "Creating memory mapping for " << g_gameName << std::endl;
    
    // Generate path for memory mapping file
    std::string mappingPath = g_modelPath + g_gameName + ".memmap";
    
    // Check if we already have a mapping file
    std::ifstream existingMap(mappingPath);
    if (existingMap) {
        std::cout << "Using existing memory mapping from " << mappingPath << std::endl;
        
        // Parse the mapping file and set up memory hooks
        std::string line;
        while (std::getline(existingMap, line)) {
            // Format: variable_name=address
            size_t pos = line.find('=');
            if (pos != std::string::npos) {
                std::string varName = line.substr(0, pos);
                std::string addrStr = line.substr(pos + 1);
                
                try {
                    uint32_t address = std::stoul(addrStr, nullptr, 16);
                    std::cout << "  " << varName << " = 0x" << std::hex << address << std::dec << std::endl;
                    
                    // TODO: Set up memory hook for this address
                } catch (...) {
                    std::cerr << "Invalid address format in mapping file: " << line << std::endl;
                }
            }
        }
        
        existingMap.close();
        return true;
    }
    
    // No existing mapping, create one with known addresses
    std::cout << "Creating new memory mapping for " << g_gameName << std::endl;
    
    // Add game variables based on game type
    std::unordered_map<std::string, uint32_t> mapping;
    
    if (g_gameType == "fighting") {
        // Use addresses from known fighting games as a starting point
        mapping["p1_health"] = getGameMemoryAddress(g_gameName, "p1_health");
        mapping["p2_health"] = getGameMemoryAddress(g_gameName, "p2_health");
        mapping["timer"] = getGameMemoryAddress(g_gameName, "round_timer");
        mapping["p1_x"] = getGameMemoryAddress(g_gameName, "p1_x");
        mapping["p1_y"] = getGameMemoryAddress(g_gameName, "p1_y");
        mapping["p2_x"] = getGameMemoryAddress(g_gameName, "p2_x");
        mapping["p2_y"] = getGameMemoryAddress(g_gameName, "p2_y");
    } else if (g_gameType == "platformer") {
        mapping["lives"] = getGameMemoryAddress(g_gameName, "p1_lives");
        mapping["score"] = getGameMemoryAddress(g_gameName, "p1_score");
        mapping["player_x"] = getGameMemoryAddress(g_gameName, "p1_x");
        mapping["player_y"] = getGameMemoryAddress(g_gameName, "p1_y");
    } else if (g_gameType == "puzzle") {
        mapping["score"] = getGameMemoryAddress(g_gameName, "p1_score");
        mapping["level"] = getGameMemoryAddress(g_gameName, "level");
    } else if (g_gameType == "shooter") {
        mapping["lives"] = getGameMemoryAddress(g_gameName, "p1_lives");
        mapping["score"] = getGameMemoryAddress(g_gameName, "p1_score");
        mapping["player_x"] = getGameMemoryAddress(g_gameName, "p1_x");
        mapping["player_y"] = getGameMemoryAddress(g_gameName, "p1_y");
    }
    
    // Save the mapping to a file
    std::ofstream mapFile(mappingPath);
    if (mapFile) {
        for (const auto& pair : mapping) {
            if (pair.second != 0) {
                mapFile << pair.first << "=0x" << std::hex << pair.second << std::endl;
            }
        }
        mapFile.close();
        std::cout << "Memory mapping saved to " << mappingPath << std::endl;
        return true;
    } else {
        std::cerr << "Failed to create memory mapping file: " << mappingPath << std::endl;
        return false;
    }
}

} // namespace ai
} // namespace metal
} // namespace fbneo

// C API for integration with Metal renderer
extern "C" {
    int Metal_AI_Init(const char* configPath) {
        return fbneo::metal::ai::initialize(configPath) ? 1 : 0;
    }
    
    void Metal_AI_Exit() {
        fbneo::metal::ai::shutdown();
    }
    
    void Metal_AI_SetEnabled(int enable) {
        fbneo::metal::ai::setEnabled(enable != 0);
    }
    
    int Metal_AI_IsEnabled() {
        return fbneo::metal::ai::isEnabled() ? 1 : 0;
    }
    
    void Metal_AI_SetTrainingMode(int enable) {
        fbneo::metal::ai::setTrainingMode(enable != 0);
    }
    
    int Metal_AI_IsTrainingMode() {
        return fbneo::metal::ai::isTrainingMode() ? 1 : 0;
    }
    
    void Metal_AI_ProcessFrame(const void* frameData, int width, int height, int pitch) {
        fbneo::metal::ai::processFrame(frameData, width, height, pitch);
    }
    
    int Metal_AI_SaveModel(const char* path) {
        return fbneo::metal::ai::saveModel(path) ? 1 : 0;
    }
    
    int Metal_AI_LoadModel(const char* path) {
        return fbneo::metal::ai::loadModel(path) ? 1 : 0;
    }
    
    void Metal_AI_StartTrainingSession() {
        fbneo::metal::ai::startTrainingSession();
    }
    
    float Metal_AI_EndTrainingSession(int success) {
        return fbneo::metal::ai::endTrainingSession(success != 0);
    }
    
    const char* Metal_AI_GetGameType() {
        return fbneo::metal::ai::getGameType();
    }
    
    int Metal_AI_ExportToCoreML(const char* path) {
        return fbneo::metal::ai::exportToCoreML(path) ? 1 : 0;
    }
    
    int Metal_AI_ConfigureDistributedTraining(int numWorkers, int syncInterval, float learningRate) {
        return fbneo::metal::ai::configureDistributedTraining(numWorkers, syncInterval, learningRate) ? 1 : 0;
    }
    
    int Metal_AI_CreateGameMemoryMapping() {
        return fbneo::metal::ai::createGameMemoryMapping() ? 1 : 0;
    }
    
    int Metal_AI_OptimizeCoreMLForDevice(const char* inputPath, const char* outputPath, const char* targetDevice) {
        std::string target = targetDevice ? targetDevice : "ALL";
        return fbneo::metal::ai::optimizeCoreMLForDevice(
            inputPath ? inputPath : "",
            outputPath ? outputPath : "",
            target
        ) ? 1 : 0;
    }
}

// C interface for external use
extern "C" {
    int Metal_AI_Initialize() {
        return MetalAIModule::Initialize() ? 0 : 1;
    }
    
    void Metal_AI_Shutdown() {
        MetalAIModule::Shutdown();
    }
    
    int Metal_AI_ProcessFrame(void* frameData, int width, int height, int pitch) {
        return MetalAIModule::ProcessFrame(frameData, width, height, pitch) ? 0 : 1;
    }
    
    int Metal_AI_RenderOverlay(void* overlayData, int width, int height, int pitch) {
        return MetalAIModule::RenderVisualization(overlayData, width, height, pitch) ? 0 : 1;
    }
    
    void Metal_AI_SetActive(int enable) {
        MetalAIModule::SetActive(enable != 0);
    }
    
    int Metal_AI_IsActive() {
        return MetalAIModule::IsActive() ? 1 : 0;
    }
    
    void Metal_AI_SetVisualizationActive(int enable) {
        MetalAIModule::SetVisualizationActive(enable != 0);
    }
    
    void Metal_AI_SetVisualizationType(int type) {
        MetalAIModule::SetVisualizationType(type);
    }
    
    void Metal_AI_SetVisualizationOpacity(float opacity) {
        MetalAIModule::SetVisualizationOpacity(opacity);
    }
    
    int Metal_AI_GetModelInfo(void* info) {
        return MetalAIModule::GetModelInfo(static_cast<AIModelInfo*>(info)) ? 1 : 0;
    }
    
    float Metal_AI_GetCurrentActionConfidence() {
        return MetalAIModule::GetCurrentActionConfidence();
    }
    
    float Metal_AI_GetStateValue() {
        return MetalAIModule::GetStateValue();
    }
    
    int Metal_AI_GetTopActionCount() {
        return MetalAIModule::GetTopActionCount();
    }
    
    void Metal_AI_GetTopActionInfo(int index, char* actionName, float* confidence) {
        MetalAIModule::GetTopActionInfo(index, actionName, confidence);
    }
    
    int Metal_AI_LoadModel(const char* path) {
        return MetalAIModule::LoadModel(path) ? 1 : 0;
    }
} 