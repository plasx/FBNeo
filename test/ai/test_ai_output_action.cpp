#include "../../src/ai/ai_output_action.h"
#include <cassert>
#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Mock for the input mapping constants
enum class ButtonType {
    UP = 0,
    DOWN,
    LEFT,
    RIGHT,
    PUNCH,
    KICK,
    SPECIAL,
    START
};

void test_output_action_creation() {
    std::cout << "Testing AIOutputAction creation... ";
    
    AIOutputAction action;
    
    // Check initial values
    assert(action.getFrameNumber() == 0);
    assert(action.getPlayerIndex() == 0);
    assert(action.getGameTick() == 0);
    
    // Check that no buttons are pressed
    std::vector<std::string> commonButtons = {"up", "down", "left", "right", "punch", "kick", "start"};
    for (const auto& button : commonButtons) {
        assert(action.getButtonState(button) == false);
    }
    
    std::cout << "PASSED" << std::endl;
}

void test_output_action_setters() {
    std::cout << "Testing AIOutputAction setters... ";
    
    AIOutputAction action;
    
    // Set values
    action.setFrameNumber(42);
    action.setPlayerIndex(1);
    action.setGameTick(123);
    
    // Check values
    assert(action.getFrameNumber() == 42);
    assert(action.getPlayerIndex() == 1);
    assert(action.getGameTick() == 123);
    
    std::cout << "PASSED" << std::endl;
}

void test_output_action_button_states() {
    std::cout << "Testing AIOutputAction button states... ";
    
    AIOutputAction action;
    
    // Set button states
    action.setButtonState("up", true);
    action.setButtonState("down", false);
    action.setButtonState("left", true);
    action.setButtonState("right", false);
    action.setButtonState("punch", true);
    action.setButtonState("kick", false);
    action.setButtonState("special", true);
    
    // Check button states
    assert(action.getButtonState("up") == true);
    assert(action.getButtonState("down") == false);
    assert(action.getButtonState("left") == true);
    assert(action.getButtonState("right") == false);
    assert(action.getButtonState("punch") == true);
    assert(action.getButtonState("kick") == false);
    assert(action.getButtonState("special") == true);
    
    // Check non-existent button
    assert(action.getButtonState("not_a_button") == false);
    
    std::cout << "PASSED" << std::endl;
}

void test_output_action_toggle_button() {
    std::cout << "Testing AIOutputAction toggle button... ";
    
    AIOutputAction action;
    
    // Set initial button state
    action.setButtonState("punch", false);
    assert(action.getButtonState("punch") == false);
    
    // Toggle button
    action.toggleButtonState("punch");
    assert(action.getButtonState("punch") == true);
    
    // Toggle again
    action.toggleButtonState("punch");
    assert(action.getButtonState("punch") == false);
    
    // Toggle non-existent button (should create it and set to true)
    action.toggleButtonState("new_button");
    assert(action.getButtonState("new_button") == true);
    
    std::cout << "PASSED" << std::endl;
}

void test_output_action_serialize() {
    std::cout << "Testing AIOutputAction serialization... ";
    
    AIOutputAction action;
    action.setFrameNumber(42);
    action.setPlayerIndex(1);
    action.setGameTick(123);
    action.setButtonState("up", true);
    action.setButtonState("punch", true);
    
    // Serialize to JSON
    json j = action.toJSON();
    
    // Check JSON values
    assert(j["frame_number"] == 42);
    assert(j["player_index"] == 1);
    assert(j["game_tick"] == 123);
    assert(j["button_states"]["up"] == true);
    assert(j["button_states"]["punch"] == true);
    assert(j["button_states"].find("down") == j["button_states"].end() || j["button_states"]["down"] == false);
    
    std::cout << "PASSED" << std::endl;
}

void test_output_action_deserialize() {
    std::cout << "Testing AIOutputAction deserialization... ";
    
    // Create JSON
    json j = {
        {"frame_number", 42},
        {"player_index", 1},
        {"game_tick", 123},
        {"button_states", {
            {"up", true},
            {"down", false},
            {"left", false},
            {"right", true},
            {"punch", true},
            {"kick", false}
        }}
    };
    
    // Create action from JSON
    AIOutputAction action = AIOutputAction::fromJSON(j);
    
    // Check values
    assert(action.getFrameNumber() == 42);
    assert(action.getPlayerIndex() == 1);
    assert(action.getGameTick() == 123);
    assert(action.getButtonState("up") == true);
    assert(action.getButtonState("down") == false);
    assert(action.getButtonState("left") == false);
    assert(action.getButtonState("right") == true);
    assert(action.getButtonState("punch") == true);
    assert(action.getButtonState("kick") == false);
    
    std::cout << "PASSED" << std::endl;
}

