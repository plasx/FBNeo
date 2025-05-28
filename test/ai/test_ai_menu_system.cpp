#include <iostream>
#include <cassert>
#include <cstring>
#include "../../src/ai/ai_menu_system.h"
#include "../../src/ai/neural_ai_controller.h"

// Mock NeuralAIController for testing
class MockNeuralAIController : public NeuralAIController {
public:
    MockNeuralAIController() 
        : m_last_player_enabled(-1)
        , m_last_player_disabled(-1)
        , m_last_model_file("")
        , m_last_difficulty_player(-1)
        , m_last_difficulty_value(0.0f)
    {}

    void EnableAIControl(int player_index, const std::string& model_file) override {
        m_last_player_enabled = player_index;
        m_last_model_file = model_file;
        m_ai_controlled_players.insert(player_index);
    }

    void DisableAIControl(int player_index) override {
        m_last_player_disabled = player_index;
        m_ai_controlled_players.erase(player_index);
    }

    void SetDifficulty(int player_index, float difficulty) override {
        m_last_difficulty_player = player_index;
        m_last_difficulty_value = difficulty;
    }

    std::unordered_set<int> GetAIControlledPlayers() const {
        return m_ai_controlled_players;
    }

    int GetLastPlayerEnabled() const { return m_last_player_enabled; }
    int GetLastPlayerDisabled() const { return m_last_player_disabled; }
    std::string GetLastModelFile() const { return m_last_model_file; }
    int GetLastDifficultyPlayer() const { return m_last_difficulty_player; }
    float GetLastDifficultyValue() const { return m_last_difficulty_value; }

private:
    int m_last_player_enabled;
    int m_last_player_disabled;
    std::string m_last_model_file;
    int m_last_difficulty_player;
    float m_last_difficulty_value;
    std::unordered_set<int> m_ai_controlled_players;
};

// Test functions
void test_menu_system_init() {
    MockNeuralAIController controller;
    AIMenuSystem menu;
    
    // Test initialization
    bool init_result = menu.Init(&controller);
    assert(init_result && "Menu should initialize successfully with valid controller");
    
    // Test null controller
    AIMenuSystem menu2;
    bool init_result2 = menu2.Init(nullptr);
    assert(!init_result2 && "Menu should fail to initialize with null controller");
    
    std::cout << "test_menu_system_init: PASSED" << std::endl;
}

void test_player_ai_control() {
    MockNeuralAIController controller;
    AIMenuSystem menu;
    menu.Init(&controller);
    
    // Test initial state
    assert(!menu.IsPlayerAIControlled(0) && "Player should not be AI controlled initially");
    
    // Test setting AI control
    menu.SetPlayerAIControlled(0, true);
    assert(menu.IsPlayerAIControlled(0) && "Player should be AI controlled after setting");
    assert(controller.GetLastPlayerEnabled() == 0 && "Controller should have enabled AI for player 0");
    
    // Test getting AI controlled players
    std::vector<int> ai_players = menu.GetAIControlledPlayers();
    assert(ai_players.size() == 1 && ai_players[0] == 0 && "AI controlled players list should contain only player 0");
    
    // Test disabling AI control
    menu.SetPlayerAIControlled(0, false);
    assert(!menu.IsPlayerAIControlled(0) && "Player should not be AI controlled after disabling");
    assert(controller.GetLastPlayerDisabled() == 0 && "Controller should have disabled AI for player 0");
    
    std::cout << "test_player_ai_control: PASSED" << std::endl;
}

void test_player_model_selection() {
    MockNeuralAIController controller;
    AIMenuSystem menu;
    menu.Init(&controller);
    
    // Create a temporary model file for testing
    const char* test_model = "test_model.pt";
    FILE* fp = fopen(test_model, "w");
    if (fp) {
        fprintf(fp, "TEST MODEL DATA");
        fclose(fp);
    } else {
        std::cerr << "Warning: Could not create test model file" << std::endl;
        return;
    }
    
    // Test setting model file
    bool result = menu.SetPlayerModelFile(0, test_model);
    assert(result && "Should successfully set valid model file");
    assert(menu.GetPlayerModelFile(0) == test_model && "Model file should match what was set");
    
    // Test activating AI with model
    menu.SetPlayerAIControlled(0, true);
    assert(controller.GetLastModelFile() == test_model && "Controller should use the correct model file");
    
    // Clean up
    remove(test_model);
    
    std::cout << "test_player_model_selection: PASSED" << std::endl;
}

