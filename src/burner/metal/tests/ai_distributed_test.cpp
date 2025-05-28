#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <fstream>
#include <random>
#include <vector>
#include <iomanip>
#include <numeric>
#include <algorithm>
#include "../ai/ai_distributed_training.h"
#include "../ai/ai_torch_policy.h"
#include "../ai/ai_input_frame.h"
#include "../ai/ai_output_action.h"

using namespace fbneo::ai;

// Simple mock policy for testing
class MockTorchPolicy : public AITorchPolicy {
private:
    // Internal network weights for testing
    std::vector<float> weights;
    std::mt19937 rng;
    int updateCount;
    
public:
    MockTorchPolicy() : updateCount(0) {
        // Initialize with dummy values
        std::vector<int> dims = {4, 84, 84}; // 4 channel 84x84 input
        initialize(dims, 10); // 10 possible actions
        
        // Initialize random weights
        std::random_device rd;
        rng = std::mt19937(rd());
        std::uniform_real_distribution<float> dist(-0.1f, 0.1f);
        
        // Create some random weights (100 for testing)
        weights.resize(100);
        for (auto& w : weights) {
            w = dist(rng);
        }
    }
    
    bool predict(const AIInputFrame& state, AIOutputAction& action, bool exploit) override {
        // Deterministic prediction based on weights sum
        float weightSum = std::accumulate(weights.begin(), weights.end(), 0.0f);
        
        // Use weight sum to determine actions in a deterministic way
        action.up = (weightSum > 0.0f);
        action.down = (weightSum < -5.0f);
        action.left = (weightSum < 0.0f);
        action.right = (weightSum > 5.0f);
        
        // Button presses based on weight values at specific indices
        for (int i = 0; i < 6; ++i) {
            action.buttons[i] = (weights[i * 10] > 0.0f);
        }
        
        // Start and coin buttons
        action.start = (weightSum > 10.0f);
        action.coin = (weightSum < -10.0f);
        
        return true;
    }
    
    float getValue(const AIInputFrame& state) override {
        // Return a value based on weights
        float weightSum = std::accumulate(weights.begin(), weights.end(), 0.0f);
        return weightSum / 50.0f; // Scale to reasonable range
    }
    
    float update(const std::vector<std::vector<float>>& states,
                const std::vector<std::vector<float>>& actions,
                const std::vector<float>& oldLogProbs,
                const std::vector<float>& advantages,
                const std::vector<float>& returns,
                float learningRate) override {
        // Simple update that shifts weights slightly
        std::uniform_real_distribution<float> dist(-learningRate, learningRate);
        
        // Update weights based on advantages (simplified)
        for (size_t i = 0; i < weights.size(); ++i) {
            float avgAdvantage = 0.0f;
            if (!advantages.empty()) {
                avgAdvantage = std::accumulate(advantages.begin(), advantages.end(), 0.0f) / advantages.size();
            }
            
            // Apply update
            weights[i] += dist(rng) * (avgAdvantage + 0.01f);
        }
        
        updateCount++;
        return 0.5f / updateCount; // Decreasing loss
    }
    
    bool save(const std::string& path) override {
        // Save weights to file
        std::ofstream file(path, std::ios::binary);
        if (!file) return false;
        
        // Save version and size
        uint32_t version = 1;
        uint32_t size = weights.size();
        file.write(reinterpret_cast<char*>(&version), sizeof(version));
        file.write(reinterpret_cast<char*>(&size), sizeof(size));
        
        // Save weights
        file.write(reinterpret_cast<char*>(weights.data()), weights.size() * sizeof(float));
        
        return file.good();
    }
    
    bool load(const std::string& path) override {
        // Load weights from file
        std::ifstream file(path, std::ios::binary);
        if (!file) return false;
        
        // Load version and size
        uint32_t version, size;
        file.read(reinterpret_cast<char*>(&version), sizeof(version));
        file.read(reinterpret_cast<char*>(&size), sizeof(size));
        
        // Load weights
        weights.resize(size);
        file.read(reinterpret_cast<char*>(weights.data()), size * sizeof(float));
        
        return file.good();
    }
    