void test_output_action_clear_buttons() {
    std::cout << "Testing AIOutputAction clear buttons... ";
    
    AIOutputAction action;
    
    // Set button states
    action.setButtonState("up", true);
    action.setButtonState("down", true);
    action.setButtonState("punch", true);
    
    // Check that buttons are set
    assert(action.getButtonState("up") == true);
    assert(action.getButtonState("down") == true);
    assert(action.getButtonState("punch") == true);
    
    // Clear buttons
    action.clearAllButtons();
    
    // Check that buttons are cleared
    assert(action.getButtonState("up") == false);
    assert(action.getButtonState("down") == false);
    assert(action.getButtonState("punch") == false);
    
    std::cout << "PASSED" << std::endl;
}

void test_output_action_copy() {
    std::cout << "Testing AIOutputAction copy... ";
    
    // Create and set up original action
    AIOutputAction original;
    original.setFrameNumber(42);
    original.setPlayerIndex(1);
    original.setGameTick(123);
    original.setButtonState("up", true);
    original.setButtonState("punch", true);
    
    // Create a copy
    AIOutputAction copy = original;
    
    // Check values
    assert(copy.getFrameNumber() == 42);
    assert(copy.getPlayerIndex() == 1);
    assert(copy.getGameTick() == 123);
    assert(copy.getButtonState("up") == true);
    assert(copy.getButtonState("punch") == true);
    
    // Modify the copy
    copy.setFrameNumber(43);
    copy.setButtonState("up", false);
    copy.setButtonState("kick", true);
    
    // Check that original is unchanged
    assert(original.getFrameNumber() == 42);
    assert(original.getButtonState("up") == true);
    assert(original.getButtonState("kick") == false);
    
    // Check that copy has new values
    assert(copy.getFrameNumber() == 43);
    assert(copy.getButtonState("up") == false);
    assert(copy.getButtonState("kick") == true);
    
    std::cout << "PASSED" << std::endl;
}

void test_output_action_get_all_buttons() {
    std::cout << "Testing AIOutputAction get all buttons... ";
    
    AIOutputAction action;
    
    // Set button states
    action.setButtonState("up", true);
    action.setButtonState("down", false);
    action.setButtonState("left", true);
    action.setButtonState("punch", true);
    
    // Get all buttons
    auto buttons = action.getAllButtons();
    
    // Check that all buttons are present
    assert(buttons.size() == 4);
    assert(buttons.find("up") != buttons.end() && buttons["up"] == true);
    assert(buttons.find("down") != buttons.end() && buttons["down"] == false);
    assert(buttons.find("left") != buttons.end() && buttons["left"] == true);
    assert(buttons.find("punch") != buttons.end() && buttons["punch"] == true);
    
    // Check that non-existent buttons are not present
    assert(buttons.find("right") == buttons.end());
    assert(buttons.find("kick") == buttons.end());
    
    std::cout << "PASSED" << std::endl;
}

void test_output_action_equality() {
    std::cout << "Testing AIOutputAction equality... ";
    
    // Create two identical actions
    AIOutputAction action1, action2;
    action1.setFrameNumber(42);
    action1.setPlayerIndex(1);
    action1.setGameTick(123);
    action1.setButtonState("up", true);
    action1.setButtonState("punch", true);
    
    action2.setFrameNumber(42);
    action2.setPlayerIndex(1);
    action2.setGameTick(123);
    action2.setButtonState("up", true);
    action2.setButtonState("punch", true);
    
    // Test equality
    assert(action1 == action2);
    
    // Modify one action
    action2.setButtonState("up", false);
    
    // Test inequality
    assert(action1 != action2);
    
    std::cout << "PASSED" << std::endl;
}