void test_player_difficulty() {
    MockNeuralAIController controller;
    AIMenuSystem menu;
    menu.Init(&controller);
    
    // Test default difficulty
    float default_difficulty = menu.GetPlayerDifficulty(0);
    assert(default_difficulty >= 0.0f && default_difficulty <= 1.0f && "Default difficulty should be between 0 and 1");
    
    // Test setting difficulty
    menu.SetPlayerDifficulty(0, 0.75f);
    assert(menu.GetPlayerDifficulty(0) == 0.75f && "Difficulty should match what was set");
    
    // Test clamping
    menu.SetPlayerDifficulty(0, 1.5f);
    assert(menu.GetPlayerDifficulty(0) == 1.0f && "Difficulty should be clamped to 1.0");
    
    menu.SetPlayerDifficulty(0, -0.5f);
    assert(menu.GetPlayerDifficulty(0) == 0.0f && "Difficulty should be clamped to 0.0");
    
    // Test difficulty propagation to controller
    menu.SetPlayerAIControlled(0, true);
    menu.SetPlayerDifficulty(0, 0.6f);
    assert(controller.GetLastDifficultyPlayer() == 0 && "Controller should set difficulty for the right player");
    assert(controller.GetLastDifficultyValue() == 0.6f && "Controller should set the correct difficulty value");
    
    std::cout << "test_player_difficulty: PASSED" << std::endl;
}

void test_menu_visibility() {
    AIMenuSystem menu;
    
    // Test initial visibility
    assert(!menu.IsMenuVisible() && "Menu should be hidden initially");
    
    // Test showing menu
    menu.ShowAIMenu();
    assert(menu.IsMenuVisible() && "Menu should be visible after showing");
    
    // Test hiding menu
    menu.HideAIMenu();
    assert(!menu.IsMenuVisible() && "Menu should be hidden after hiding");
    
    std::cout << "test_menu_visibility: PASSED" << std::endl;
}

void test_menu_state_json() {
    MockNeuralAIController controller;
    AIMenuSystem menu;
    menu.Init(&controller);
    
    // Setup some state
    menu.SetPlayerAIControlled(0, true);
    menu.SetPlayerModelFile(0, "test_model.pt");
    menu.SetPlayerDifficulty(0, 0.7f);
    menu.ShowAIMenu();
    
    // Test saving state
    std::string json = menu.GetMenuStateJson();
    assert(!json.empty() && "JSON state should not be empty");
    
    // Reset menu
    menu.SetPlayerAIControlled(0, false);
    menu.SetPlayerModelFile(0, "");
    menu.SetPlayerDifficulty(0, 0.5f);
    menu.HideAIMenu();
    
    // Test loading state
    bool load_result = menu.LoadMenuStateJson(json);
    assert(load_result && "Should successfully load valid JSON");
    assert(menu.IsPlayerAIControlled(0) && "Player AI control should be restored");
    assert(menu.GetPlayerModelFile(0) == "test_model.pt" && "Model file should be restored");
    assert(menu.GetPlayerDifficulty(0) == 0.7f && "Difficulty should be restored");
    assert(menu.IsMenuVisible() && "Menu visibility should be restored");
    
    std::cout << "test_menu_state_json: PASSED" << std::endl;
}

int main() {
    std::cout << "Running AIMenuSystem tests..." << std::endl;
    
    test_menu_system_init();
    test_player_ai_control();
    test_player_model_selection();
    test_player_difficulty();
    test_menu_visibility();
    test_menu_state_json();
    
    std::cout << "All tests PASSED" << std::endl;
    return 0;
} 