    AITorchPolicy* clone() override {
        // Create a new policy
        MockTorchPolicy* clone = new MockTorchPolicy();
        
        // Copy weights
        clone->weights = weights;
        
        return clone;
    }
    
    void copyFrom(const AITorchPolicy* other) override {
        // Copy weights from another policy
        const MockTorchPolicy* otherPolicy = dynamic_cast<const MockTorchPolicy*>(other);
        if (otherPolicy) {
            weights = otherPolicy->weights;
        }
    }
    
    std::vector<float> getWeights() const {
        return weights;
    }
};

// Helper class for generating test environments
class TestEnvironment {
private:
    std::mt19937 rng;
    std::vector<uint8_t> frameBuffer;
    int width, height;
    
public:
    TestEnvironment(int width = 84, int height = 84) : width(width), height(height) {
        std::random_device rd;
        rng = std::mt19937(rd());
        frameBuffer.resize(width * height * 4, 0); // RGBA buffer
        
        // Initialize with random data
        regenerateFrame();
    }
    
    void regenerateFrame() {
        std::uniform_int_distribution<uint8_t> dist(0, 255);
        for (auto& b : frameBuffer) {
            b = dist(rng);
        }
    }
    
    AIInputFrame getFrame() const {
        AIInputFrame frame;
        frame.width = width;
        frame.height = height;
        frame.frameBuffer = const_cast<void*>(static_cast<const void*>(frameBuffer.data()));
        return frame;
    }
    
    AIInputFrame getNextFrame(const AIOutputAction& action) {
        // Update frame based on action
        if (action.up) {
            // Shift frame up
            for (int y = 0; y < height - 1; ++y) {
                for (int x = 0; x < width; ++x) {
                    int destIdx = (y * width + x) * 4;
                    int srcIdx = ((y + 1) * width + x) * 4;
                    std::copy(frameBuffer.begin() + srcIdx, 
                             frameBuffer.begin() + srcIdx + 4, 
                             frameBuffer.begin() + destIdx);
                }
            }
        } else if (action.down) {
            // Shift frame down
            for (int y = height - 1; y > 0; --y) {
                for (int x = 0; x < width; ++x) {
                    int destIdx = (y * width + x) * 4;
                    int srcIdx = ((y - 1) * width + x) * 4;
                    std::copy(frameBuffer.begin() + srcIdx, 
                             frameBuffer.begin() + srcIdx + 4, 
                             frameBuffer.begin() + destIdx);
                }
            }
        } else if (action.left) {
            // Shift frame left
            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width - 1; ++x) {
                    int destIdx = (y * width + x) * 4;
                    int srcIdx = (y * width + (x + 1)) * 4;
                    std::copy(frameBuffer.begin() + srcIdx, 
                             frameBuffer.begin() + srcIdx + 4, 
                             frameBuffer.begin() + destIdx);
                }
            }
        } else if (action.right) {
            // Shift frame right
            for (int y = 0; y < height; ++y) {
                for (int x = width - 1; x > 0; --x) {
                    int destIdx = (y * width + x) * 4;
                    int srcIdx = (y * width + (x - 1)) * 4;
                    std::copy(frameBuffer.begin() + srcIdx, 
                             frameBuffer.begin() + srcIdx + 4, 
                             frameBuffer.begin() + destIdx);
                }
            }
        }
        
        // Add some random noise
        std::uniform_int_distribution<int> pixelDist(0, width * height - 1);
        std::uniform_int_distribution<uint8_t> colorDist(0, 255);
        
        for (int i = 0; i < 10; ++i) {
            int pixel = pixelDist(rng);
            for (int c = 0; c < 4; ++c) {
                frameBuffer[pixel * 4 + c] = colorDist(rng);
            }
        }
        
        return getFrame();
    }
};

