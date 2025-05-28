#include "burner.h"
#include "ai_menu_system.h"
#include "neural_ai_controller.h"
#include <vector>
#include <string>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <map>
#include <unordered_map>
#include <memory>

// For file path handling
#ifdef _WIN32
#include <windows.h>
#define PATH_SEPARATOR "\\"
#else
#define PATH_SEPARATOR "/"
#endif

// Namespace alias for std::filesystem with compatibility check
#if defined(__cplusplus) && __cplusplus >= 201703L && defined(__has_include)
#if __has_include(<filesystem>)
namespace fs = std::filesystem;
#else
namespace fs = std::experimental::filesystem;
#endif
#else
namespace fs = std::experimental::filesystem;
#endif

// Global instance
AI::AIMenuSystem* g_pAIMenuSystem = nullptr;

// Menu ID enumeration
enum AIMenuItems {
    // Main menu items
    MENU_AI_ENABLE = 1000,
    MENU_AI_MODEL_SELECT,
    MENU_AI_PLAY_MODE,
    MENU_AI_TRAINING_OPTIONS,
    MENU_AI_MEMORY_MAPPING,
    MENU_AI_ADVANCED_OPTIONS,
    
    // Model selection menu items
    MENU_AI_MODEL_SELECT_START = 2000,
    MENU_AI_MODEL_SELECT_END = 2999,
    MENU_AI_MODEL_REFRESH = 2999,
    
    // Play mode menu items
    MENU_AI_MODE_HUMAN = 3000,
    MENU_AI_MODE_AI_PLAY,
    MENU_AI_MODE_AI_ASSIST,
    MENU_AI_MODE_RECORD,
    
    // Training options menu items
    MENU_AI_TRAIN_ENABLE_COLLECTION = 4000,
    MENU_AI_TRAIN_COLLECTION_RATE,
    MENU_AI_TRAIN_VISUALIZE_COLLECTION,
    MENU_AI_TRAIN_EXPORT_DATASET,
    
    // Memory mapping menu items
    MENU_AI_MEM_REFRESH_MAPPING = 5000,
    MENU_AI_MEM_EDIT_MAPPING,
    MENU_AI_MEM_VISUALIZE_MAPPING,
    
    // Advanced options menu items
    MENU_AI_ADV_ENABLE_DEBUG = 6000,
    MENU_AI_ADV_REPLAY_VALIDATION,
    MENU_AI_ADV_DETERMINISM_DASHBOARD,
    MENU_AI_ADV_NETWORK_SETTINGS,
};

// Play modes
enum class AIPlayMode {
    Human,
    AIPlay,
    AIAssist,
    Record
};

// Menu option structure
struct MenuOption {
    int id;
    std::string name;
    bool* pBool;
    int* pInt;
    std::vector<std::string> stringValues;
    int currentStringValue;
};

