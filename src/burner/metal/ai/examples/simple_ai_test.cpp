#include "metal_bridge.h"
#include "ai_definitions.h"
#include <iostream>
#include <cstring>

/**
 * FBNeo CoreML Integration Test
 * 
 * This example demonstrates how to:
 * 1. Initialize the CoreML subsystem
 * 2. Load a model
 * 3. Process frames
 * 4. Get model information
 * 5. Visualize outputs (optional)
 */

// Helper function to print model info
void printModelInfo(const AIModelInfo& info) {
    std::cout << "=== Model Information ===" << std::endl;
    std::cout << "Name: " << info.name << std::endl;
    std::cout << "Version: " << info.version << std::endl;
    std::cout << "Input dimensions: " << info.input_width << "x" << info.input_height 
              << "x" << info.input_channels << std::endl;
    std::cout << "Action count: " << info.action_count << std::endl;
    
    // Print model type
    std::cout << "Model type: ";
    switch (info.model_type) {
        case FBNEO_AI_MODEL_TYPE_COREML:     std::cout << "CoreML"; break;
        case FBNEO_AI_MODEL_TYPE_PYTORCH:    std::cout << "PyTorch"; break;
        case FBNEO_AI_MODEL_TYPE_ONNX:       std::cout << "ONNX"; break;
        case FBNEO_AI_MODEL_TYPE_TENSORFLOW_LITE: std::cout << "TensorFlow Lite"; break;
        case FBNEO_AI_MODEL_TYPE_METAL_GRAPH: std::cout << "Metal Graph"; break;
        default: std::cout << "Unknown"; break;
    }
    std::cout << std::endl;
    
    // Print compute backend
    std::cout << "Compute backend: ";
    switch (info.compute_backend) {
        case FBNEO_AI_COMPUTE_CPU_ONLY:  std::cout << "CPU Only"; break;
        case FBNEO_AI_COMPUTE_GPU_ONLY:  std::cout << "GPU Only"; break;
        case FBNEO_AI_COMPUTE_ANE_ONLY:  std::cout << "ANE Only"; break;
        case FBNEO_AI_COMPUTE_CPU_GPU:   std::cout << "CPU+GPU"; break;
        case FBNEO_AI_COMPUTE_CPU_ANE:   std::cout << "CPU+ANE"; break;
        case FBNEO_AI_COMPUTE_GPU_ANE:   std::cout << "GPU+ANE"; break;
        case FBNEO_AI_COMPUTE_ALL:       std::cout << "All (CPU+GPU+ANE)"; break;
        default: std::cout << "Unknown"; break;
    }
    std::cout << std::endl;
    
    // Print supported features
    std::cout << "Supported features: ";
    if (info.features & FBNEO_AI_FEATURE_PLAYER_ASSIST) std::cout << "Player Assist, ";
    if (info.features & FBNEO_AI_FEATURE_CPU_ENHANCEMENT) std::cout << "CPU Enhancement, ";
    if (info.features & FBNEO_AI_FEATURE_SELF_PLAY) std::cout << "Self Play, ";
    if (info.features & FBNEO_AI_FEATURE_TRAINING) std::cout << "Training, ";
    if (info.features & FBNEO_AI_FEATURE_UPSCALING) std::cout << "Upscaling, ";
    if (info.features & FBNEO_AI_FEATURE_PREDICTION) std::cout << "Prediction, ";
    if (info.features & FBNEO_AI_FEATURE_ANALYTICS) std::cout << "Analytics, ";
    if (info.features & FBNEO_AI_FEATURE_CONTENT_GEN) std::cout << "Content Generation, ";
    std::cout << std::endl;
    
    std::cout << "==========================" << std::endl;
}

// Convert model output to action
void convertOutputToAction(const float* results, int resultCount, AIOutputAction* action) {
    // Simple conversion from model outputs to button actions
    // This will need to be adjusted based on your model's output format
    
    if (resultCount < 8 || !action) return;
    
    // Clear the action
    memset(action, 0, sizeof(AIOutputAction));
    
    // Threshold for button press
    const float threshold = 0.5f;
    
    // Map outputs to buttons
    // Example format: [Up, Down, Left, Right, A, B, C, X, Y, Z, Start]
    // Your model may use a different format
    action->player = 1; // Player 1
    
    // Set button states based on thresholded outputs
    if (results[0] > threshold) action->button_press |= (1 << 0); // Up
    if (results[1] > threshold) action->button_press |= (1 << 1); // Down
    if (results[2] > threshold) action->button_press |= (1 << 2); // Left
    if (results[3] > threshold) action->button_press |= (1 << 3); // Right
    if (results[4] > threshold) action->button_press |= (1 << 4); // A
    if (results[5] > threshold) action->button_press |= (1 << 5); // B
    if (results[6] > threshold) action->button_press |= (1 << 6); // C
    if (resultCount > 7 && results[7] > threshold) action->button_press |= (1 << 7); // Start
    
    // Store confidence values directly
    action->confidence = results[0]; // Example: use first output's confidence
}