void test_output_action_hash() {
    std::cout << "Testing AIOutputAction hash... ";
    
    // Create two identical actions
    AIOutputAction action1, action2;
    action1.setFrameNumber(42);
    action1.setPlayerIndex(1);
    action1.setGameTick(123);
    action1.setButtonState("up", true);
    action1.setButtonState("punch", true);
    
    action2.setFrameNumber(42);
    action2.setPlayerIndex(1);
    action2.setGameTick(123);
    action2.setButtonState("up", true);
    action2.setButtonState("punch", true);
    
    // Test hash equality
    assert(action1.hash() == action2.hash());
    
    // Modify one action
    action2.setButtonState("up", false);
    
    // Test hash inequality
    assert(action1.hash() != action2.hash());
    
    std::cout << "PASSED" << std::endl;
}

// Test action construction and basic properties
void test_action_construction() {
    std::cout << "Testing AIOutputAction construction... ";
    
    // Test default constructor
    AIOutputAction emptyAction;
    assert(emptyAction.getFrameNumber() == 0);
    assert(emptyAction.getPlayerIndex() == 0);
    assert(emptyAction.getGameId().empty());
    
    // Test constructor with frame number and player index
    AIOutputAction action(42, 1);
    assert(action.getFrameNumber() == 42);
    assert(action.getPlayerIndex() == 1);
    assert(action.getGameId().empty());
    
    // Test setting and getting game ID
    action.setGameId("test_game");
    assert(action.getGameId() == "test_game");
    
    // Test frame number modification
    action.setFrameNumber(100);
    assert(action.getFrameNumber() == 100);
    
    // Test player index modification
    action.setPlayerIndex(2);
    assert(action.getPlayerIndex() == 2);
    
    std::cout << "PASSED" << std::endl;
}

// Test button state operations
void test_button_states() {
    std::cout << "Testing AIOutputAction button states... ";
    
    AIOutputAction action(1, 0);
    
    // Check that all buttons are initially not pressed
    assert(!action.getButtonState(AIOutputAction::Button::Up));
    assert(!action.getButtonState(AIOutputAction::Button::Down));
    assert(!action.getButtonState(AIOutputAction::Button::Left));
    assert(!action.getButtonState(AIOutputAction::Button::Right));
    assert(!action.getButtonState(AIOutputAction::Button::Button1));
    assert(!action.getButtonState(AIOutputAction::Button::Button2));
    assert(!action.getButtonState(AIOutputAction::Button::Button3));
    assert(!action.getButtonState(AIOutputAction::Button::Button4));
    assert(!action.getButtonState(AIOutputAction::Button::Button5));
    assert(!action.getButtonState(AIOutputAction::Button::Button6));
    assert(!action.getButtonState(AIOutputAction::Button::Start));
    assert(!action.getButtonState(AIOutputAction::Button::Coin));
    
    // Set some button states
    action.setButtonState(AIOutputAction::Button::Up, true);
    action.setButtonState(AIOutputAction::Button::Button1, true);
    action.setButtonState(AIOutputAction::Button::Button3, true);
    
    // Verify the set states
    assert(action.getButtonState(AIOutputAction::Button::Up));
    assert(!action.getButtonState(AIOutputAction::Button::Down));  // Not set
    assert(!action.getButtonState(AIOutputAction::Button::Left));  // Not set
    assert(!action.getButtonState(AIOutputAction::Button::Right)); // Not set
    assert(action.getButtonState(AIOutputAction::Button::Button1));
    assert(!action.getButtonState(AIOutputAction::Button::Button2)); // Not set
    assert(action.getButtonState(AIOutputAction::Button::Button3));
    assert(!action.getButtonState(AIOutputAction::Button::Button4)); // Not set
    
    // Toggle some states
    action.setButtonState(AIOutputAction::Button::Up, false);    // Turn off
    action.setButtonState(AIOutputAction::Button::Down, true);   // Turn on
    action.setButtonState(AIOutputAction::Button::Button1, true); // No change (already on)
    
    // Verify the updated states
    assert(!action.getButtonState(AIOutputAction::Button::Up));
    assert(action.getButtonState(AIOutputAction::Button::Down));
    assert(action.getButtonState(AIOutputAction::Button::Button1));
    assert(action.getButtonState(AIOutputAction::Button::Button3));
    
    std::cout << "PASSED" << std::endl;
}

