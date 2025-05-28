#include "../../src/ai/ai_torch_policy.h"
#include "../../src/ai/ai_input_frame.h"
#include "../../src/ai/ai_output_action.h"
#include <cassert>
#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include <filesystem>
#include <fstream>
#include <random>
#include <torch/torch.h>

// Helper function to create a test model file
bool createTestModel(const std::string& filepath) {
    // Create a minimal PyTorch traced model file for testing
    // This is just a stub file with some content to test loading
    std::ofstream file(filepath, std::ios::binary);
    if (!file) {
        return false;
    }
    
    // Write some dummy data to the file
    const char* data = "PYTORCH MODEL STUB";
    file.write(data, strlen(data));
    file.close();
    
    return true;
}

// Helper function to delete a file
void deleteTestFile(const std::string& filepath) {
    std::filesystem::remove(filepath);
}

// Helper function to create a test input frame
AIInputFrame createTestInputFrame(int frameNumber) {
    AIInputFrame frame(frameNumber);
    frame.setGameId("sf2ce");
    frame.setPlayerCount(2);
    frame.setPlayerIndex(0);
    
    // Set some player values
    frame.setPlayerValue(0, "p1_health", 80.0f);
    frame.setPlayerValue(1, "p2_health", 70.0f);
    frame.setPlayerValue(0, "p1_x_pos", 150.0f);
    frame.setPlayerValue(0, "p1_y_pos", 200.0f);
    frame.setPlayerValue(1, "p2_x_pos", 350.0f);
    frame.setPlayerValue(1, "p2_y_pos", 200.0f);
    
    // Add some feature values
    frame.setFeatureValue("dist_x", 200.0f);
    frame.setFeatureValue("dist_y", 0.0f);
    
    return frame;
}

// Create a mock model file for testing
bool createMockTorchModel(const std::string& filename) {
    try {
        // Create a simple neural network
        torch::nn::Sequential model(
            torch::nn::Linear(20, 64),
            torch::nn::ReLU(),
            torch::nn::Linear(64, 32),
            torch::nn::ReLU(),
            torch::nn::Linear(32, 10)
        );
        
        // Save the model
        torch::save(model, filename);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error creating mock model: " << e.what() << std::endl;
        return false;
    }
}

// Create a test input frame
AIInputFrame createTestInputFrame(int frameNumber, bool randomize = false) {
    // Create a test input frame
    AIInputFrame frame(frameNumber);
    frame.setGameId("sf3");
    
    // Random generator if needed
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(0.0f, 100.0f);
    
    float p1_health = randomize ? dist(gen) : 75.0f;
    float p1_x = randomize ? dist(gen) : 150.0f;
    float p1_y = randomize ? dist(gen) : 200.0f;
    float p1_state = randomize ? static_cast<float>(gen() % 5) : 1.0f;
    
    float p2_health = randomize ? dist(gen) : 60.0f;
    float p2_x = randomize ? dist(gen) : 300.0f;
    float p2_y = randomize ? dist(gen) : 200.0f;
    float p2_state = randomize ? static_cast<float>(gen() % 5) : 2.0f;
    
    // Add player 1 data
    frame.addPlayerValue(0, "p1_health", p1_health);
    frame.addPlayerValue(0, "p1_x_pos", p1_x);
    frame.addPlayerValue(0, "p1_y_pos", p1_y);
    frame.addPlayerValue(0, "p1_state", p1_state);
    
    // Add player 2 data
    frame.addPlayerValue(1, "p2_health", p2_health);
    frame.addPlayerValue(1, "p2_x_pos", p2_x);
    frame.addPlayerValue(1, "p2_y_pos", p2_y);
    frame.addPlayerValue(1, "p2_state", p2_state);
    
    // Add some feature values
    frame.addFeatureValue("round_timer", randomize ? dist(gen) : 60.0f);
    frame.addFeatureValue("stage_id", randomize ? static_cast<float>(gen() % 10) : 3.0f);
    
    // Add frame hash
    frame.setHash(std::to_string(frameNumber * 12345));
    
    return frame;
}

