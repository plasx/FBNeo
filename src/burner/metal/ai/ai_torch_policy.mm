#include "ai_interface.h"
#include "ai_torch_policy.h"
#include <torch/torch.h>
#include <torch/script.h>

#include <string>
#include <vector>
#include <iostream>
#include <memory>

// Maximum number of PyTorch models that can be loaded
#define MAX_TORCH_MODELS 4

// Structure to hold PyTorch model data
struct TorchModelData {
    int modelId;
    std::string name;
    torch::jit::script::Module module;
    bool isActive;
    bool isLoaded;
};

// Global state
static TorchModelData g_torchModels[MAX_TORCH_MODELS];
static bool g_torchInitialized = false;
static int g_numModelsLoaded = 0;

// Initialize PyTorch subsystem
extern "C" bool PyTorch_Initialize() {
    if (g_torchInitialized) {
        return true; // Already initialized
    }
    
    // Initialize PyTorch (nothing special needed for C++ API)
    g_torchInitialized = true;
    
    // Initialize model slots
    for (int i = 0; i < MAX_TORCH_MODELS; i++) {
        g_torchModels[i].modelId = -1;
        g_torchModels[i].isActive = false;
        g_torchModels[i].isLoaded = false;
    }
    
    g_numModelsLoaded = 0;
    
    return true;
}

// Shutdown PyTorch subsystem
extern "C" void PyTorch_Shutdown() {
    if (!g_torchInitialized) {
        return;
    }
    
    // Clear all loaded models
    for (int i = 0; i < MAX_TORCH_MODELS; i++) {
        if (g_torchModels[i].isLoaded) {
            // PyTorch models don't need explicit unloading
            g_torchModels[i].isLoaded = false;
            g_torchModels[i].isActive = false;
        }
    }
    
    g_numModelsLoaded = 0;
    g_torchInitialized = false;
}

// Load a PyTorch model from path
extern "C" int PyTorch_LoadModel(const char* path) {
    if (!g_torchInitialized) {
        if (!PyTorch_Initialize()) {
            return -1;
        }
    }
    
    // Check if we have space for a new model
    if (g_numModelsLoaded >= MAX_TORCH_MODELS) {
        std::cerr << "Maximum number of PyTorch models already loaded" << std::endl;
        return -1;
    }
    
    // Find an empty slot
    int slotIndex = -1;
    for (int i = 0; i < MAX_TORCH_MODELS; i++) {
        if (!g_torchModels[i].isLoaded) {
            slotIndex = i;
            break;
        }
    }
    
    if (slotIndex == -1) {
        std::cerr << "No available slots for PyTorch models" << std::endl;
        return -1;
    }
    
    // Load the model
    try {
        // Load the TorchScript model
        torch::jit::script::Module module = torch::jit::load(path);
        module.eval(); // Set to evaluation mode
        
        // Store model data
        g_torchModels[slotIndex].modelId = slotIndex;
        g_torchModels[slotIndex].name = std::string(path);
        g_torchModels[slotIndex].module = module;
        g_torchModels[slotIndex].isLoaded = true;
        g_torchModels[slotIndex].isActive = false;
        
        g_numModelsLoaded++;
        
        return slotIndex;
    }
    catch (const c10::Error& e) {
        std::cerr << "Error loading PyTorch model: " << e.what() << std::endl;
        return -1;
    }
}

// Activate a loaded model
extern "C" bool PyTorch_ActivateModel(int modelId) {
    if (!g_torchInitialized || modelId < 0 || modelId >= MAX_TORCH_MODELS) {
        return false;
    }
    
    if (!g_torchModels[modelId].isLoaded) {
        return false;
    }
    
    g_torchModels[modelId].isActive = true;
    return true;
}

// Deactivate a model
extern "C" bool PyTorch_DeactivateModel(int modelId) {
    if (!g_torchInitialized || modelId < 0 || modelId >= MAX_TORCH_MODELS) {
        return false;
    }
    
    if (!g_torchModels[modelId].isLoaded) {
        return false;
    }
    
    g_torchModels[modelId].isActive = false;
    return true;
}