// Test button bitmask operations
void test_button_bitmask() {
    std::cout << "Testing AIOutputAction button bitmask... ";
    
    AIOutputAction action(1, 0);
    
    // Initially all buttons are off, so bitmask should be 0
    assert(action.getButtonBitmask() == 0);
    
    // Set some buttons
    action.setButtonState(AIOutputAction::Button::Up, true);      // Bit 0
    action.setButtonState(AIOutputAction::Button::Button1, true); // Bit 4
    
    // Calculate expected bitmask
    uint16_t expectedBitmask = (1 << static_cast<int>(AIOutputAction::Button::Up)) |
                               (1 << static_cast<int>(AIOutputAction::Button::Button1));
    
    // Verify bitmask
    assert(action.getButtonBitmask() == expectedBitmask);
    
    // Test setting from bitmask
    AIOutputAction action2(1, 0);
    uint16_t testBitmask = (1 << static_cast<int>(AIOutputAction::Button::Down)) |
                           (1 << static_cast<int>(AIOutputAction::Button::Right)) |
                           (1 << static_cast<int>(AIOutputAction::Button::Button2));
    
    action2.setButtonsFromBitmask(testBitmask);
    
    // Verify buttons were set correctly
    assert(!action2.getButtonState(AIOutputAction::Button::Up));
    assert(action2.getButtonState(AIOutputAction::Button::Down));
    assert(!action2.getButtonState(AIOutputAction::Button::Left));
    assert(action2.getButtonState(AIOutputAction::Button::Right));
    assert(!action2.getButtonState(AIOutputAction::Button::Button1));
    assert(action2.getButtonState(AIOutputAction::Button::Button2));
    assert(!action2.getButtonState(AIOutputAction::Button::Button3));
    
    // Verify that the bitmask matches the expected one
    assert(action2.getButtonBitmask() == testBitmask);
    
    std::cout << "PASSED" << std::endl;
}

// Test JSON serialization and deserialization
void test_json_serialization() {
    std::cout << "Testing AIOutputAction JSON serialization... ";
    
    // Create an action with data
    AIOutputAction originalAction(42, 1);
    originalAction.setGameId("test_game");
    
    // Set some button states
    originalAction.setButtonState(AIOutputAction::Button::Up, true);
    originalAction.setButtonState(AIOutputAction::Button::Button1, true);
    originalAction.setButtonState(AIOutputAction::Button::Button3, true);
    
    // Serialize to JSON string
    std::string jsonStr = originalAction.toJson();
    
    // Create a new action from the JSON
    AIOutputAction deserializedAction;
    bool loadResult = deserializedAction.fromJson(jsonStr);
    assert(loadResult);
    
    // Verify deserialized data
    assert(deserializedAction.getFrameNumber() == 42);
    assert(deserializedAction.getPlayerIndex() == 1);
    assert(deserializedAction.getGameId() == "test_game");
    
    // Verify button states
    assert(deserializedAction.getButtonState(AIOutputAction::Button::Up));
    assert(!deserializedAction.getButtonState(AIOutputAction::Button::Down));
    assert(!deserializedAction.getButtonState(AIOutputAction::Button::Left));
    assert(!deserializedAction.getButtonState(AIOutputAction::Button::Right));
    assert(deserializedAction.getButtonState(AIOutputAction::Button::Button1));
    assert(!deserializedAction.getButtonState(AIOutputAction::Button::Button2));
    assert(deserializedAction.getButtonState(AIOutputAction::Button::Button3));
    assert(!deserializedAction.getButtonState(AIOutputAction::Button::Button4));
    
    // Test with invalid JSON
    AIOutputAction invalidAction;
    bool invalidResult = invalidAction.fromJson("{invalid json}");
    assert(!invalidResult);
    
    // Test with valid JSON but missing fields
    bool incompleteResult = invalidAction.fromJson(R"({"game_id": "test"})");
    assert(incompleteResult);  // Should still succeed but with default values
    assert(invalidAction.getGameId() == "test");
    assert(invalidAction.getFrameNumber() == 0);  // Default value
    assert(invalidAction.getPlayerIndex() == 0);  // Default value
    
    std::cout << "PASSED" << std::endl;
}