// Test distributed training with different configurations
void testDistributedTraining() {
    std::cout << "=== Testing Distributed Training ===" << std::endl;
    
    // Create a global policy
    auto policy = std::make_unique<MockTorchPolicy>();
    
    // Create a distributed trainer with 4 workers
    DistributedTrainer trainer(policy.get(), 4);
    
    // Set hyperparameters
    std::unordered_map<std::string, float> params = {
        {"learning_rate", 0.001f},
        {"gamma", 0.99f},
        {"sync_frequency", 10.0f}
    };
    trainer.setHyperparameters(params);
    
    // Test different configurations
    std::cout << "\n--- Testing with Experience Sharing Enabled ---" << std::endl;
    trainer.setExperienceSharing(true, 5000);
    trainer.setAlgorithm("a3c");
    
    // Start training for a short period
    std::cout << "Starting training for 5 episodes per worker..." << std::endl;
    trainer.startTraining(5);
    
    // Wait for training to complete or timeout
    for (int i = 0; i < 15; ++i) {
        std::cout << "Waiting for training to complete: " << (i + 1) << "/15 seconds..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        // Get status and print it occasionally
        if (i % 5 == 0) {
            std::cout << trainer.getStatus() << std::endl;
        }
    }
    
    // Stop training
    trainer.stopTraining();
    
    // Test model saving and loading
    std::cout << "\n--- Testing Model Save/Load ---" << std::endl;
    bool saved = trainer.saveModel("distributed_test_model");
    std::cout << "Model saved: " << (saved ? "success" : "failed") << std::endl;
    
    bool loaded = trainer.loadModel("distributed_test_model");
    std::cout << "Model loaded: " << (loaded ? "success" : "failed") << std::endl;
    
    // Test with different algorithm
    std::cout << "\n--- Testing with PPO Algorithm ---" << std::endl;
    trainer.setAlgorithm("ppo");
    trainer.setExperienceSharing(false);
    
    // Start training for a short period
    std::cout << "Starting training with PPO for 3 episodes per worker..." << std::endl;
    trainer.startTraining(3);
    
    // Wait briefly and stop
    std::this_thread::sleep_for(std::chrono::seconds(10));
    trainer.stopTraining();
    
    std::cout << "\nDistributed training tests completed!" << std::endl;
}

// Test hardware-specific optimizations on Metal
void testMetalOptimizations() {
    std::cout << "\n=== Testing Metal-Specific Optimizations ===" << std::endl;
    
#if defined(__APPLE__)
    // Create a global policy
    auto policy = std::make_unique<MockTorchPolicy>();
    
    // Create a distributed trainer with 2 workers
    DistributedTrainer trainer(policy.get(), 2);
    
    // Get hardware info
    std::string hardwareInfo = trainer.getHardwareInfo();
    std::cout << "Hardware Info:\n" << hardwareInfo << std::endl;
    
    // Test Metal optimization
    std::cout << "Optimizing for Metal..." << std::endl;
    trainer.optimizeForMetal();
    
    // Run a simple benchmark
    std::cout << "Running Metal compute benchmark..." << std::endl;
    
    // Prepare test data
    std::vector<Experience> testBatch;
    for (int i = 0; i < 100; ++i) {
        std::vector<float> state(84 * 84, 0.5f);
        std::vector<float> nextState(84 * 84, 0.6f);
        std::vector<float> action(10, 0.0f);
        action[i % 10] = 1.0f;
        
        Experience exp{state, action, 1.0f, nextState, false};
        testBatch.push_back(exp);
    }
    
    // Process batch with Metal
    auto startTime = std::chrono::high_resolution_clock::now();
    trainer.processExperienceBatchWithMetal(testBatch);
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    std::cout << "Metal batch processing time: " << duration.count() << " ms" << std::endl;
#else
    std::cout << "Metal optimizations are only available on Apple platforms." << std::endl;
#endif
    
    std::cout << "Metal optimization tests completed!" << std::endl;
}