// Main test function
int testCoreMLIntegration(const char* modelPath) {
    std::cout << "Testing CoreML integration with model: " << modelPath << std::endl;
    
    // Step 1: Initialize CoreML
    if (!CoreML_Initialize()) {
        std::cerr << "Failed to initialize CoreML" << std::endl;
        return 1;
    }
    
    std::cout << "CoreML initialized successfully" << std::endl;
    
    // Step 2: Load the model
    if (!CoreML_LoadModel(modelPath)) {
        std::cerr << "Failed to load model: " << modelPath << std::endl;
        CoreML_Shutdown();
        return 1;
    }
    
    std::cout << "Model loaded successfully" << std::endl;
    
    // Step 3: Get model information
    AIModelInfo modelInfo;
    if (CoreML_GetModelInfo(&modelInfo)) {
        printModelInfo(modelInfo);
    } else {
        std::cerr << "Failed to get model information" << std::endl;
    }
    
    // Step 4: Create a test frame (simulated game screen)
    const int width = 384;
    const int height = 224;
    const int channels = 4; // RGBA
    const int pitch = width * channels;
    
    // Allocate memory for the test frame
    uint8_t* testFrame = new uint8_t[height * pitch];
    
    // Fill with a simple pattern (gradient)
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int pos = (y * width + x) * channels;
            testFrame[pos + 0] = (uint8_t)(x % 256);           // B
            testFrame[pos + 1] = (uint8_t)(y % 256);           // G
            testFrame[pos + 2] = (uint8_t)((x + y) % 256);     // R
            testFrame[pos + 3] = 255;                           // A (full opacity)
        }
    }
    
    // Step 5: Process the frame with CoreML
    const int maxResults = 32;
    float results[maxResults];
    bool success = CoreML_ProcessFrame(testFrame, width, height, pitch, results, maxResults);
    
    if (success) {
        std::cout << "Frame processed successfully" << std::endl;
        
        // Print the outputs
        std::cout << "Model outputs:" << std::endl;
        for (int i = 0; i < std::min(maxResults, modelInfo.action_count); i++) {
            std::cout << "  Output " << i << ": " << results[i] << std::endl;
        }
        
        // Convert outputs to action
        AIOutputAction action;
        convertOutputToAction(results, std::min(maxResults, modelInfo.action_count), &action);
        
        // Print the action
        std::cout << "Converted action:" << std::endl;
        std::cout << "  Player: " << action.player << std::endl;
        std::cout << "  Button presses: 0x" << std::hex << action.button_press << std::dec << std::endl;
        std::cout << "  Confidence: " << action.confidence << std::endl;
    } else {
        std::cerr << "Failed to process frame" << std::endl;
    }
    
    // Step 6: Create a visualization (optional)
    uint8_t* visualization = new uint8_t[height * pitch];
    bool visSuccess = CoreML_RenderVisualization(visualization, width, height, pitch, 0);
    
    if (visSuccess) {
        std::cout << "Visualization created successfully" << std::endl;
        // In a real application, you would display this visualization
        // Here we'll just indicate success
    } else {
        std::cout << "Visualization not available" << std::endl;
    }
    
    // Clean up
    delete[] testFrame;
    delete[] visualization;
    
    // Step 7: Shutdown CoreML
    CoreML_Shutdown();
    std::cout << "CoreML shut down" << std::endl;
    
    return 0;
}

// When integrated into FBNeo, this function would be called to test the AI
int main(int argc, char* argv[]) {
    // Default model path
    const char* modelPath = "models/generic.mlmodel";
    
    // Use command line argument for model path if provided
    if (argc > 1) {
        modelPath = argv[1];
    }
    
    return testCoreMLIntegration(modelPath);
} 