namespace AI {

// Maximum number of players supported
constexpr int MAX_PLAYERS = 2;

// Initialize static members
AIMenuSystem* AIMenuSystem::s_instance = nullptr;

// Singleton instance function
AIMenuSystem& AIMenuSystem::getInstance() {
    if (!s_instance) {
        s_instance = new AIMenuSystem();
    }
    return *s_instance;
}

AIMenuSystem::AIMenuSystem() noexcept
    : m_initialized(false)
    , m_configMenuOpen(false)
    , m_activePlayerForModelSelection(-1)
    , m_menu_visible(false)
    , m_selected_player(0)
    , m_selected_model(0)
    , m_file_dialog_open(false)
    , m_file_dialog_path("")
    , m_controller(nullptr)
{
    m_modelSelectionMenuOpen[0] = false;
    m_modelSelectionMenuOpen[1] = false;
    
    // Initialize player settings
    for (int i = 0; i < MAX_PLAYERS; i++) {
        m_aiEnabled[i] = false;
        m_currentModelPath[i] = "";
        m_difficultyLevel[i] = AIDifficultyLevel::MEDIUM;
    }
}

AIMenuSystem::~AIMenuSystem() {
    // Save settings before destruction
    try {
        std::string json = GetMenuStateJson();
        std::string configDir = "config";
        
        if (!fs::exists(configDir)) {
            fs::create_directory(configDir);
        }
        
        std::ofstream file(configDir + "/ai_menu_settings.json");
        if (file.is_open()) {
            file << json;
            file.close();
        }
    } catch (const std::exception& e) {
        std::cerr << "Error saving AI menu settings: " << e.what() << std::endl;
    }
}

bool AIMenuSystem::Init(std::shared_ptr<NeuralAIController> controller) {
    m_controller = controller;
    m_initialized = (m_controller != nullptr);
    
    // Reset menu state
    m_menu_visible = false;
    m_selected_player = 0;
    m_selected_model = 0;
    m_file_dialog_open = false;
    m_file_dialog_path = "";
    
    // Initialize player settings
    for (int i = 0; i < MAX_PLAYERS; i++) {
        m_aiEnabled[i] = false;
        m_currentModelPath[i] = "";
        m_difficultyLevel[i] = AIDifficultyLevel::MEDIUM;
        m_modelSelectionMenuOpen[i] = false;
    }
    
    // Scan for available models
    if (m_initialized) {
        scanAvailableModels();
        loadDefaultModels();
        
        // Try to load saved settings
        try {
            std::string configDir = "config";
            std::string configPath = configDir + "/ai_menu_settings.json";
            
            if (fs::exists(configPath)) {
                std::ifstream file(configPath);
                if (file.is_open()) {
                    std::string jsonStr((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                    file.close();
                    LoadMenuStateJson(jsonStr);
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Error loading AI menu settings: " << e.what() << std::endl;
        }
    }
    
    return m_initialized;
}

void AIMenuSystem::HandleInput(float dt) {
    // Check for menu toggle key combination
    // This would typically be implemented with your game's input system
    // For example:
    // if (IsKeyPressed(KEY_F1) && (IsKeyDown(KEY_LALT) || IsKeyDown(KEY_RALT))) {
    //     ToggleMenu();
    // }
}

void AIMenuSystem::Render() {
    if (!m_menu_visible || !m_initialized) {
        return;
    }
    
    // This would be implemented using your UI system (e.g., ImGui)
    // For example with ImGui:
    // drawMainMenu();
    
    // If file dialog is open, render it
    if (m_file_dialog_open) {
        RenderFileDialog();
    }
}

void AIMenuSystem::ShowAIMenu() {
    m_menu_visible = true;
}

void AIMenuSystem::HideAIMenu() {
    m_menu_visible = false;
}

bool AIMenuSystem::IsMenuVisible() const {
    return m_menu_visible;
}

void AIMenuSystem::SetPlayerAIControlled(int player_index, bool enabled) {
    if (player_index < 0 || player_index >= MAX_PLAYERS) {
        return;
    }
    
    m_aiEnabled[player_index] = enabled;
    
    // Update player settings
    UpdatePlayerSettings(player_index);
    
    // Apply settings to controller
    ApplyPlayerSettings(player_index);
    
    // Call callbacks if registered
    if (m_on_player_ai_changed) {
        m_on_player_ai_changed(player_index, enabled);
    }
}

bool AIMenuSystem::IsPlayerAIControlled(int player_index) const {
    if (player_index < 0 || player_index >= MAX_PLAYERS) {
        return false;
    }
    
    return m_aiEnabled[player_index];
}

std::vector<int> AIMenuSystem::GetAIControlledPlayers() const {
    std::vector<int> result;
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (m_aiEnabled[i]) {
            result.push_back(i);
        }
    }
    return result;
}

bool AIMenuSystem::SetPlayerModelFile(int player_index, const std::string& model_file) {
    if (player_index < 0 || player_index >= MAX_PLAYERS) {
        return false;
    }
    
    // Check if file exists
    if (!model_file.empty()) {
        try {
            if (!fs::exists(model_file)) {
                return false;
            }
        } catch (const std::exception& e) {
            std::cerr << "Error checking model file: " << e.what() << std::endl;
            return false;
        }
    }
    
    // Update model path
    m_currentModelPath[player_index] = model_file;
    
    // Update player settings
    UpdatePlayerSettings(player_index);
    
    // Apply settings to controller
    ApplyPlayerSettings(player_index);
    
    // Call callbacks if registered
    if (m_on_player_model_changed) {
        m_on_player_model_changed(player_index, model_file);
    }
    
    return true;
}

std::string AIMenuSystem::GetPlayerModelFile(int player_index) const {
    if (player_index < 0 || player_index >= MAX_PLAYERS) {
        return "";
    }
    
    return m_currentModelPath[player_index];
}

void AIMenuSystem::SetPlayerDifficulty(int player_index, float difficulty) {
    if (player_index < 0 || player_index >= MAX_PLAYERS) {
        return;
    }
    
    // Clamp difficulty to valid range
    float clamped_difficulty = std::min(1.0f, std::max(0.0f, difficulty));
    
    // Map float difficulty to enum
    if (clamped_difficulty < 0.25f) {
        m_difficultyLevel[player_index] = AIDifficultyLevel::EASY;
    } else if (clamped_difficulty < 0.5f) {
        m_difficultyLevel[player_index] = AIDifficultyLevel::MEDIUM;
    } else if (clamped_difficulty < 0.75f) {
        m_difficultyLevel[player_index] = AIDifficultyLevel::HARD;
    } else {
        m_difficultyLevel[player_index] = AIDifficultyLevel::EXPERT;
    }
    
    // Update player settings
    UpdatePlayerSettings(player_index);
    
    // Apply settings to controller
    ApplyPlayerSettings(player_index);
    
    // Call callbacks if registered
    if (m_on_player_difficulty_changed) {
        m_on_player_difficulty_changed(player_index, clamped_difficulty);
    }
}

float AIMenuSystem::GetPlayerDifficulty(int player_index) const {
    if (player_index < 0 || player_index >= MAX_PLAYERS) {
        return 0.5f; // Default to medium
    }
    
    // Map enum to float value
    switch (m_difficultyLevel[player_index]) {
        case AIDifficultyLevel::EASY:
            return 0.0f;
        case AIDifficultyLevel::MEDIUM:
            return 0.33f;
        case AIDifficultyLevel::HARD:
            return 0.66f;
        case AIDifficultyLevel::EXPERT:
            return 1.0f;
        default:
            return 0.5f;
    }
}

void AIMenuSystem::SetOnPlayerAIChanged(PlayerAIChangedCallback callback) {
    m_on_player_ai_changed = callback;
}

void AIMenuSystem::SetOnPlayerModelChanged(PlayerModelChangedCallback callback) {
    m_on_player_model_changed = callback;
}

void AIMenuSystem::SetOnPlayerDifficultyChanged(PlayerDifficultyChangedCallback callback) {
    m_on_player_difficulty_changed = callback;
}

std::string AIMenuSystem::GetMenuStateJson() const {
    // Simple JSON string generation without external library dependency
    std::string json = "{\n";
    json += "  \"players\": {\n";
    
    for (int i = 0; i < MAX_PLAYERS; i++) {
        json += "    \"" + std::to_string(i) + "\": {\n";
        json += "      \"ai_controlled\": " + std::string(m_aiEnabled[i] ? "true" : "false") + ",\n";
        json += "      \"model_file\": \"" + m_currentModelPath[i] + "\",\n";
        
        // Convert difficulty enum to string
        std::string difficulty = "MEDIUM";
        switch (m_difficultyLevel[i]) {
            case AIDifficultyLevel::EASY:
                difficulty = "EASY";
                break;
            case AIDifficultyLevel::MEDIUM:
                difficulty = "MEDIUM";
                break;
            case AIDifficultyLevel::HARD:
                difficulty = "HARD";
                break;
            case AIDifficultyLevel::EXPERT:
                difficulty = "EXPERT";
                break;
        }
        json += "      \"difficulty\": \"" + difficulty + "\"\n";
        
        json += "    }" + std::string(i < MAX_PLAYERS - 1 ? "," : "") + "\n";
    }
    
    json += "  }\n";
    json += "}";
    
    return json;
}

bool AIMenuSystem::LoadMenuStateJson(const std::string& json_str) {
    try {
        // Very simple parsing - look for key patterns
        for (int i = 0; i < MAX_PLAYERS; i++) {
            std::string player_str = "\"" + std::to_string(i) + "\"";
            size_t player_pos = json_str.find(player_str);
            if (player_pos == std::string::npos) continue;
            
            // Try to get AI controlled status
            size_t ai_controlled_pos = json_str.find("\"ai_controlled\"", player_pos);
            if (ai_controlled_pos != std::string::npos) {
                size_t value_pos = json_str.find(":", ai_controlled_pos);
                if (value_pos != std::string::npos) {
                    size_t true_pos = json_str.find("true", value_pos);
                    size_t false_pos = json_str.find("false", value_pos);
                    size_t next_comma = json_str.find(",", value_pos);
                    
                    if (true_pos != std::string::npos && 
                        (next_comma == std::string::npos || true_pos < next_comma)) {
                        m_aiEnabled[i] = true;
                    } else if (false_pos != std::string::npos &&
                              (next_comma == std::string::npos || false_pos < next_comma)) {
                        m_aiEnabled[i] = false;
                    }
                }
            }
            
            // Try to get model file
            size_t model_file_pos = json_str.find("\"model_file\"", player_pos);
            if (model_file_pos != std::string::npos) {
                size_t value_start = json_str.find("\"", model_file_pos + 13);
                if (value_start != std::string::npos) {
                    size_t value_end = json_str.find("\"", value_start + 1);
                    if (value_end != std::string::npos) {
                        std::string model_file = json_str.substr(value_start + 1, value_end - value_start - 1);
                        if (model_file.empty() || fs::exists(model_file)) {
                            m_currentModelPath[i] = model_file;
                        }
                    }
                }
            }
            
            // Try to get difficulty
            size_t difficulty_pos = json_str.find("\"difficulty\"", player_pos);
            if (difficulty_pos != std::string::npos) {
                size_t value_start = json_str.find("\"", difficulty_pos + 13);
                if (value_start != std::string::npos) {
                    size_t value_end = json_str.find("\"", value_start + 1);
                    if (value_end != std::string::npos) {
                        std::string difficulty = json_str.substr(value_start + 1, value_end - value_start - 1);
                        if (difficulty == "EASY") {
                            m_difficultyLevel[i] = AIDifficultyLevel::EASY;
                        } else if (difficulty == "MEDIUM") {
                            m_difficultyLevel[i] = AIDifficultyLevel::MEDIUM;
                        } else if (difficulty == "HARD") {
                            m_difficultyLevel[i] = AIDifficultyLevel::HARD;
                        } else if (difficulty == "EXPERT") {
                            m_difficultyLevel[i] = AIDifficultyLevel::EXPERT;
                        }
                    }
                }
            }
            
            // Apply settings to controller
            ApplyPlayerSettings(i);
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error parsing AI menu settings: " << e.what() << std::endl;
        return false;
    }
}

std::vector<std::string> AIMenuSystem::ScanModelFiles(const std::string& directory) {
    std::vector<std::string> model_files;
    
    std::string model_dir = directory;
    if (model_dir.empty()) {
        model_dir = getModelDirectory();
    }
    
    try {
        if (fs::exists(model_dir) && fs::is_directory(model_dir)) {
            for (const auto& entry : fs::recursive_directory_iterator(model_dir)) {
                if (entry.is_regular_file()) {
                    std::string ext = entry.path().extension().string();
                    std::transform(ext.begin(), ext.end(), ext.begin(), 
                        [](unsigned char c) { return std::tolower(c); });
                    
                    // Check for model file extensions
                    if (ext == ".pt" || ext == ".pth" || ext == ".onnx" || ext == ".bin") {
                        model_files.push_back(entry.path().string());
                    }
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error scanning model files: " << e.what() << std::endl;
    }
    
    return model_files;
}

std::unordered_map<std::string, std::string> AIMenuSystem::GetModelInfo(const std::string& model_file) {
    std::unordered_map<std::string, std::string> info;
    
    try {
        if (model_file.empty() || !fs::exists(model_file)) {
            info["error"] = "File does not exist";
            return info;
        }
        
        // Extract basic info from filename
        fs::path path(model_file);
        std::string filename = path.filename().string();
        std::string basename = path.stem().string();
        std::string extension = path.extension().string();
        
        info["filename"] = filename;
        info["path"] = model_file;
        info["size"] = std::to_string(fs::file_size(model_file)) + " bytes";
        
        // Try to parse additional info from the filename or metadata file
        
        // Example: sf2_Ken_v1.0.pt -> game: sf2, character: Ken, version: 1.0
        size_t firstUnderscore = basename.find('_');
        if (firstUnderscore != std::string::npos) {
            info["game"] = basename.substr(0, firstUnderscore);
            
            size_t secondUnderscore = basename.find('_', firstUnderscore + 1);
            if (secondUnderscore != std::string::npos) {
                info["character"] = basename.substr(firstUnderscore + 1, secondUnderscore - firstUnderscore - 1);
                info["version"] = basename.substr(secondUnderscore + 1);
            } else {
                info["character"] = basename.substr(firstUnderscore + 1);
            }
        } else {
            info["name"] = basename;
        }
    } catch (const std::exception& e) {
        info["error"] = e.what();
    }
    
    return info;
}

bool AIMenuSystem::enableAIControl(int playerIndex, bool enable) {
    if (playerIndex < 0 || playerIndex >= MAX_PLAYERS) {
        return false;
    }
    
    m_aiEnabled[playerIndex] = enable;
    
    if (m_controller) {
        m_controller->setControllingPlayer(playerIndex, enable);
    }
    
    return true;
}

bool AIMenuSystem::isAIControlEnabled(int playerIndex) const {
    if (playerIndex < 0 || playerIndex >= MAX_PLAYERS) {
        return false;
    }
    
    return m_aiEnabled[playerIndex];
}

bool AIMenuSystem::selectModel(const std::string& modelPath, int playerIndex) {
    return SetPlayerModelFile(playerIndex, modelPath);
}

std::string AIMenuSystem::getCurrentModelPath(int playerIndex) const {
    return GetPlayerModelFile(playerIndex);
}

std::vector<AIModelOption> AIMenuSystem::getAvailableModels() const {
    std::vector<AIModelOption> options;
    
    for (const auto& model : m_availableModels) {
        options.push_back(model);
    }
    
    return options;
}

void AIMenuSystem::refreshAvailableModels() {
    scanAvailableModels();
}

void AIMenuSystem::setDifficultyLevel(AIDifficultyLevel level, int playerIndex) {
    if (playerIndex < 0 || playerIndex >= MAX_PLAYERS) {
        return;
    }
    
    m_difficultyLevel[playerIndex] = level;
    
    // Apply difficulty settings to the controller
    ApplyPlayerSettings(playerIndex);
}

AIDifficultyLevel AIMenuSystem::getDifficultyLevel(int playerIndex) const {
    if (playerIndex < 0 || playerIndex >= MAX_PLAYERS) {
        return AIDifficultyLevel::MEDIUM;  // Default
    }
    
    return m_difficultyLevel[playerIndex];
}

void AIMenuSystem::applyDifficultySettings(std::shared_ptr<NeuralAIController> controller, AIDifficultyLevel level) {
    if (!controller) {
        return;
    }
    
    float difficulty = 0.5f;  // Default medium
    float randomActionProb = 0.1f;  // Default medium randomness
    int reactionDelay = 5;  // Default medium delay (in frames)
    
    switch (level) {
        case AIDifficultyLevel::EASY:
            // Easy: More random actions, slower reactions
            difficulty = 0.25f;
            randomActionProb = 0.3f;
            reactionDelay = 10;
            break;
            
        case AIDifficultyLevel::MEDIUM:
            // Medium: Default settings
            difficulty = 0.5f;
            randomActionProb = 0.1f;
            reactionDelay = 5;
            break;
            
        case AIDifficultyLevel::HARD:
            // Hard: Less random, faster reactions
            difficulty = 0.75f;
            randomActionProb = 0.05f;
            reactionDelay = 2;
            break;
            
        case AIDifficultyLevel::EXPERT:
            // Expert: Minimal randomness, immediate reactions
            difficulty = 1.0f;
            randomActionProb = 0.01f;
            reactionDelay = 0;
            break;
    }
    
    // Apply settings to controller
    controller->setDifficulty(difficulty);
    controller->setRandomActionProbability(randomActionProb);
    controller->setReactionDelay(reactionDelay);
}

void AIMenuSystem::update() {
    // Handle any updates needed each frame
}

void AIMenuSystem::initialize(std::shared_ptr<NeuralAIController> controller) {
    Init(controller);
}

bool AIMenuSystem::drawMainMenu() {
    // This would be implemented with your UI system
    return false;
}

bool AIMenuSystem::drawConfigMenu() {
    // This would be implemented with your UI system
    return false;
}

bool AIMenuSystem::drawModelSelectionMenu(int playerIndex) {
    // This would be implemented with your UI system
    return false;
}

void AIMenuSystem::toggleAIControl(int playerIndex) {
    SetPlayerAIControlled(playerIndex, !IsPlayerAIControlled(playerIndex));
}

void AIMenuSystem::setDifficulty(float level) {
    // Apply to currently selected player
    SetPlayerDifficulty(m_selected_player, level);
}

void AIMenuSystem::setRandomActionProbability(float probability) {
    if (m_controller) {
        m_controller->setRandomActionProbability(probability);
    }
}

void AIMenuSystem::setReactionDelay(int frames) {
    if (m_controller) {
        m_controller->setReactionDelay(frames);
    }
}

bool AIMenuSystem::loadModelForPlayer(const std::string& modelPath, int playerIndex) {
    if (!m_controller || playerIndex < 0 || playerIndex >= MAX_PLAYERS) {
        return false;
    }
    
    // Check if file exists
    try {
        if (modelPath.empty() || !fs::exists(modelPath)) {
            std::cerr << "Model file not found: " << modelPath << std::endl;
            return false;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error checking model file: " << e.what() << std::endl;
        return false;
    }
    
    // Update model path
    m_currentModelPath[playerIndex] = modelPath;
    
    // Try to load the model using the controller
    bool success = m_controller->loadModelForPlayer(modelPath, playerIndex);
    
    if (success) {
        // Call callbacks if registered
        if (m_on_player_model_changed) {
            m_on_player_model_changed(playerIndex, modelPath);
        }
    } else {
        std::cerr << "Failed to load model: " << modelPath << std::endl;
    }
    
    return success;
}

bool AIMenuSystem::isModelLoadedForPlayer(int playerIndex) const {
    if (!m_controller || playerIndex < 0 || playerIndex >= MAX_PLAYERS) {
        return false;
    }
    
    return m_controller->isModelLoadedForPlayer(playerIndex);
}

void AIMenuSystem::scanAvailableModels() {
    m_availableModels.clear();
    
    // Get model files
    std::vector<std::string> modelFiles = ScanModelFiles();
    
    for (const auto& filePath : modelFiles) {
        try {
            // Get model info
            auto info = GetModelInfo(filePath);
            
            AIModelOption option;
            option.filepath = filePath;
            
            // Extract filename
            fs::path path(filePath);
            option.name = path.filename().string();
            
            // Set description based on available info
            if (info.count("game") && info.count("character")) {
                option.description = info["game"] + " - " + info["character"];
                if (info.count("version")) {
                    option.description += " (v" + info["version"] + ")";
                }
            } else {
                option.description = "AI Model";
            }
            
            // Determine if it's a built-in model
            option.isBuiltIn = filePath.find("models/builtin") != std::string::npos;
            
            m_availableModels.push_back(option);
        } catch (const std::exception& e) {
            std::cerr << "Error processing model: " << filePath << " - " << e.what() << std::endl;
        }
    }
}

std::string AIMenuSystem::getLoadedModelName(int playerIndex) const {
    if (playerIndex < 0 || playerIndex >= MAX_PLAYERS || m_currentModelPath[playerIndex].empty()) {
        return "None";
    }
    
    try {
        fs::path path(m_currentModelPath[playerIndex]);
        return path.filename().string();
    } catch (const std::exception& e) {
        std::cerr << "Error getting model name: " << e.what() << std::endl;
        return "Unknown";
    }
}

bool AIMenuSystem::isControllingPlayer(int playerIndex) const {
    return IsPlayerAIControlled(playerIndex);
}

void AIMenuSystem::UpdatePlayerSettings(int player_index) {
    if (player_index < 0 || player_index >= MAX_PLAYERS) {
        return;
    }
    
    // Update the PlayerSettings map
    PlayerSettings settings;
    settings.ai_controlled = m_aiEnabled[player_index];
    settings.model_file = m_currentModelPath[player_index];
    
    // Map difficulty enum to float
    switch (m_difficultyLevel[player_index]) {
        case AIDifficultyLevel::EASY:
            settings.difficulty = 0.0f;
            break;
        case AIDifficultyLevel::MEDIUM:
            settings.difficulty = 0.33f;
            break;
        case AIDifficultyLevel::HARD:
            settings.difficulty = 0.66f;
            break;
        case AIDifficultyLevel::EXPERT:
            settings.difficulty = 1.0f;
            break;
        default:
            settings.difficulty = 0.5f;
            break;
    }
    
    m_player_settings[player_index] = settings;
}

bool AIMenuSystem::RenderFileDialog() {
    // This would be implemented with your UI system
    return false;
}

void AIMenuSystem::ApplyPlayerSettings(int player_index) {
    if (player_index < 0 || player_index >= MAX_PLAYERS || !m_controller) {
        return;
    }
    
    // Set AI control
    m_controller->setControllingPlayer(player_index, m_aiEnabled[player_index]);
    
    // Load model if AI is enabled and model path is valid
    if (m_aiEnabled[player_index] && !m_currentModelPath[player_index].empty()) {
        try {
            if (fs::exists(m_currentModelPath[player_index])) {
                m_controller->loadModelForPlayer(m_currentModelPath[player_index], player_index);
            }
        } catch (const std::exception& e) {
            std::cerr << "Error applying model: " << e.what() << std::endl;
        }
    }
    
    // Apply difficulty settings
    applyDifficultySettings(m_controller, m_difficultyLevel[player_index]);
}

void AIMenuSystem::loadDefaultModels() {
    // Check for default models in the built-in directory
    std::string builtinModelDir = "models/builtin";
    try {
        if (fs::exists(builtinModelDir) && fs::is_directory(builtinModelDir)) {
            std::vector<std::string> builtinModels = ScanModelFiles(builtinModelDir);
            
            // Find a default model for each supported game
            // For example, look for specific game identifiers in filenames
            for (const auto& modelPath : builtinModels) {
                // Extract filename and check for game identifiers
                fs::path path(modelPath);
                std::string filename = path.filename().string();
                std::string basename = path.stem().string();
                
                // Add to available models
                AIModelOption option;
                option.filepath = modelPath;
                option.name = filename;
                option.description = "Built-in AI Model";
                option.isBuiltIn = true;
                
                m_availableModels.push_back(option);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error loading default models: " << e.what() << std::endl;
    }
}

void AIMenuSystem::scanUserModelDirectory() {
    // Scan for user models
    std::string userModelDir = "models/user";
    try {
        if (fs::exists(userModelDir) && fs::is_directory(userModelDir)) {
            std::vector<std::string> userModels = ScanModelFiles(userModelDir);
            
            for (const auto& modelPath : userModels) {
                // Extract filename
                fs::path path(modelPath);
                std::string filename = path.filename().string();
                
                // Add to available models
                AIModelOption option;
                option.filepath = modelPath;
                option.name = filename;
                option.description = "User AI Model";
                option.isBuiltIn = false;
                
                m_availableModels.push_back(option);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error scanning user models: " << e.what() << std::endl;
    }
}

std::string AIMenuSystem::getModelDirectory() const {
    // Check for models directory in various locations
    std::vector<std::string> possibleDirs = {
        "models",
        "../models",
        "../../models",
        "./models"
    };
    
    for (const auto& dir : possibleDirs) {
        try {
            if (fs::exists(dir) && fs::is_directory(dir)) {
                return dir;
            }
        } catch (const std::exception& e) {
            std::cerr << "Error checking directory: " << dir << " - " << e.what() << std::endl;
        }
    }
    
    // Default to current directory
    return "models";
}

void AIMenuSystem::BuildMenu(const char* pszTitle, bool bSelectOnly) {
    // This would be implemented with the appropriate menu system
    // Placeholder implementation
}

void AIMenuSystem::HandleMenuSelect(int nVal) {
    // This would be implemented with the appropriate menu system
    // Placeholder implementation
}

} // namespace AI

// C interface implementations
extern "C" {

int AIMenuSystemInit() {
    try {
        if (g_pAIMenuSystem) {
            delete g_pAIMenuSystem;
        }
        g_pAIMenuSystem = new AI::AIMenuSystem();
        return 0;
    } catch (...) {
        return -1;
    }
}

int AIMenuSystemExit() {
    try {
        if (g_pAIMenuSystem) {
            delete g_pAIMenuSystem;
            g_pAIMenuSystem = nullptr;
        }
        return 0;
    } catch (...) {
        return -1;
    }
}

void AIMenuSystemBuildMenu(const char* pszTitle, bool bSelectOnly) {
    if (g_pAIMenuSystem) {
        g_pAIMenuSystem->BuildMenu(pszTitle, bSelectOnly);
    }
}

void AIMenuSystemHandleMenuSelect(int nVal) {
    if (g_pAIMenuSystem) {
        g_pAIMenuSystem->HandleMenuSelect(nVal);
    }
}

} // extern "C" 