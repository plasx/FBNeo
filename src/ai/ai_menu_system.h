#pragma once

/**
 * @file ai_menu_system.h
 * @brief User interface for AI configuration and control
 */

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <optional>
#include <unordered_map>
#include <filesystem>
#include "neural_ai_controller.h"

namespace AI {

// Forward declarations
class NeuralAIController;

// Enum for AI difficulty levels
enum class AIDifficultyLevel {
    EASY,
    MEDIUM,
    HARD,
    EXPERT
};

// Structure to represent an AI model option
struct AIModelOption {
    std::string name;           // Display name
    std::string filepath;       // Path to model file
    std::string description;    // Brief description of the model
    bool isBuiltIn;             // Whether this is a built-in model
};

/**
 * @class AIMenuSystem
 * @brief Manages the AI menu interface and player settings
 * 
 * Provides a user interface for configuring AI opponents, including:
 * - Enabling/disabling AI control for players
 * - Selecting AI models for each player
 * - Adjusting difficulty settings
 * - Saving/loading menu state
 */
class AIMenuSystem {
public:
    struct PlayerSettings {
        bool ai_controlled = false;
        std::string model_file = "";
        float difficulty = 0.5f; // 0.0 to 1.0
    };

    // Callback types
    using PlayerAIChangedCallback = std::function<void(int player_index, bool enabled)>;
    using PlayerModelChangedCallback = std::function<void(int player_index, const std::string& model_file)>;
    using PlayerDifficultyChangedCallback = std::function<void(int player_index, float difficulty)>;

    // Singleton pattern
    static AIMenuSystem& getInstance();
    
    /**
     * @brief Initialize the menu system
     * @param controller Pointer to the NeuralAIController
     * @return true if initialization succeeded, false otherwise
     */
    bool Init(std::shared_ptr<NeuralAIController> controller);
    
    /**
     * @brief Process input for the menu
     * @param dt Delta time since last frame
     */
    void HandleInput(float dt);
    
    /**
     * @brief Render the menu
     */
    void Render();
    
    /**
     * @brief Show the AI menu
     */
    void ShowAIMenu();
    
    /**
     * @brief Hide the AI menu
     */
    void HideAIMenu();
    
    /**
     * @brief Check if the menu is currently visible
     * @return true if the menu is visible, false otherwise
     */
    bool IsMenuVisible() const;
    
    /**
     * @brief Set whether a player is AI controlled
     * @param player_index Player index (0-based)
     * @param enabled Whether AI control is enabled
     */
    void SetPlayerAIControlled(int player_index, bool enabled);
    
    /**
     * @brief Check if a player is AI controlled
     * @param player_index Player index (0-based)
     * @return true if the player is AI controlled, false otherwise
     */
    bool IsPlayerAIControlled(int player_index) const;
    
    /**
     * @brief Get a list of all AI controlled players
     * @return Vector of player indices
     */
    std::vector<int> GetAIControlledPlayers() const;
    
    /**
     * @brief Set the model file for a player
     * @param player_index Player index (0-based)
     * @param model_file Path to the model file
     * @return true if the model file was set successfully, false otherwise
     */
    bool SetPlayerModelFile(int player_index, const std::string& model_file);
    
    /**
     * @brief Get the model file for a player
     * @param player_index Player index (0-based)
     * @return Path to the model file, or empty string if none is set
     */
    std::string GetPlayerModelFile(int player_index) const;
    
    /**
     * @brief Set the difficulty for a player
     * @param player_index Player index (0-based)
     * @param difficulty Difficulty value (0.0 to 1.0)
     */
    void SetPlayerDifficulty(int player_index, float difficulty);
    
    /**
     * @brief Get the difficulty for a player
     * @param player_index Player index (0-based)
     * @return Difficulty value (0.0 to 1.0)
     */
    float GetPlayerDifficulty(int player_index) const;
    
    /**
     * @brief Set callback for when a player's AI control changes
     * @param callback Function to call when a player's AI control changes
     */
    void SetOnPlayerAIChanged(PlayerAIChangedCallback callback);
    
    /**
     * @brief Set callback for when a player's model changes
     * @param callback Function to call when a player's model changes
     */
    void SetOnPlayerModelChanged(PlayerModelChangedCallback callback);
    
    /**
     * @brief Set callback for when a player's difficulty changes
     * @param callback Function to call when a player's difficulty changes
     */
    void SetOnPlayerDifficultyChanged(PlayerDifficultyChangedCallback callback);
    
    /**
     * @brief Get the menu state as a JSON string
     * @return JSON string containing the menu state
     */
    std::string GetMenuStateJson() const;
    
    /**
     * @brief Load the menu state from a JSON string
     * @param json JSON string containing the menu state
     * @return true if the state was loaded successfully, false otherwise
     */
    bool LoadMenuStateJson(const std::string& json);
    
    /**
     * @brief Scan for model files
     * @param directory Directory to scan for model files
     * @return Vector of model file paths
     */
    std::vector<std::string> ScanModelFiles(const std::string& directory = "");
    
    /**
     * @brief Get information about a model file
     * @param model_file Path to the model file
     * @return Map of key-value pairs with model information
     */
    std::unordered_map<std::string, std::string> GetModelInfo(const std::string& model_file);
    