// Test action comparison
void test_action_comparison() {
    std::cout << "Testing AIOutputAction comparison... ";
    
    // Create two identical actions
    AIOutputAction action1(1, 0);
    action1.setGameId("test_game");
    action1.setButtonState(AIOutputAction::Button::Up, true);
    action1.setButtonState(AIOutputAction::Button::Button1, true);
    
    AIOutputAction action2(1, 0);
    action2.setGameId("test_game");
    action2.setButtonState(AIOutputAction::Button::Up, true);
    action2.setButtonState(AIOutputAction::Button::Button1, true);
    
    // Create an action with different button states
    AIOutputAction action3(1, 0);
    action3.setGameId("test_game");
    action3.setButtonState(AIOutputAction::Button::Down, true);
    action3.setButtonState(AIOutputAction::Button::Button1, true);
    
    // Test equality
    assert(action1.equals(action2));
    assert(!action1.equals(action3));
    
    // Create an action with different frame number
    AIOutputAction action4(2, 0);
    action4.setGameId("test_game");
    action4.setButtonState(AIOutputAction::Button::Up, true);
    action4.setButtonState(AIOutputAction::Button::Button1, true);
    
    assert(!action1.equals(action4));
    
    // Create an action with different player index
    AIOutputAction action5(1, 1);
    action5.setGameId("test_game");
    action5.setButtonState(AIOutputAction::Button::Up, true);
    action5.setButtonState(AIOutputAction::Button::Button1, true);
    
    assert(!action1.equals(action5));
    
    std::cout << "PASSED" << std::endl;
}

// Test action copying
void test_action_copying() {
    std::cout << "Testing AIOutputAction copying... ";
    
    // Create an action with data
    AIOutputAction originalAction(42, 1);
    originalAction.setGameId("test_game");
    originalAction.setButtonState(AIOutputAction::Button::Up, true);
    originalAction.setButtonState(AIOutputAction::Button::Button1, true);
    
    // Test copy constructor
    AIOutputAction copiedAction(originalAction);
    
    assert(copiedAction.getFrameNumber() == 42);
    assert(copiedAction.getPlayerIndex() == 1);
    assert(copiedAction.getGameId() == "test_game");
    assert(copiedAction.getButtonState(AIOutputAction::Button::Up));
    assert(copiedAction.getButtonState(AIOutputAction::Button::Button1));
    assert(!copiedAction.getButtonState(AIOutputAction::Button::Down));
    
    // Modify original and ensure copy is not affected
    originalAction.setFrameNumber(43);
    originalAction.setButtonState(AIOutputAction::Button::Up, false);
    originalAction.setButtonState(AIOutputAction::Button::Down, true);
    
    assert(copiedAction.getFrameNumber() == 42);  // Unchanged
    assert(copiedAction.getButtonState(AIOutputAction::Button::Up));  // Unchanged
    assert(!copiedAction.getButtonState(AIOutputAction::Button::Down));  // Unchanged
    
    // Test assignment operator
    AIOutputAction assignedAction;
    assignedAction = originalAction;
    
    assert(assignedAction.getFrameNumber() == 43);
    assert(assignedAction.getPlayerIndex() == 1);
    assert(assignedAction.getGameId() == "test_game");
    assert(!assignedAction.getButtonState(AIOutputAction::Button::Up));
    assert(assignedAction.getButtonState(AIOutputAction::Button::Down));
    assert(assignedAction.getButtonState(AIOutputAction::Button::Button1));
    
    std::cout << "PASSED" << std::endl;
}

