#include "../../src/ai/neural_ai_controller.h"
#include "../../src/ai/ai_input_frame.h"
#include "../../src/ai/ai_output_action.h"
#include "../../src/ai/ai_memory_mapping.h"
#include <cassert>
#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include <thread>
#include <chrono>

// Forward declarations of test helper functions
class MockAIPolicy : public AIPolicy {
private:
    bool modelLoaded = false;
    std::string modelPath;
    int inferenceCount = 0;

public:
    MockAIPolicy() = default;
    ~MockAIPolicy() override = default;

    bool loadModel(const std::string& path) override {
        modelPath = path;
        modelLoaded = true;
        return true;
    }

    bool isModelLoaded() const override {
        return modelLoaded;
    }

    AIOutputAction inferAction(const AIInputFrame& inputFrame, int playerIndex) override {
        inferenceCount++;
        
        // Create a simple output action based on the input frame
        AIOutputAction action(inputFrame.getFrameNumber(), playerIndex);
        
        // Set some buttons based on frame number for testing
        action.setButtonState(AIOutputAction::Button::Up, inputFrame.getFrameNumber() % 2 == 0);
        action.setButtonState(AIOutputAction::Button::Down, inputFrame.getFrameNumber() % 2 == 1);
        action.setButtonState(AIOutputAction::Button::Left, inputFrame.getFrameNumber() % 3 == 0);
        action.setButtonState(AIOutputAction::Button::Right, inputFrame.getFrameNumber() % 3 == 1);
        action.setButtonState(AIOutputAction::Button::Button1, inputFrame.getFrameNumber() % 5 == 0);
        
        return action;
    }

    int getInferenceCount() const {
        return inferenceCount;
    }

    void resetInferenceCount() {
        inferenceCount = 0;
    }
};

class MockAIMemoryMapping : public AIMemoryMapping {
private:
    std::string gameId;
    int frameCount = 0;
    bool validExtraction = true;

public:
    explicit MockAIMemoryMapping(const std::string& gameId) : gameId(gameId) {}
    ~MockAIMemoryMapping() override = default;

    bool extractFromMemory(AIInputFrame& frame) override {
        if (!validExtraction) {
            return false;
        }
        
        frameCount++;
        
        // Populate the frame with mock data
        frame.setGameId(gameId);
        frame.setFrameNumber(frameCount);
        
        // Add player 1 data
        frame.addPlayerValue(0, "p1_health", 100.0f - frameCount * 0.5f);
        frame.addPlayerValue(0, "p1_x_pos", 150.0f + (frameCount % 10) * 5.0f);
        frame.addPlayerValue(0, "p1_y_pos", 200.0f);
        frame.addPlayerValue(0, "p1_state", 1.0f);
        
        // Add player 2 data
        frame.addPlayerValue(1, "p2_health", 90.0f - frameCount * 0.3f);
        frame.addPlayerValue(1, "p2_x_pos", 300.0f - (frameCount % 10) * 5.0f);
        frame.addPlayerValue(1, "p2_y_pos", 200.0f);
        frame.addPlayerValue(1, "p2_state", 2.0f);
        
        // Add some feature values
        frame.addFeatureValue("round_timer", 60.0f - frameCount * 0.1f);
        frame.addFeatureValue("stage_id", 3.0f);
        
        // Add frame hash
        frame.setHash(std::to_string(frameCount * 12345));
        
        return true;
    }
    
    void setValidExtraction(bool valid) {
        validExtraction = valid;
    }
    
    int getFrameCount() const {
        return frameCount;
    }
    
    void resetFrameCount() {
        frameCount = 0;
    }
};

// Test initialization and configuration
void test_controller_initialization() {
    std::cout << "Testing NeuralAIController initialization... ";
    
    // Create mock policy and mapping
    auto mockPolicy = std::make_shared<MockAIPolicy>();
    auto mockMapping = std::make_shared<MockAIMemoryMapping>("sf3");
    
    // Create controller
    NeuralAIController controller;
    
    // Configure controller
    controller.setPolicy(mockPolicy);
    controller.setMemoryMapping(mockMapping);
    controller.setPlayerIndex(1);
    
    // Check if configuration was successful
    assert(controller.isConfigured());
    
    std::cout << "PASSED" << std::endl;
}

// Test controller update and action generation
void test_controller_update() {
    std::cout << "Testing NeuralAIController update... ";
    
    // Create mock policy and mapping
    auto mockPolicy = std::make_shared<MockAIPolicy>();
    auto mockMapping = std::make_shared<MockAIMemoryMapping>("sf3");
    
    // Create controller
    NeuralAIController controller;
    
    // Configure controller
    controller.setPolicy(mockPolicy);
    controller.setMemoryMapping(mockMapping);
    controller.setPlayerIndex(1);
    
    // Perform an update
    bool updateResult = controller.update();
    assert(updateResult);
    
    // Check that the policy performed inference
    assert(mockPolicy->getInferenceCount() == 1);
    
    // Check that the controller produced an action
    const AIOutputAction& lastAction = controller.getLastAction();
    assert(lastAction.getFrameNumber() == 1);
    assert(lastAction.getPlayerIndex() == 1);
    
    std::cout << "PASSED" << std::endl;
}

