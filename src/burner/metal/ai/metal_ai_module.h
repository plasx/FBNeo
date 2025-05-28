#pragma once

#ifdef __cplusplus
namespace fbneo {
namespace metal {
namespace ai {

/**
 * @brief Initialize the AI module for Metal backend
 * @param configPath Path to configuration file (optional)
 * @return True if initialization was successful
 */
bool initialize(const char* configPath = nullptr);

/**
 * @brief Shutdown the AI module
 */
void shutdown();

/**
 * @brief Enable or disable AI
 * @param enable Whether to enable AI
 */
void setEnabled(bool enable);

/**
 * @brief Check if AI is enabled
 * @return True if AI is enabled
 */
bool isEnabled();

/**
 * @brief Enable or disable training mode
 * @param enable Whether to enable training mode
 */
void setTrainingMode(bool enable);

/**
 * @brief Check if training mode is enabled
 * @return True if training mode is enabled
 */
bool isTrainingMode();

/**
 * @brief Process a frame from the Metal renderer
 * @param frameData Frame buffer data
 * @param width Frame width
 * @param height Frame height
 * @param pitch Frame pitch (bytes per row)
 */
void processFrame(const void* frameData, int width, int height, int pitch);

/**
 * @brief Save the current AI model
 * @param path Path to save to (optional, uses default if not provided)
 * @return True if successful
 */
bool saveModel(const char* path = nullptr);

/**
 * @brief Load an AI model
 * @param path Path to load from (optional, uses default if not provided)
 * @return True if successful
 */
bool loadModel(const char* path = nullptr);

/**
 * @brief Start a training session
 */
void startTrainingSession();

/**
 * @brief End the current training session
 * @param success Whether the session was successful
 * @return The total reward for the session
 */
float endTrainingSession(bool success);

/**
 * @brief Get the game type for the current game
 * @return Game type string
 */
const char* getGameType();

/**
 * @brief Export the current AI model to CoreML format
 * @param path Path to save to (optional, uses default if not provided)
 * @return True if successful
 */
bool exportToCoreML(const char* path = nullptr);

/**
 * @brief Configure distributed training settings
 * @param numWorkers Number of worker threads
 * @param syncInterval Synchronization interval in frames
 * @param learningRate Learning rate for training
 * @return True if successful
 */
bool configureDistributedTraining(int numWorkers, int syncInterval, float learningRate);

/**
 * @brief Get memory address for game-specific state variable
 * @param gameName Name of the game
 * @param varName Name of the variable
 * @return Memory address or 0 if not found
 */
uint32_t getGameMemoryAddress(const std::string& gameName, const std::string& varName);

/**
 * @brief Optimize CoreML model for specific Apple hardware
 * @param inputPath Input CoreML model path
 * @param outputPath Output optimized model path
 * @param targetDevice Target device (CPU, GPU, ANE, ALL)
 * @return True if optimization was successful
 */
bool optimizeCoreMLForDevice(const std::string& inputPath, 
                            const std::string& outputPath,
                            const std::string& targetDevice);

/**
 * @brief Create a memory mapping for the current game
 * @return True if mapping was created successfully
 */
bool createGameMemoryMapping();

} // namespace ai
} // namespace metal
} // namespace fbneo
#endif

#ifdef __cplusplus
extern "C" {
#endif

// C API for integration with Metal renderer
int Metal_AI_Init(const char* configPath);
void Metal_AI_Exit();
void Metal_AI_SetEnabled(int enable);
int Metal_AI_IsEnabled();
void Metal_AI_SetTrainingMode(int enable);
int Metal_AI_IsTrainingMode();
void Metal_AI_ProcessFrame(const void* frameData, int width, int height, int pitch);
int Metal_AI_SaveModel(const char* path);
int Metal_AI_LoadModel(const char* path);
void Metal_AI_StartTrainingSession();
float Metal_AI_EndTrainingSession(int success);
const char* Metal_AI_GetGameType();
int Metal_AI_ExportToCoreML(const char* path);
int Metal_AI_ConfigureDistributedTraining(int numWorkers, int syncInterval, float learningRate);
int Metal_AI_CreateGameMemoryMapping();
int Metal_AI_OptimizeCoreMLForDevice(const char* inputPath, const char* outputPath, const char* targetDevice);

#ifdef __cplusplus
}
#endif 