// Test file I/O operations with actions
void test_action_file_io() {
    std::cout << "Testing AIOutputAction file I/O... ";
    
    // Create a temporary file
    std::string filename = "test_action.jsonl";
    std::ofstream file(filename);
    assert(file.is_open());
    
    // Create and save multiple actions
    AIOutputAction action1(1, 0);
    action1.setGameId("test_game");
    action1.setButtonState(AIOutputAction::Button::Up, true);
    action1.setButtonState(AIOutputAction::Button::Button1, true);
    
    AIOutputAction action2(2, 0);
    action2.setGameId("test_game");
    action2.setButtonState(AIOutputAction::Button::Down, true);
    action2.setButtonState(AIOutputAction::Button::Button2, true);
    
    // Write actions to file
    file << action1.toJson() << std::endl;
    file << action2.toJson() << std::endl;
    file.close();
    
    // Read actions from file
    std::ifstream inputFile(filename);
    assert(inputFile.is_open());
    
    std::string line;
    std::vector<AIOutputAction> loadedActions;
    
    while (std::getline(inputFile, line)) {
        AIOutputAction action;
        bool success = action.fromJson(line);
        assert(success);
        loadedActions.push_back(action);
    }
    
    inputFile.close();
    
    // Verify loaded actions
    assert(loadedActions.size() == 2);
    assert(loadedActions[0].getFrameNumber() == 1);
    assert(loadedActions[1].getFrameNumber() == 2);
    
    assert(loadedActions[0].getButtonState(AIOutputAction::Button::Up));
    assert(loadedActions[0].getButtonState(AIOutputAction::Button::Button1));
    assert(!loadedActions[0].getButtonState(AIOutputAction::Button::Down));
    
    assert(loadedActions[1].getButtonState(AIOutputAction::Button::Down));
    assert(loadedActions[1].getButtonState(AIOutputAction::Button::Button2));
    assert(!loadedActions[1].getButtonState(AIOutputAction::Button::Up));
    
    // Clean up
    std::remove(filename.c_str());
    
    std::cout << "PASSED" << std::endl;
}

// Test button conflict resolution
void test_button_conflicts() {
    std::cout << "Testing AIOutputAction button conflicts... ";
    
    AIOutputAction action(1, 0);
    
    // Test conflicting directional inputs
    action.setButtonState(AIOutputAction::Button::Up, true);
    action.setButtonState(AIOutputAction::Button::Down, true);
    
    // If implemented, the action should resolve this conflict
    // This would typically be game-specific, but a general rule is that
    // opposing directions cannot be pressed simultaneously
    
    // Check if implementation resolves conflicts
    // Note: The actual behavior depends on implementation, modify these assertions as needed
    bool canPressOpposingDirections = action.getButtonState(AIOutputAction::Button::Up) && 
                                     action.getButtonState(AIOutputAction::Button::Down);
    
    // Output information about conflict handling
    if (canPressOpposingDirections) {
        std::cout << "Implementation allows opposing directions to be pressed simultaneously. ";
    } else {
        std::cout << "Implementation prevents opposing directions from being pressed simultaneously. ";
    }
    
    // Test diagonal inputs
    action = AIOutputAction(1, 0);  // Reset action
    action.setButtonState(AIOutputAction::Button::Up, true);
    action.setButtonState(AIOutputAction::Button::Right, true);
    
    // These should be allowed
    assert(action.getButtonState(AIOutputAction::Button::Up));
    assert(action.getButtonState(AIOutputAction::Button::Right));
    
    std::cout << "PASSED" << std::endl;
}

// Test string representation of buttons and actions
void test_button_string_representation() {
    std::cout << "Testing AIOutputAction string representation... ";
    
    AIOutputAction action(1, 0);
    action.setButtonState(AIOutputAction::Button::Up, true);
    action.setButtonState(AIOutputAction::Button::Button1, true);
    action.setButtonState(AIOutputAction::Button::Button3, true);
    
    // Test string conversion for buttons
    // This will depend on the implementation; update these assertions based on your implementation
    std::string actionStr = action.toString();
    
    // The string should contain references to the pressed buttons
    assert(actionStr.find("Up") != std::string::npos);
    assert(actionStr.find("Button1") != std::string::npos);
    assert(actionStr.find("Button3") != std::string::npos);
    
    // It should not contain references to unpressed buttons
    assert(actionStr.find("Down") == std::string::npos);
    assert(actionStr.find("Button2") == std::string::npos);
    
    std::cout << "PASSED" << std::endl;
}

int main() {
    std::cout << "==== AIOutputAction Tests ====" << std::endl;
    
    test_output_action_creation();
    test_output_action_setters();
    test_output_action_button_states();
    test_output_action_toggle_button();
    test_output_action_serialize();
    test_output_action_deserialize();
    test_output_action_clear_buttons();
    test_output_action_copy();
    test_output_action_get_all_buttons();
    test_output_action_equality();
    test_output_action_hash();
    
    test_action_construction();
    test_button_states();
    test_button_bitmask();
    test_json_serialization();
    test_action_comparison();
    test_action_copying();
    test_action_file_io();
    test_button_conflicts();
    test_button_string_representation();
    
    std::cout << "All tests PASSED!" << std::endl;
    return 0;
} 