// Test synchronization mechanisms
void testSynchronization() {
    std::cout << "\n=== Testing Synchronization Mechanisms ===" << std::endl;
    
    // Create a global policy with known weights
    auto globalPolicy = std::make_unique<MockTorchPolicy>();
    auto initialWeights = dynamic_cast<MockTorchPolicy*>(globalPolicy.get())->getWeights();
    
    // Print initial weights statistics
    float initialSum = std::accumulate(initialWeights.begin(), initialWeights.end(), 0.0f);
    std::cout << "Initial weights sum: " << initialSum << std::endl;
    
    // Create a distributed trainer with 3 workers
    DistributedTrainer trainer(globalPolicy.get(), 3);
    
    // Set up for A3C
    trainer.setAlgorithm("a3c");
    trainer.setSynchronizationFrequency(2);
    
    // Start training for a very short period
    std::cout << "Starting training for 2 episodes per worker..." << std::endl;
    trainer.startTraining(2);
    
    // Wait briefly
    std::this_thread::sleep_for(std::chrono::seconds(5));
    
    // Stop training
    trainer.stopTraining();
    
    // Check if weights were updated
    auto finalWeights = dynamic_cast<MockTorchPolicy*>(globalPolicy.get())->getWeights();
    float finalSum = std::accumulate(finalWeights.begin(), finalWeights.end(), 0.0f);
    std::cout << "Final weights sum: " << finalSum << std::endl;
    
    // Calculate weight change
    float weightChange = std::abs(finalSum - initialSum);
    std::cout << "Weight change magnitude: " << weightChange << std::endl;
    
    if (weightChange > 0.01f) {
        std::cout << "Synchronization test passed: weights were updated" << std::endl;
    } else {
        std::cout << "Synchronization test failed: weights didn't change significantly" << std::endl;
    }
}

// Test training with multiple episodes
void testMultiEpisodeTraining() {
    std::cout << "\n=== Testing Multi-Episode Training ===" << std::endl;
    
    // Create environment
    TestEnvironment env;
    
    // Create a global policy
    auto policy = std::make_unique<MockTorchPolicy>();
    
    // Create a distributed trainer with 2 workers
    DistributedTrainer trainer(policy.get(), 2);
    trainer.setAlgorithm("ppo");
    
    // Start training
    std::cout << "Starting multi-episode training..." << std::endl;
    trainer.startTraining(10);
    
    // Periodically check progress
    for (int i = 0; i < 3; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        std::cout << "Progress update after " << ((i+1) * 5) << " seconds:" << std::endl;
        std::cout << trainer.getStatus() << std::endl;
    }
    
    // Stop training
    trainer.stopTraining();
    
    // Test the trained policy
    std::cout << "Testing trained policy..." << std::endl;
    AIInputFrame testFrame = env.getFrame();
    AIOutputAction action;
    
    policy->predict(testFrame, action, true);
    
    // Print action
    std::cout << "Predicted action: "
              << "Up=" << action.up
              << ", Down=" << action.down
              << ", Left=" << action.left
              << ", Right=" << action.right
              << ", Buttons=[";
    
    for (int i = 0; i < 6; ++i) {
        std::cout << action.buttons[i];
        if (i < 5) std::cout << ",";
    }
    std::cout << "]" << std::endl;
    
    std::cout << "Multi-episode training test completed!" << std::endl;
}

// Comprehensive test suite
void runComprehensiveTests() {
    std::cout << "=============================================" << std::endl;
    std::cout << "    FBNEO AI DISTRIBUTED TRAINING TESTS     " << std::endl;
    std::cout << "=============================================" << std::endl;
    
    // Record start time
    auto startTime = std::chrono::high_resolution_clock::now();
    
    try {
        // Basic distributed training test
        testDistributedTraining();
        
        // Test Metal optimizations
        testMetalOptimizations();
        
        // Test synchronization mechanisms
        testSynchronization();
        
        // Test multi-episode training
        testMultiEpisodeTraining();
        
        // Record end time and print total duration
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime);
        
        std::cout << "\n=============================================" << std::endl;
        std::cout << "All tests completed in " << duration.count() << " seconds" << std::endl;
        std::cout << "=============================================" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
    }
}

int main(int argc, char* argv[]) {
    // Parse command line arguments
    bool runSpecificTest = false;
    std::string testName;
    
    if (argc > 1) {
        testName = argv[1];
        runSpecificTest = true;
    }
    
    try {
        if (!runSpecificTest) {
            // Run all tests
            runComprehensiveTests();
        } else {
            // Run specific test
            if (testName == "distributed") {
                testDistributedTraining();
            } else if (testName == "metal") {
                testMetalOptimizations();
            } else if (testName == "sync") {
                testSynchronization();
            } else if (testName == "episodes") {
                testMultiEpisodeTraining();
            } else {
                std::cout << "Unknown test: " << testName << std::endl;
                std::cout << "Available tests: distributed, metal, sync, episodes" << std::endl;
            }
        }
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return 1;
    }
} 