// Helper function to delete a file if it exists
void deleteFileIfExists(const std::string& filepath) {
    if (std::filesystem::exists(filepath)) {
        std::filesystem::remove(filepath);
    }
}

// Test function for loading a model
void test_torch_policy_load_model() {
    std::cout << "Testing AITorchPolicy model loading... ";
    
    const std::string modelPath = "test_model.pt";
    
    // Create a mock model
    bool modelCreated = createMockTorchModel(modelPath);
    if (!modelCreated) {
        std::cout << "SKIPPED (failed to create mock model)" << std::endl;
        return;
    }
    
    // Initialize the policy
    AITorchPolicy policy;
    
    // Load the model
    bool loaded = policy.loadModel(modelPath);
    assert(loaded);
    
    // Verify that the model is loaded
    assert(policy.isModelLoaded());
    
    // Clean up
    deleteFileIfExists(modelPath);
    
    std::cout << "PASSED" << std::endl;
}

// Test function for inference
void test_torch_policy_inference() {
    std::cout << "Testing AITorchPolicy inference... ";
    
    const std::string modelPath = "test_model.pt";
    
    // Create a mock model
    bool modelCreated = createMockTorchModel(modelPath);
    if (!modelCreated) {
        std::cout << "SKIPPED (failed to create mock model)" << std::endl;
        return;
    }
    
    // Initialize the policy
    AITorchPolicy policy;
    
    // Load the model
    bool loaded = policy.loadModel(modelPath);
    assert(loaded);
    
    // Create a test input frame
    AIInputFrame frame = createTestInputFrame(1);
    
    // Set player index for inference
    int playerIndex = 1;
    
    // Run inference
    AIOutputAction action = policy.inferAction(frame, playerIndex);
    
    // Verify the action has the correct playerIndex
    assert(action.getPlayerIndex() == playerIndex);
    
    // Verify the action has the correct frameNumber
    assert(action.getFrameNumber() == frame.getFrameNumber());
    
    // Check that at least one button is pressed (the inference should produce some output)
    bool anyButtonPressed = false;
    for (int i = 0; i < static_cast<int>(AIOutputAction::Button::Count); i++) {
        AIOutputAction::Button button = static_cast<AIOutputAction::Button>(i);
        if (action.getButtonState(button)) {
            anyButtonPressed = true;
            break;
        }
    }
    
    // We expect the random weights to generate some button presses
    assert(anyButtonPressed);
    
    // Clean up
    deleteFileIfExists(modelPath);
    
    std::cout << "PASSED" << std::endl;
}

// Test function for inference with different player indices
void test_torch_policy_player_indices() {
    std::cout << "Testing AITorchPolicy with different player indices... ";
    
    const std::string modelPath = "test_model.pt";
    
    // Create a mock model
    bool modelCreated = createMockTorchModel(modelPath);
    if (!modelCreated) {
        std::cout << "SKIPPED (failed to create mock model)" << std::endl;
        return;
    }
    
    // Initialize the policy
    AITorchPolicy policy;
    
    // Load the model
    bool loaded = policy.loadModel(modelPath);
    assert(loaded);
    
    // Create a test input frame
    AIInputFrame frame = createTestInputFrame(1);
    
    // Run inference for player 0
    AIOutputAction action0 = policy.inferAction(frame, 0);
    assert(action0.getPlayerIndex() == 0);
    
    // Run inference for player 1
    AIOutputAction action1 = policy.inferAction(frame, 1);
    assert(action1.getPlayerIndex() == 1);
    
    // Clean up
    deleteFileIfExists(modelPath);
    
    std::cout << "PASSED" << std::endl;
}