// Test controller button state
void test_controller_button_state() {
    std::cout << "Testing NeuralAIController button state... ";
    
    // Create mock policy and mapping
    auto mockPolicy = std::make_shared<MockAIPolicy>();
    auto mockMapping = std::make_shared<MockAIMemoryMapping>("sf3");
    
    // Create controller
    NeuralAIController controller;
    
    // Configure controller
    controller.setPolicy(mockPolicy);
    controller.setMemoryMapping(mockMapping);
    controller.setPlayerIndex(1);
    
    // Perform an update
    bool updateResult = controller.update();
    assert(updateResult);
    
    // Check button states using the getButton method
    // Button states are determined by the MockAIPolicy
    assert(controller.getButton(AIOutputAction::Button::Up) == (1 % 2 == 0));
    assert(controller.getButton(AIOutputAction::Button::Down) == (1 % 2 == 1));
    assert(controller.getButton(AIOutputAction::Button::Left) == (1 % 3 == 0));
    assert(controller.getButton(AIOutputAction::Button::Right) == (1 % 3 == 1));
    assert(controller.getButton(AIOutputAction::Button::Button1) == (1 % 5 == 0));
    
    std::cout << "PASSED" << std::endl;
}

// Test controller error handling
void test_controller_error_handling() {
    std::cout << "Testing NeuralAIController error handling... ";
    
    // Create mock policy and mapping
    auto mockPolicy = std::make_shared<MockAIPolicy>();
    auto mockMapping = std::make_shared<MockAIMemoryMapping>("sf3");
    
    // Create controller
    NeuralAIController controller;
    
    // Test update without configuration
    bool updateWithoutConfig = controller.update();
    assert(!updateWithoutConfig);
    
    // Configure controller partially
    controller.setPolicy(mockPolicy);
    bool updateWithPartialConfig = controller.update();
    assert(!updateWithPartialConfig);
    
    // Configure controller completely
    controller.setMemoryMapping(mockMapping);
    controller.setPlayerIndex(1);
    
    // Test with valid extraction
    bool updateWithValidExtraction = controller.update();
    assert(updateWithValidExtraction);
    
    // Test with invalid extraction
    mockMapping->setValidExtraction(false);
    bool updateWithInvalidExtraction = controller.update();
    assert(!updateWithInvalidExtraction);
    
    std::cout << "PASSED" << std::endl;
}

// Test multiple controller updates
void test_controller_multiple_updates() {
    std::cout << "Testing NeuralAIController multiple updates... ";
    
    // Create mock policy and mapping
    auto mockPolicy = std::make_shared<MockAIPolicy>();
    auto mockMapping = std::make_shared<MockAIMemoryMapping>("sf3");
    
    // Create controller
    NeuralAIController controller;
    
    // Configure controller
    controller.setPolicy(mockPolicy);
    controller.setMemoryMapping(mockMapping);
    controller.setPlayerIndex(1);
    
    // Perform multiple updates
    const int numUpdates = 5;
    for (int i = 0; i < numUpdates; i++) {
        bool updateResult = controller.update();
        assert(updateResult);
    }
    
    // Check that the policy performed inference the expected number of times
    assert(mockPolicy->getInferenceCount() == numUpdates);
    
    // Check that the controller has the correct last action
    const AIOutputAction& lastAction = controller.getLastAction();
    assert(lastAction.getFrameNumber() == numUpdates);
    assert(lastAction.getPlayerIndex() == 1);
    
    std::cout << "PASSED" << std::endl;
}

// Test controller with different player indices
void test_controller_player_indices() {
    std::cout << "Testing NeuralAIController with different player indices... ";
    
    // Create mock policy and mapping
    auto mockPolicy = std::make_shared<MockAIPolicy>();
    auto mockMapping = std::make_shared<MockAIMemoryMapping>("sf3");
    
    // Create controller for player 0
    NeuralAIController controller0;
    controller0.setPolicy(mockPolicy);
    controller0.setMemoryMapping(mockMapping);
    controller0.setPlayerIndex(0);
    
    // Perform an update
    mockPolicy->resetInferenceCount();
    mockMapping->resetFrameCount();
    bool updateResult0 = controller0.update();
    assert(updateResult0);
    
    // Check that the controller produced an action for player 0
    const AIOutputAction& action0 = controller0.getLastAction();
    assert(action0.getPlayerIndex() == 0);
    
    // Create controller for player 1
    NeuralAIController controller1;
    controller1.setPolicy(mockPolicy);
    controller1.setMemoryMapping(mockMapping);
    controller1.setPlayerIndex(1);
    
    // Perform an update
    bool updateResult1 = controller1.update();
    assert(updateResult1);
    
    // Check that the controller produced an action for player 1
    const AIOutputAction& action1 = controller1.getLastAction();
    assert(action1.getPlayerIndex() == 1);
    
    std::cout << "PASSED" << std::endl;
}

// Test loading model through the controller
void test_controller_load_model() {
    std::cout << "Testing NeuralAIController load model... ";
    
    // Create mock policy and mapping
    auto mockPolicy = std::make_shared<MockAIPolicy>();
    auto mockMapping = std::make_shared<MockAIMemoryMapping>("sf3");
    
    // Create controller
    NeuralAIController controller;
    
    // Configure controller
    controller.setPolicy(mockPolicy);
    controller.setMemoryMapping(mockMapping);
    controller.setPlayerIndex(1);
    
    // Load model
    const std::string modelPath = "test_model.pt";
    bool loadResult = controller.loadModel(modelPath);
    assert(loadResult);
    assert(mockPolicy->isModelLoaded());
    
    std::cout << "PASSED" << std::endl;
}

int main() {
    std::cout << "==== NeuralAIController Tests ====" << std::endl;
    
    test_controller_initialization();
    test_controller_update();
    test_controller_button_state();
    test_controller_error_handling();
    test_controller_multiple_updates();
    test_controller_player_indices();
    test_controller_load_model();
    
    std::cout << "All tests PASSED!" << std::endl;
    
    return 0;
} 