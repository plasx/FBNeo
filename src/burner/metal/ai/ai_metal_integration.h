#pragma once

#include "ai_rl_algorithms.h"
#include "ai_torch_policy.h"
#include "ai_input_frame.h"
#include "ai_output_action.h"
#include "ai_memory_mapping.h"

namespace fbneo {
namespace ai {

/**
 * @brief Initialize the AI systems for the Metal implementation
 * 
 * @param modelPath Path to the model file, or empty string for a new model
 * @param algorithmType Type of RL algorithm to use (default: "ppo")
 * @return true if initialization succeeded
 */
bool Metal_InitializeAI(const char* modelPath = "", const char* algorithmType = "ppo");

/**
 * @brief Shutdown AI systems
 */
void Metal_ShutdownAI();

/**
 * @brief Process a frame with the AI system
 * 
 * @param frameBuffer Pointer to RGBA frame data
 * @param width Frame width
 * @param height Frame height
 * @param gameState Current game state
 * @return AIOutputAction The AI's chosen action
 */
AIOutputAction Metal_ProcessAIFrame(const void* frameBuffer, int width, int height, const GameState& gameState);

/**
 * @brief Save the current AI model
 * 
 * @param path Path to save the model
 * @return true if save succeeded
 */
bool Metal_SaveAIModel(const char* path);

/**
 * @brief Load an AI model
 * 
 * @param path Path to the model
 * @return true if load succeeded
 */
bool Metal_LoadAIModel(const char* path);

/**
 * @brief Set training mode for the AI
 * 
 * @param enabled Whether training is enabled
 */
void Metal_SetAITrainingMode(bool enabled);

/**
 * @brief Is the AI in training mode?
 * 
 * @return true if in training mode
 */
bool Metal_IsAITrainingMode();

/**
 * @brief Calculate reward for the current state
 * 
 * @param prevState Previous game state
 * @param currentState Current game state
 * @return float Reward value
 */
float Metal_CalculateReward(const GameState& prevState, const GameState& currentState);

/**
 * @brief Extract game state from memory
 * 
 * @param gameMapping Memory mapping for the game
 * @param state GameState to fill
 * @return true if state extraction succeeded
 */
bool Metal_ExtractGameState(const AIMemoryMapping& gameMapping, GameState& state);

/**
 * @brief Check if the current episode is over
 * 
 * @param state Current game state
 * @return true if episode is over
 */
bool Metal_IsEpisodeOver(const GameState& state);

/**
 * @brief Apply AI action to the game input system
 * 
 * @param action AI output action
 * @param playerIndex Player index to apply input to
 */
void Metal_ApplyAIAction(const AIOutputAction& action, int playerIndex);

} // namespace ai
} // namespace fbneo 