#pragma once

#include <cstdint>

namespace AI {

// Forward declarations
class NeuralAIController;

/**
 * @brief Initialize the AI input system
 * 
 * This should be called during application initialization
 * to set up the AI controller for input integration.
 * 
 * @return true if initialized successfully, false otherwise
 */
bool InitializeAIInputSystem();

/**
 * @brief Shutdown the AI input system
 * 
 * This should be called during application shutdown
 * to clean up resources.
 */
void ShutdownAIInputSystem();

/**
 * @brief Process AI inputs for a frame
 * 
 * This should be called once per frame in the main game loop
 * to update the AI controller and process AI inputs.
 */
void ProcessAIFrame();

/**
 * @brief Get AI inputs for a player
 * 
 * This function is called by the input system to get
 * inputs for a player controlled by AI.
 * 
 * @param playerIndex The player index (0-based)
 * @return Input bits for the player (0 if not AI-controlled)
 */
uint32_t GetAIInputs(int playerIndex);

/**
 * @brief Show visual indicator for AI-controlled player
 * 
 * This function draws a visual indicator on screen to
 * show that a player is being controlled by AI.
 * 
 * @param playerIndex The player index (0-based)
 */
void ShowAIIndicator(int playerIndex);

/**
 * @brief Get the AI controller instance
 * 
 * Used by the menu system to interface with the controller.
 * 
 * @return The NeuralAIController instance, or nullptr if not initialized
 */
NeuralAIController* GetAIController();

} // namespace AI 