    // Player control methods
    bool enableAIControl(int playerIndex, bool enable);
    bool isAIControlEnabled(int playerIndex) const;
    
    // Model selection methods
    bool selectModel(const std::string& modelPath, int playerIndex);
    std::string getCurrentModelPath(int playerIndex) const;
    
    // Get available AI models
    std::vector<AIModelOption> getAvailableModels() const;
    void refreshAvailableModels();
    
    // Difficulty settings
    void setDifficultyLevel(AIDifficultyLevel level, int playerIndex);
    AIDifficultyLevel getDifficultyLevel(int playerIndex) const;
    
    // Apply difficulty to controller
    void applyDifficultySettings(std::shared_ptr<NeuralAIController> controller, AIDifficultyLevel level);
    
    // Update - called each frame to handle any menu-related tasks
    void update();
    
    // Initialize the menu system with a reference to the AI controller
    void initialize(std::shared_ptr<NeuralAIController> controller);
    
    // Draw the main AI menu
    bool drawMainMenu();
    
    // Draw the AI configuration menu
    bool drawConfigMenu();
    
    // Draw the model selection menu
    bool drawModelSelectionMenu(int playerIndex);
    
    // Toggle AI control for a player
    void toggleAIControl(int playerIndex);
    
    // Set difficulty level
    void setDifficulty(float level);
    
    // Set the random action probability
    void setRandomActionProbability(float probability);
    
    // Set the reaction delay
    void setReactionDelay(int frames);
    
    // Load a model for a specific player
    bool loadModelForPlayer(const std::string& modelPath, int playerIndex);
    
    // Check if there's a model loaded for a player
    bool isModelLoadedForPlayer(int playerIndex) const;
    
    // Scan the models directory for available models
    void scanAvailableModels();
    
    // Get the name of the currently loaded model for a player
    std::string getLoadedModelName(int playerIndex) const;
    
    // Check if the menu system is initialized
    bool isInitialized() const { return m_initialized; }
    
    // Get player control status
    bool isControllingPlayer(int playerIndex) const;
    
    /**
     * @brief FBNeo integration - Build the menu
     * @param pszTitle Menu title
     * @param bSelectOnly Whether only selection is allowed
     */
    void BuildMenu(const char* pszTitle, bool bSelectOnly);
    
    /**
     * @brief FBNeo integration - Handle menu selection
     * @param nVal Selected menu value
     */
    void HandleMenuSelect(int nVal);

private:
    /**
     * @brief Update the player settings
     * @param player_index Player index (0-based)
     */
    void UpdatePlayerSettings(int player_index);
    
    /**
     * @brief Render the file dialog
     * @return true if a file was selected, false otherwise
     */
    bool RenderFileDialog();
    
    /**
     * @brief Apply the player settings to the controller
     * @param player_index Player index (0-based)
     */
    void ApplyPlayerSettings(int player_index);
    
    // Internal state
    bool m_aiEnabled[2];                       // Control status for 2 players
    std::string m_currentModelPath[2];         // Selected model paths
    AIDifficultyLevel m_difficultyLevel[2];    // Selected difficulty levels
    std::vector<AIModelOption> m_availableModels;  // Available models
    std::vector<PlayerAIChangedCallback> m_playerAIChangedCallbacks;
    std::vector<PlayerModelChangedCallback> m_playerModelChangedCallbacks;
    std::vector<PlayerDifficultyChangedCallback> m_playerDifficultyChangedCallbacks;
    
    // Internal methods
    void loadDefaultModels();
    void scanUserModelDirectory();
    std::string getModelDirectory() const;
    
    // Singleton instance
    static AIMenuSystem* s_instance;
    
    // Menu state
    bool m_menu_visible;
    int m_selected_player;
    size_t m_selected_model;
    
    // AI controller reference
    std::shared_ptr<NeuralAIController> m_controller;
    
    // Model files
    std::vector<std::string> m_model_files;
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> m_model_info;
    
    // Player settings
    std::unordered_map<int, PlayerSettings> m_player_settings;
    
    // File dialog state
    bool m_file_dialog_open;
    std::string m_file_dialog_path;
    
    // Callbacks
    PlayerAIChangedCallback m_on_player_ai_changed;
    PlayerModelChangedCallback m_on_player_model_changed;
    PlayerDifficultyChangedCallback m_on_player_difficulty_changed;
    
    // Additional member variables for the new methods
    bool m_initialized;
    bool m_configMenuOpen;
    bool m_modelSelectionMenuOpen[2];
    int m_activePlayerForModelSelection;
};

// Global instance
extern AIMenuSystem* g_pAIMenuSystem;

// External C interface for FBNeo integration
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the AI menu system
 * 
 * @return 0 on success, non-zero on failure
 */
int AIMenuSystemInit();

/**
 * @brief Clean up the AI menu system
 * 
 * @return 0 on success, non-zero on failure
 */
int AIMenuSystemExit();

/**
 * @brief Build the AI options menu
 * 
 * @param pszTitle Title for the menu
 * @param bSelectOnly Whether only selection is allowed
 */
void AIMenuSystemBuildMenu(const char* pszTitle, bool bSelectOnly);

/**
 * @brief Handle selection from the menu
 * 
 * @param nVal Selected menu value
 */
void AIMenuSystemHandleMenuSelect(int nVal);

#ifdef __cplusplus
}
#endif 
} // namespace AI 