// Run inference on a PyTorch model with tensor inputs
extern "C" bool PyTorch_Inference(int modelId, void* inputTensorData, int inputSize, void* outputTensorData, int* outputSize) {
    if (!g_torchInitialized || modelId < 0 || modelId >= MAX_TORCH_MODELS) {
        return false;
    }
    
    if (!g_torchModels[modelId].isLoaded || !g_torchModels[modelId].isActive) {
        return false;
    }
    
    try {
        // Create input tensor from raw data
        // Note: This assumes float32 data in a specific shape
        // In a real implementation, you would need more metadata about the tensor
        float* inputData = static_cast<float*>(inputTensorData);
        torch::Tensor inputTensor = torch::from_blob(inputData, {inputSize / sizeof(float)});
        
        // Create a vector of inputs for the model
        std::vector<torch::jit::IValue> inputs;
        inputs.push_back(inputTensor);
        
        // Run inference
        torch::NoGradGuard no_grad; // Disable gradient computation
        auto output = g_torchModels[modelId].module.forward(inputs).toTensor();
        
        // Copy output tensor data to output buffer
        auto outputAccessor = output.accessor<float, 1>();
        float* outputData = static_cast<float*>(outputTensorData);
        int outputTensorSize = output.numel() * sizeof(float);
        
        if (outputTensorSize > *outputSize) {
            *outputSize = outputTensorSize;
            return false; // Output buffer too small
        }
        
        *outputSize = outputTensorSize;
        
        // Copy data to output buffer
        for (int i = 0; i < output.numel(); i++) {
            outputData[i] = outputAccessor[i];
        }
        
        return true;
    }
    catch (const c10::Error& e) {
        std::cerr << "Error during PyTorch inference: " << e.what() << std::endl;
        return false;
    }
}

// Run inference on a PyTorch model with screen buffer
extern "C" bool PyTorch_InferenceWithScreenBuffer(int modelId, void* screenBuffer, int width, int height, void* outputTensor, int* outputSize) {
    if (!g_torchInitialized || modelId < 0 || modelId >= MAX_TORCH_MODELS) {
        return false;
    }
    
    if (!g_torchModels[modelId].isLoaded || !g_torchModels[modelId].isActive) {
        return false;
    }
    
    try {
        // Convert screen buffer (RGBA format) to PyTorch tensor
        // Assuming 4 channels (RGBA) and unsigned byte data
        unsigned char* bufferData = static_cast<unsigned char*>(screenBuffer);
        
        // Create a tensor from the screen buffer
        // Shape: [height, width, 4] -> [1, 3, height, width] (convert RGBA to RGB and add batch dimension)
        torch::Tensor inputTensor = torch::from_blob(bufferData, {height, width, 4}, torch::kUInt8);
        
        // Convert to RGB (drop alpha channel) and normalize to [0, 1]
        inputTensor = inputTensor.slice(2, 0, 3).to(torch::kFloat32).div(255.0);
        
        // Permute dimensions to [height, width, channels] -> [channels, height, width]
        inputTensor = inputTensor.permute({2, 0, 1});
        
        // Add batch dimension
        inputTensor = inputTensor.unsqueeze(0);
        
        // Create a vector of inputs for the model
        std::vector<torch::jit::IValue> inputs;
        inputs.push_back(inputTensor);
        
        // Run inference
        torch::NoGradGuard no_grad; // Disable gradient computation
        auto output = g_torchModels[modelId].module.forward(inputs).toTensor();
        
        // Copy output tensor data to output buffer
        // This assumes the output is a 1D tensor of floats
        if (output.dim() > 0) {
            auto outputAccessor = output.reshape({-1}).accessor<float, 1>();
            float* outputData = static_cast<float*>(outputTensor);
            int outputTensorSize = output.numel() * sizeof(float);
            
            if (outputTensorSize > *outputSize) {
                *outputSize = outputTensorSize;
                return false; // Output buffer too small
            }
            
            *outputSize = outputTensorSize;
            
            // Copy data to output buffer
            for (int i = 0; i < output.numel(); i++) {
                outputData[i] = outputAccessor[i];
            }
        }
        
        return true;
    }
    catch (const c10::Error& e) {
        std::cerr << "Error during PyTorch inference with screen buffer: " << e.what() << std::endl;
        return false;
    }
} 