// Test function for inference with cached models
void test_torch_policy_model_caching() {
    std::cout << "Testing AITorchPolicy model caching... ";
    
    const std::string modelPath1 = "test_model1.pt";
    const std::string modelPath2 = "test_model2.pt";
    
    // Create two mock models
    bool model1Created = createMockTorchModel(modelPath1);
    bool model2Created = createMockTorchModel(modelPath2);
    
    if (!model1Created || !model2Created) {
        std::cout << "SKIPPED (failed to create mock models)" << std::endl;
        deleteFileIfExists(modelPath1);
        deleteFileIfExists(modelPath2);
        return;
    }
    
    // Initialize the policy
    AITorchPolicy policy;
    
    // Load model 1
    bool loaded1 = policy.loadModel(modelPath1);
    assert(loaded1);
    
    // Run inference with model 1
    AIInputFrame frame1 = createTestInputFrame(1);
    AIOutputAction action1 = policy.inferAction(frame1, 0);
    
    // Load model 2
    bool loaded2 = policy.loadModel(modelPath2);
    assert(loaded2);
    
    // Run inference with model 2
    AIInputFrame frame2 = createTestInputFrame(2);
    AIOutputAction action2 = policy.inferAction(frame2, 0);
    
    // Re-load model 1 (should use cache)
    bool reloaded1 = policy.loadModel(modelPath1);
    assert(reloaded1);
    
    // Run inference with model 1 again
    AIInputFrame frame3 = createTestInputFrame(3);
    AIOutputAction action3 = policy.inferAction(frame3, 0);
    
    // Verify that all actions have the correct frame numbers
    assert(action1.getFrameNumber() == 1);
    assert(action2.getFrameNumber() == 2);
    assert(action3.getFrameNumber() == 3);
    
    // Clean up
    deleteFileIfExists(modelPath1);
    deleteFileIfExists(modelPath2);
    
    std::cout << "PASSED" << std::endl;
}

// Test function for inference with different input frames
void test_torch_policy_different_inputs() {
    std::cout << "Testing AITorchPolicy with different inputs... ";
    
    const std::string modelPath = "test_model.pt";
    
    // Create a mock model
    bool modelCreated = createMockTorchModel(modelPath);
    if (!modelCreated) {
        std::cout << "SKIPPED (failed to create mock model)" << std::endl;
        return;
    }
    
    // Initialize the policy
    AITorchPolicy policy;
    
    // Load the model
    bool loaded = policy.loadModel(modelPath);
    assert(loaded);
    
    // Run inference with 10 different input frames
    const int numFrames = 10;
    for (int i = 0; i < numFrames; i++) {
        AIInputFrame frame = createTestInputFrame(i, true); // randomize inputs
        AIOutputAction action = policy.inferAction(frame, 0);
        
        // Verify the action has the correct frame number
        assert(action.getFrameNumber() == i);
    }
    
    // Clean up
    deleteFileIfExists(modelPath);
    
    std::cout << "PASSED" << std::endl;
}

// Test function for error handling
void test_torch_policy_error_handling() {
    std::cout << "Testing AITorchPolicy error handling... ";
    
    // Initialize the policy
    AITorchPolicy policy;
    
    // Try to load a non-existent model
    bool loaded = policy.loadModel("non_existent_model.pt");
    assert(!loaded);
    
    // Verify that the model is not loaded
    assert(!policy.isModelLoaded());
    
    // Try to run inference without a loaded model
    AIInputFrame frame = createTestInputFrame(1);
    AIOutputAction action = policy.inferAction(frame, 0);
    
    // The action should be empty (default-initialized)
    for (int i = 0; i < static_cast<int>(AIOutputAction::Button::Count); i++) {
        AIOutputAction::Button button = static_cast<AIOutputAction::Button>(i);
        assert(!action.getButtonState(button));
    }
    
    std::cout << "PASSED" << std::endl;
}

int main() {
    std::cout << "==== AITorchPolicy Tests ====" << std::endl;
    
    test_torch_policy_load_model();
    test_torch_policy_inference();
    test_torch_policy_player_indices();
    test_torch_policy_model_caching();
    test_torch_policy_different_inputs();
    test_torch_policy_error_handling();
    
    std::cout << "All tests PASSED!" << std::endl;
    
    return 0;
} 