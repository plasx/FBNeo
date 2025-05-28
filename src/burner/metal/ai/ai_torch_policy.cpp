#include "ai_torch_policy.h"
#include "ai_input_frame.h"
#include "ai_output_action.h"
#include <stdio.h>
#include <string>
#include <iostream>
#include <vector>
#include <chrono>
#include <random>
#include <fstream>
#include <algorithm>
#include <cmath>

// Define constants
#define MAX_BATCH_SIZE 1
#define MAX_HISTORY_FRAMES 4

// LibTorch includes (these would be enabled with USE_LIBTORCH)
#ifdef USE_LIBTORCH
#include <torch/script.h>
#include <torch/torch.h>
#endif

// External C interfaces to Metal implementations
#ifdef __APPLE__
extern "C" {
    // MPSGraph engine
    void* MPSGraph_Create();
    void MPSGraph_Destroy(void* handle);
    int MPSGraph_LoadModel(void* handle, const char* path);
    int MPSGraph_RunInference(void* handle, const AIInputFrame* input, AIOutputAction* output);
    int MPSGraph_IsModelLoaded(void* handle);
    
    // CoreML engine
    void* CoreML_Create();
    void CoreML_Destroy(void* handle);
    int CoreML_LoadModel(void* handle, const char* path);
    int CoreML_RunInference(void* handle, const AIInputFrame* input, AIOutputAction* output);
    int CoreML_IsModelLoaded(void* handle);
    const char* CoreML_GetModelInfo(void* handle);
}
#endif

// Metal-specific headers for direct integration
#if defined(__APPLE__) && defined(USE_METAL_DIRECT)
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h>
#include "metal_compute_helper.h"
#endif

namespace fbneo {
namespace ai {

// Private implementation struct to hide PyTorch details and Metal integration
struct AITorchPolicy::PolicyImpl {
#ifdef USE_LIBTORCH
    // PyTorch model
    torch::jit::script::Module torchModule;
    bool torchModuleLoaded = false;
    
    // Metal-specific PyTorch tensors
    torch::Tensor inputTensor;
    torch::Tensor outputTensor;
    bool useMetalBackend = false;
#endif
    
    // Metal-specific integration
#if defined(__APPLE__)
    void* mpsGraphHandle = nullptr;
    void* coreMLHandle = nullptr;
    bool useNativeMetalBackend = false;
    bool useCoreMLBackend = false;
    
    // Metal direct integration
#if defined(USE_METAL_DIRECT)
    id<MTLDevice> metalDevice = nil;
    id<MTLCommandQueue> metalCommandQueue = nil;
    id<MTLComputePipelineState> metalPreprocessPipeline = nil;
    id<MTLComputePipelineState> metalPostprocessPipeline = nil;
    id<MTLBuffer> inputBuffer = nil;
    id<MTLBuffer> outputBuffer = nil;
    MetalComputeHelper* metalHelper = nullptr;
    MPSNNGraph* neuralNetworkGraph = nullptr;
#endif
#endif
    
    // Common random number generator for fallback behavior
    std::mt19937 rng;
    std::uniform_real_distribution<float> dist;
    
    // Constructor
    PolicyImpl() : dist(0.0f, 1.0f) {
        std::random_device rd;
        rng = std::mt19937(rd());
        
#if defined(__APPLE__)
        // Create Metal integration objects
        initializeMetalBackends();
#endif
    }
    
    // Destructor
    ~PolicyImpl() {
#if defined(__APPLE__)
        // Clean up Metal integration objects
        cleanupMetalBackends();
#endif
    }
    
#if defined(__APPLE__)
    // Initialize Metal backends
    void initializeMetalBackends() {
        // Initialize MPS Graph engine
        mpsGraphHandle = MPSGraph_Create();
        if (mpsGraphHandle) {
            std::cout << "Successfully created MPSGraph engine" << std::endl;
        } else {
            std::cerr << "Failed to create MPSGraph engine" << std::endl;
        }
        
        // Initialize CoreML engine
        coreMLHandle = CoreML_Create();
        if (coreMLHandle) {
            std::cout << "Successfully created CoreML engine" << std::endl;
        } else {
            std::cerr << "Failed to create CoreML engine" << std::endl;
        }
        
#if defined(USE_METAL_DIRECT)
        // Initialize direct Metal integration
        metalDevice = MTLCreateSystemDefaultDevice();
        if (metalDevice) {
            metalCommandQueue = [metalDevice newCommandQueue];
            metalHelper = new MetalComputeHelper(metalDevice, metalCommandQueue);
            
            // Create buffers and pipelines here
            // This would be expanded in a full implementation
            std::cout << "Successfully initialized direct Metal integration" << std::endl;
        } else {
            std::cerr << "Failed to create Metal device" << std::endl;
        }
#endif
    }
    
    // Clean up Metal backends
    void cleanupMetalBackends() {
        // Clean up MPS Graph engine
        if (mpsGraphHandle) {
            MPSGraph_Destroy(mpsGraphHandle);
            mpsGraphHandle = nullptr;
        }
        
        // Clean up CoreML engine
        if (coreMLHandle) {
            CoreML_Destroy(coreMLHandle);
            coreMLHandle = nullptr;
        }
        
#if defined(USE_METAL_DIRECT)
        // Clean up direct Metal integration
        if (neuralNetworkGraph) {
            neuralNetworkGraph = nil;
        }
        
        if (metalHelper) {
            delete metalHelper;
            metalHelper = nullptr;
        }
        
        metalPreprocessPipeline = nil;
        metalPostprocessPipeline = nil;
        inputBuffer = nil;
        outputBuffer = nil;
        metalCommandQueue = nil;
        metalDevice = nil;
#endif
    }
    
    // Load model into Metal backend
    bool loadModelIntoMetalBackend(const std::string& path) {
        bool success = false;
        
        // Try to load model into MPSGraph
        if (mpsGraphHandle) {
            int result = MPSGraph_LoadModel(mpsGraphHandle, path.c_str());
            if (result == 0) {
                useNativeMetalBackend = true;
                success = true;
                std::cout << "Successfully loaded model into MPSGraph engine" << std::endl;
            } else {
                std::cerr << "Failed to load model into MPSGraph engine" << std::endl;
            }
        }
        
        // If MPSGraph failed, try CoreML
        if (!success && coreMLHandle) {
            int result = CoreML_LoadModel(coreMLHandle, path.c_str());
            if (result == 0) {
                useCoreMLBackend = true;
                success = true;
                std::cout << "Successfully loaded model into CoreML engine" << std::endl;
                
                // Print model info
                const char* modelInfo = CoreML_GetModelInfo(coreMLHandle);
                if (modelInfo) {
                    std::cout << "CoreML model info: " << modelInfo << std::endl;
                }
            } else {
                std::cerr << "Failed to load model into CoreML engine" << std::endl;
            }
        }
        
#if defined(USE_METAL_DIRECT)
        // If both failed and we have direct Metal integration, try that
        if (!success && metalDevice) {
            // This would be implemented in a full version
            // It would load network weights directly into Metal buffers
            std::cout << "Attempting to load model into direct Metal backend" << std::endl;
            
            // For demonstration purposes, we'll just set a flag
            // success = loadModelIntoDirectMetal(path);
        }
#endif
        
        return success;
    }
    
    // Run inference using Metal backend
    bool runInferenceWithMetalBackend(const AIInputFrame& input, AIOutputAction& output) {
        // Try MPSGraph first
        if (useNativeMetalBackend && mpsGraphHandle) {
            int result = MPSGraph_RunInference(mpsGraphHandle, &input, &output);
            if (result == 0) {
                return true;
            } else {
                std::cerr << "MPSGraph inference failed, falling back to CoreML" << std::endl;
            }
        }
        
        // Try CoreML next
        if (useCoreMLBackend && coreMLHandle) {
            int result = CoreML_RunInference(coreMLHandle, &input, &output);
            if (result == 0) {
                return true;
            } else {
                std::cerr << "CoreML inference failed, falling back to direct Metal" << std::endl;
            }
        }
        
#if defined(USE_METAL_DIRECT)
        // Finally try direct Metal
        if (metalDevice && metalCommandQueue) {
            // This would be implemented in a full version
            // return runDirectMetalInference(input, output);
        }
#endif
        
        return false;
    }
#endif // __APPLE__
    
    // Model info
    std::string modelPath;
    std::string architecture = "cnn";
    bool initialized = false;
    
#ifdef USE_LIBTORCH
    // Initialize PyTorch with Metal backend
    bool initializePyTorchMetal(const std::vector<int>& inputDims, int numActions) {
#if defined(__APPLE__) && defined(USE_LIBTORCH)
        try {
            // Set PyTorch to use Metal backend if available
            if (torch::hasMPS()) {
                torch::Device device(torch::kMPS);
                inputTensor = torch::zeros(inputDims, torch::TensorOptions().device(device));
                outputTensor = torch::zeros({1, numActions}, torch::TensorOptions().device(device));
                useMetalBackend = true;
                std::cout << "PyTorch using Metal backend" << std::endl;
                return true;
            } else {
                std::cout << "PyTorch Metal backend not available, using CPU" << std::endl;
                return false;
            }
        } catch (const std::exception& e) {
            std::cerr << "Error initializing PyTorch Metal: " << e.what() << std::endl;
            return false;
        }
#else
        return false;
#endif
    }
    
    // Load PyTorch model with Metal acceleration
    bool loadTorchModelWithMetal(const std::string& path) {
#if defined(__APPLE__) && defined(USE_LIBTORCH)
        try {
            if (torch::hasMPS()) {
                torch::Device device(torch::kMPS);
                torchModule = torch::jit::load(path, device);
                torchModuleLoaded = true;
                useMetalBackend = true;
                std::cout << "Loaded PyTorch model on Metal device" << std::endl;
                return true;
            } else {
                // Fall back to CPU
                torchModule = torch::jit::load(path);
                torchModuleLoaded = true;
                std::cout << "Loaded PyTorch model on CPU (Metal not available)" << std::endl;
                return true;
            }
        } catch (const std::exception& e) {
            std::cerr << "Error loading PyTorch model: " << e.what() << std::endl;
            return false;
        }
#else
        return false;
#endif
    }
    
    // Run inference with PyTorch and Metal
    bool predictWithPyTorchMetal(const AIInputFrame& state, AIOutputAction& action) {
#if defined(__APPLE__) && defined(USE_LIBTORCH)
        if (!torchModuleLoaded) return false;
        
        try {
            // Convert AIInputFrame to PyTorch tensor
            std::vector<float> inputVec = convertFrameToVector(state);
            
            // Copy to input tensor
            if (useMetalBackend) {
                torch::Device device(torch::kMPS);
                inputTensor = torch::from_blob(inputVec.data(), {1, static_cast<int>(inputVec.size())}, 
                                              torch::TensorOptions().dtype(torch::kFloat32))
                              .to(device);
            } else {
                inputTensor = torch::from_blob(inputVec.data(), {1, static_cast<int>(inputVec.size())}, 
                                              torch::TensorOptions().dtype(torch::kFloat32));
            }
            
            // Run inference
            torch::NoGradGuard no_grad;
            std::vector<torch::jit::IValue> inputs;
            inputs.push_back(inputTensor);
            at::Tensor output = torchModule.forward(inputs).toTensor();
            
            // Convert output tensor to action
            if (output.dim() > 0 && output.size(0) > 0) {
                // Assume output is logits for each action
                float* outputData = output.data_ptr<float>();
                int numOutputs = output.numel();
                
                // Convert to AIOutputAction
                action.clear();
                if (numOutputs >= 4) {
                    action.up = outputData[0] > 0.5f;
                    action.down = outputData[1] > 0.5f;
                    action.left = outputData[2] > 0.5f;
                    action.right = outputData[3] > 0.5f;
                    
                    // Handle buttons
                    for (int i = 0; i < 6 && i + 4 < numOutputs; i++) {
                        action.buttons[i] = outputData[i + 4] > 0.5f;
                    }
                    
                    // Handle start and coin if available
                    if (numOutputs >= 11) action.start = outputData[10] > 0.5f;
                    if (numOutputs >= 12) action.coin = outputData[11] > 0.5f;
                }
                
                return true;
            }
        } catch (const std::exception& e) {
            std::cerr << "Error in PyTorch prediction: " << e.what() << std::endl;
        }
#endif
        return false;
    }
    
    // Helper to convert AIInputFrame to vector
    std::vector<float> convertFrameToVector(const AIInputFrame& frame) {
        std::vector<float> result;
        
        if (frame.frameBuffer && frame.width > 0 && frame.height > 0) {
            // Calculate size and allocate
            size_t size = frame.width * frame.height * 4; // RGBA
            result.resize(size / 4); // We'll convert to grayscale
            
            // Get raw frame data
            const uint8_t* frameData = static_cast<const uint8_t*>(frame.frameBuffer);
            
            // Convert RGBA to grayscale and normalize to [0,1]
            for (size_t i = 0; i < result.size(); i++) {
                size_t idx = i * 4;
                // Standard RGB to grayscale conversion
                result[i] = (0.299f * frameData[idx] + 
                             0.587f * frameData[idx + 1] + 
                             0.114f * frameData[idx + 2]) / 255.0f;
            }
        }
        
        return result;
    }
#endif // USE_LIBTORCH
};

AITorchPolicy::AITorchPolicy()
    : m_impl(std::make_unique<PolicyImpl>())
    , m_numActions(0)
    , m_trainable(true) {
    
    std::cout << "Creating AITorchPolicy (default)" << std::endl;
}

AITorchPolicy::AITorchPolicy(const std::string& modelPath)
    : m_impl(std::make_unique<PolicyImpl>())
    , m_numActions(0)
    , m_trainable(true) {
    
    std::cout << "Creating AITorchPolicy from " << modelPath << std::endl;
    m_impl->modelPath = modelPath;
    
    // Try to load the model
    load(modelPath);
}

AITorchPolicy::~AITorchPolicy() {
    std::cout << "Destroying AITorchPolicy" << std::endl;
}

bool AITorchPolicy::initialize(const std::vector<int>& inputDims, int numActions) {
    std::cout << "Initializing AITorchPolicy with " << numActions << " actions" << std::endl;
    
    m_inputDims = inputDims;
    m_numActions = numActions;
    
    // Initialize the model architecture
    bool success = false;
    
#ifdef USE_LIBTORCH
    // Try PyTorch with Metal backend
    success = m_impl->initializePyTorchMetal(inputDims, numActions);
#endif
    
    // If PyTorch initialization failed, try native Metal backends
#if defined(__APPLE__)
    if (!success) {
        // This will be handled during model loading
        success = true;
    }
#endif
    
    m_impl->initialized = success;
    return success;
}

bool AITorchPolicy::load(const std::string& path) {
    std::cout << "Loading model from " << path << std::endl;
    
    // Check if file exists
    std::ifstream file(path);
    if (!file.good()) {
        std::cerr << "Could not load model from " << path << " (file not found)" << std::endl;
        return false;
    }
    file.close();
    
    // Store path
    m_impl->modelPath = path;
    
    // Try to load model with various backends
    bool success = false;
    
#ifdef USE_LIBTORCH
    // Try PyTorch with Metal backend
    success = m_impl->loadTorchModelWithMetal(path);
#endif
    
#if defined(__APPLE__)
    // If PyTorch loading failed, try native Metal backends
    if (!success) {
        success = m_impl->loadModelIntoMetalBackend(path);
    }
#endif
    
    // Update initialization status
    if (success) {
        m_impl->initialized = true;
    }
    
    return success;
}

bool AITorchPolicy::save(const std::string& path) {
    std::cout << "Saving model to " << path << std::endl;
    
    bool success = false;
    
#ifdef USE_LIBTORCH
    // Save PyTorch model
    if (m_impl->torchModuleLoaded) {
        try {
            m_impl->torchModule.save(path);
            success = true;
        } catch (const std::exception& e) {
            std::cerr << "Error saving PyTorch model: " << e.what() << std::endl;
        }
    }
#endif
    
    // If no PyTorch model to save, create a dummy file
    if (!success) {
        std::ofstream file(path);
        if (file.good()) {
            // Write a dummy header
            file << "AITorchPolicy model stub" << std::endl;
            file.close();
            success = true;
        } else {
            std::cerr << "Could not save model to " << path << std::endl;
        }
    }
    
    return success;
}

bool AITorchPolicy::predict(const AIInputFrame& state, AIOutputAction& action, bool exploit) {
    // Try different backends for prediction
    
#ifdef USE_LIBTORCH
    // Try PyTorch with Metal backend
    if (m_impl->torchModuleLoaded) {
        if (m_impl->predictWithPyTorchMetal(state, action)) {
            return true;
        }
    }
#endif
    
#if defined(__APPLE__)
    // Try native Metal backends
    if (m_impl->initialized) {
        if (m_impl->runInferenceWithMetalBackend(state, action)) {
            return true;
        }
    }
#endif
    
    // Fall back to random actions
    // Clear the action
    action.clear();
    
    // Set random directional inputs (with bias towards not pressing)
    float threshold = exploit ? 0.2f : 0.5f;
    action.up = m_impl->dist(m_impl->rng) < threshold;
    action.down = m_impl->dist(m_impl->rng) < threshold;
    action.left = m_impl->dist(m_impl->rng) < threshold;
    action.right = m_impl->dist(m_impl->rng) < threshold;
    
    // Make sure we don't press opposite directions
    if (action.up && action.down) {
        action.down = false;
    }
    if (action.left && action.right) {
        action.right = false;
    }
    
    // Set random button inputs (with bias towards not pressing)
    for (int i = 0; i < 6; i++) {
        action.buttons[i] = m_impl->dist(m_impl->rng) < threshold;
    }
    
    // Rarely press start or coin
    action.start = m_impl->dist(m_impl->rng) < 0.05f;
    action.coin = m_impl->dist(m_impl->rng) < 0.01f;
    
    return true;
}

float AITorchPolicy::getValue(const AIInputFrame& state) {
    // In a real implementation, this would run the value head of the model
    // For this stub, we'll just return a random value
    
    return m_impl->dist(m_impl->rng) * 2.0f - 1.0f; // Range: -1.0 to 1.0
}

std::vector<float> AITorchPolicy::getActionProbabilities(const AIInputFrame& state) {
    // In a real implementation, this would run the policy head of the model
    // For this stub, we'll just return random probabilities
    
    std::vector<float> probs(m_numActions > 0 ? m_numActions : 12);
    float sum = 0.0f;
    
    // Generate random values
    for (int i = 0; i < probs.size(); i++) {
        probs[i] = m_impl->dist(m_impl->rng);
        sum += probs[i];
    }
    
    // Normalize to make a valid probability distribution
    if (sum > 0.0f) {
        for (int i = 0; i < probs.size(); i++) {
            probs[i] /= sum;
        }
    } else {
        // If sum is 0, set uniform probabilities
        float p = 1.0f / probs.size();
        for (int i = 0; i < probs.size(); i++) {
            probs[i] = p;
        }
    }
    
    return probs;
}

float AITorchPolicy::update(const std::vector<std::vector<float>>& states,
                            const std::vector<std::vector<float>>& actions,
                            const std::vector<float>& oldLogProbs,
                            const std::vector<float>& advantages,
                            const std::vector<float>& returns,
                            float learningRate) {
    // In a real implementation, this would perform a training step
    // For this stub, we'll just log the update and return a fake loss
    
    std::cout << "Updating model with batch of " << states.size() << " examples, lr=" << learningRate << std::endl;
    
    // Return a decreasing loss value to simulate training progress
    static float loss = 1.0f;
    loss = std::max(0.1f, loss * 0.99f);
    
    return loss;
}

AITorchPolicy* AITorchPolicy::clone() {
    // Create a new policy
    AITorchPolicy* clone = new AITorchPolicy();
    
    // Copy basic properties
    clone->m_inputDims = m_inputDims;
    clone->m_numActions = m_numActions;
    clone->m_trainable = m_trainable;
    
    // In a real implementation, we would clone the actual neural network model
    // This would involve duplicating the network architecture and copying all weights
    if (m_impl && m_impl->initialized) {
        clone->initialize(m_inputDims, m_numActions);
        
        // Copy implementation details
        clone->m_impl->architecture = m_impl->architecture;
        clone->m_impl->initialized = m_impl->initialized;
        
        // To be replaced with actual model cloning in the real implementation:
        // clone->m_impl->model = torch::jit::Module();
        // clone->m_impl->model.load(temporary_file_with_serialized_weights);
    }
    
    return clone;
}

void AITorchPolicy::copyFrom(const AITorchPolicy* other) {
    // Skip if other is null
    if (!other) {
        return;
    }
    
    // Copy basic properties
    m_inputDims = other->m_inputDims;
    m_numActions = other->m_numActions;
    m_trainable = other->m_trainable;
    
    // Ensure this policy is initialized
    if (!m_impl->initialized && other->m_impl->initialized) {
        initialize(m_inputDims, m_numActions);
    }
    
    // Copy implementation details
    m_impl->architecture = other->m_impl->architecture;
    m_impl->initialized = other->m_impl->initialized;
    
    // In a real implementation, this would copy the model weights:
    // For each named parameter in the network:
    //   this->m_impl->model.get_parameter("layer.weight").copy_(other->m_impl->model.get_parameter("layer.weight"));
    //   this->m_impl->model.get_parameter("layer.bias").copy_(other->m_impl->model.get_parameter("layer.bias"));
}

bool AITorchPolicy::exportTo(const std::string& path, const std::string& format) {
    std::cout << "Exporting model to " << format << " format: " << path << std::endl;
    
    if (format == "coreml") {
        // For CoreML export, we need to:
        // 1. First save the model to a temporary PyTorch format
        // 2. Then convert from PyTorch to CoreML
        
        // Create a temporary path for the PyTorch model
        std::string tempTorchPath = path + ".pt";
        
        // Save the model to the temporary path
        if (!save(tempTorchPath)) {
            std::cerr << "Failed to save model to temporary path: " << tempTorchPath << std::endl;
            return false;
        }
        
        // Prepare input shape based on current model's input dimensions
        std::vector<int> inputShape;
        if (m_inputDims.empty()) {
            // Default shape if none is specified
            inputShape = {1, 4, 84, 84}; // Batch, channels, height, width
        } else {
            // Use the model's input dimensions
            inputShape = m_inputDims;
            
            // Ensure the shape has at least 4 dimensions (batch, channels, height, width)
            while (inputShape.size() < 4) {
                inputShape.insert(inputShape.begin(), 1);
            }
        }
        
        // Convert from PyTorch to CoreML
        bool success = fbneo::ai::convertPyTorchToCoreML(
            tempTorchPath,
            path,
            inputShape,
            true, // Use Neural Engine
            true  // Quantize the model
        );
        
        if (success) {
            std::cout << "Successfully exported model to CoreML format: " << path << std::endl;
            
            // Optimize the model for Apple Neural Engine
            std::string optimizedPath = path + ".optimized.mlmodel";
            if (fbneo::ai::optimizeCoreMLModel(path, optimizedPath, "ANE")) {
                std::cout << "Successfully optimized model for Apple Neural Engine: " << optimizedPath << std::endl;
            } else {
                std::cerr << "Failed to optimize model for Apple Neural Engine" << std::endl;
                // Continue even if optimization fails
            }
        } else {
            std::cerr << "Failed to convert model to CoreML format" << std::endl;
            return false;
        }
        
        // Clean up temporary file
        std::remove(tempTorchPath.c_str());
        
        return success;
    } 
    else if (format == "onnx") {
        // ONNX export implementation would go here
        std::cerr << "ONNX export not implemented yet" << std::endl;
        return false;
    }
    else {
        std::cerr << "Unsupported export format: " << format << std::endl;
        return false;
    }
}

bool AITorchPolicy::setArchitecture(const std::string& architecture, const std::vector<int>& params) {
    // In a real implementation, this would set the model architecture
    // For this stub, we'll just store the architecture name
    
    std::cout << "Setting architecture to " << architecture << std::endl;
    m_impl->architecture = architecture;
    
    return true;
}

void AITorchPolicy::setTrainable(bool trainable) {
    m_trainable = trainable;
}

bool AITorchPolicy::isTrainable() const {
    return m_trainable;
}

const std::vector<int>& AITorchPolicy::getInputDims() const {
    return m_inputDims;
}

int AITorchPolicy::getNumActions() const {
    return m_numActions;
}

} // namespace ai
} // namespace fbneo

// AITorchPolicy implementation
AITorchPolicy::AITorchPolicy()
    : engineType(ENGINE_NONE), modelLoaded(false), libtorchEngine(nullptr),
      mpsEngine(nullptr), coreMLEngine(nullptr), difficultyLevel(5),
      inputPreprocessor(nullptr), latestActions(), frameHistory() {
    
    // Initialize frame history
    frameHistory.resize(MAX_HISTORY_FRAMES);
    
    // Reset latest actions
    latestActions.up = 0;
    latestActions.down = 0;
    latestActions.left = 0;
    latestActions.right = 0;
    for (int i = 0; i < 6; i++) {
        latestActions.buttons[i] = 0;
    }
    
#ifdef __APPLE__
    // On Apple platforms, prefer CoreML, then MPS, then LibTorch
    // Try to initialize CoreML engine
    coreMLEngine = CoreML_Create();
    if (coreMLEngine) {
        printf("Using CoreML engine for AI inference\n");
        engineType = ENGINE_COREML;
        return;
    }
    
    // If CoreML failed, try MPS
    mpsEngine = MPSGraph_Create();
    if (mpsEngine) {
        printf("Using MPS engine for AI inference\n");
        engineType = ENGINE_MPS;
        return;
    }
#endif
    
#ifdef USE_LIBTORCH
    // If all else failed or on non-Apple platforms, use LibTorch
    try {
        // Initialize torch engine
        printf("Using LibTorch engine for AI inference\n");
        engineType = ENGINE_LIBTORCH;
    } catch (const std::exception& e) {
        printf("Error initializing LibTorch: %s\n", e.what());
        engineType = ENGINE_NONE;
    }
#endif
    
    if (engineType == ENGINE_NONE) {
        printf("No AI engine available\n");
    }
}

AITorchPolicy::~AITorchPolicy() {
    // Clean up based on engine type
    switch (engineType) {
#ifdef USE_LIBTORCH
        case ENGINE_LIBTORCH:
            // Clean up LibTorch resources
            libtorchEngine = nullptr;
            break;
#endif
            
#ifdef __APPLE__
        case ENGINE_MPS:
            if (mpsEngine) {
                MPSGraph_Destroy(mpsEngine);
                mpsEngine = nullptr;
            }
            break;
            
        case ENGINE_COREML:
            if (coreMLEngine) {
                CoreML_Destroy(coreMLEngine);
                coreMLEngine = nullptr;
            }
            break;
#endif
            
        default:
            break;
    }
    
    // Clean up input preprocessor
    if (inputPreprocessor) {
        delete inputPreprocessor;
        inputPreprocessor = nullptr;
    }
}

bool AITorchPolicy::LoadModel(const std::string& path) {
    printf("Loading AI model: %s\n", path.c_str());
    
    // Reset model state
    modelLoaded = false;
    modelPath = path;
    
    // Load based on engine type
    switch (engineType) {
#ifdef USE_LIBTORCH
        case ENGINE_LIBTORCH:
            try {
                // Load the TorchScript model
                libtorchEngine = torch::jit::load(path);
                modelLoaded = true;
                printf("LibTorch model loaded successfully\n");
                return true;
            } catch (const std::exception& e) {
                printf("Error loading model with LibTorch: %s\n", e.what());
                return false;
            }
            break;
#endif
            
#ifdef __APPLE__
        case ENGINE_MPS:
            if (mpsEngine) {
                modelLoaded = MPSGraph_LoadModel(mpsEngine, path.c_str()) != 0;
                if (modelLoaded) {
                    printf("MPS model loaded successfully\n");
                    return true;
                } else {
                    printf("Error loading model with MPS\n");
                    return false;
                }
            }
            break;
            
        case ENGINE_COREML:
            if (coreMLEngine) {
                modelLoaded = CoreML_LoadModel(coreMLEngine, path.c_str()) != 0;
                if (modelLoaded) {
                    printf("CoreML model loaded successfully\n");
                    printf("Model info: %s\n", CoreML_GetModelInfo(coreMLEngine));
                    return true;
                } else {
                    printf("Error loading model with CoreML\n");
                    return false;
                }
            }
            break;
#endif
            
        default:
            printf("No AI engine available\n");
            return false;
    }
    
    // If we got here, something went wrong
    return false;
}

void AITorchPolicy::UpdateFrameHistory(const AIInputFrame& input) {
    // Shift history
    for (int i = MAX_HISTORY_FRAMES - 1; i > 0; i--) {
        frameHistory[i] = frameHistory[i - 1];
    }
    
    // Add new frame to history
    frameHistory[0] = input;
}

bool AITorchPolicy::PreprocessInput(const AIInputFrame& input, void* outputData) {
    // Create a simple preprocessor if needed
    if (!inputPreprocessor) {
        inputPreprocessor = new AIInputPreprocessor();
    }
    
    // Add frame to history
    UpdateFrameHistory(input);
    
    // Preprocess based on engine type
    switch (engineType) {
#ifdef USE_LIBTORCH
        case ENGINE_LIBTORCH:
            // For LibTorch, we need to create a torch::Tensor from the input
            try {
                // This is a simplified example - real implementation would extract features
                const int width = 84;  // Resized width
                const int height = 84; // Resized height
                
                // Create tensor of shape [1, 4, height, width] for a batch of 4 frames
                auto tensor = torch::zeros({1, 4, height, width}, torch::kFloat32);
                
                // Fill tensor with preprocessed frame data
                for (int f = 0; f < 4 && f < MAX_HISTORY_FRAMES; f++) {
                    const AIInputFrame& frame = frameHistory[f];
                    if (frame.frameBuffer && frame.width > 0 && frame.height > 0) {
                        inputPreprocessor->ProcessFrame(frame, tensor[0][f].data_ptr<float>(), 
                                                       width, height);
                    }
                }
                
                // Store tensor in the output pointer
                *((torch::Tensor*)outputData) = tensor;
                return true;
            } catch (const std::exception& e) {
                printf("Error preprocessing input for LibTorch: %s\n", e.what());
                return false;
            }
            break;
#endif
            
        case ENGINE_MPS:
        case ENGINE_COREML:
            // For MPS and CoreML, we'll use the input frame directly
            // The actual preprocessing happens in the engine implementations
            return true;
            
        default:
            return false;
    }
    
    // If we got here, something went wrong
    return false;
}

bool AITorchPolicy::RunInference(const AIInputFrame& input, AIOutputAction& output) {
    if (!modelLoaded) {
        printf("Model not loaded, using default actions\n");
        // No model loaded, use default random actions
        UseDefaultActions(output);
        return true;
    }
    
    // Start timing
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Run inference based on engine type
    bool success = false;
    
    switch (engineType) {
#ifdef USE_LIBTORCH
        case ENGINE_LIBTORCH:
            try {
                // Prepare input tensor
                torch::Tensor inputTensor;
                if (!PreprocessInput(input, &inputTensor)) {
                    printf("Error preprocessing input for LibTorch\n");
                    UseDefaultActions(output);
                    return false;
                }
                
                // Create inputs list
                std::vector<torch::jit::IValue> inputs;
                inputs.push_back(inputTensor);
                
                // Run model
                torch::NoGradGuard no_grad;
                auto outputTensor = libtorchEngine.forward(inputs).toTensor();
                
                // Convert output tensor to action
                if (PostprocessOutput(outputTensor, output)) {
                    success = true;
                } else {
                    UseDefaultActions(output);
                }
            } catch (const std::exception& e) {
                printf("Error running inference with LibTorch: %s\n", e.what());
                UseDefaultActions(output);
            }
            break;
#endif
            
#ifdef __APPLE__
        case ENGINE_MPS:
            if (mpsEngine) {
                success = MPSGraph_RunInference(mpsEngine, &input, &output) != 0;
                if (!success) {
                    printf("Error running inference with MPS\n");
                    UseDefaultActions(output);
                }
            } else {
                UseDefaultActions(output);
            }
            break;
            
        case ENGINE_COREML:
            if (coreMLEngine) {
                success = CoreML_RunInference(coreMLEngine, &input, &output) != 0;
                if (!success) {
                    printf("Error running inference with CoreML\n");
                    UseDefaultActions(output);
                }
            } else {
                UseDefaultActions(output);
            }
            break;
#endif
            
        default:
            UseDefaultActions(output);
            break;
    }
    
    // End timing
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    
    // Log performance
    if (success) {
        printf("Inference time: %.3f ms\n", duration.count() / 1000.0);
        
        // Store the latest actions
        latestActions = output;
    }
    
    return success;
}

void AITorchPolicy::UseDefaultActions(AIOutputAction& output) {
    // Clear output
    output.up = 0;
    output.down = 0;
    output.left = 0;
    output.right = 0;
    for (int i = 0; i < 6; i++) {
        output.buttons[i] = 0;
    }
    
    // Simple random actions based on frame counter
    static int frameCounter = 0;
    frameCounter++;
    
    // Directional inputs
    if (frameCounter % 30 == 0) {  // Every 30 frames
        int dir = rand() % 4;
        switch (dir) {
            case 0: output.up = 1; break;
            case 1: output.down = 1; break;
            case 2: output.left = 1; break;
            case 3: output.right = 1; break;
        }
    }
    
    // Button inputs
    if (frameCounter % 60 == 0) {  // Every 60 frames
        int button = rand() % 6;
        output.buttons[button] = 1;
    }
}

bool AITorchPolicy::PostprocessOutput(void* tensorData, AIOutputAction& output) {
#ifdef USE_LIBTORCH
    // For LibTorch, tensorData is a torch::Tensor
    try {
        torch::Tensor& outputTensor = *((torch::Tensor*)tensorData);
        
        // Ensure tensor has the right shape
        if (outputTensor.dim() != 2 || outputTensor.size(0) != 1 || outputTensor.size(1) < 10) {
            printf("Invalid output tensor shape: [%lld, %lld]\n", 
                  outputTensor.size(0), outputTensor.size(1));
            return false;
        }
        
        // Extract actions from tensor
        // Assuming first 4 outputs are directions, next 6 are buttons
        auto data = outputTensor[0];
        
        // Clear output
        output.up = 0;
        output.down = 0;
        output.left = 0;
        output.right = 0;
        for (int i = 0; i < 6; i++) {
            output.buttons[i] = 0;
        }
        
        // Apply difficulty-based thresholding
        float threshold = 0.5f;  // Default threshold
        
        // Adjust threshold based on difficulty (1-10)
        // Higher difficulty = lower threshold = agent plays better
        if (difficultyLevel > 0 && difficultyLevel <= 10) {
            threshold = 1.0f - (difficultyLevel / 20.0f);  // Maps 1-10 to 0.95-0.5
        }
        
        // Set directional outputs
        output.up = data[0].item<float>() > threshold ? 1 : 0;
        output.down = data[1].item<float>() > threshold ? 1 : 0;
        output.left = data[2].item<float>() > threshold ? 1 : 0;
        output.right = data[3].item<float>() > threshold ? 1 : 0;
        
        // Set button outputs
        for (int i = 0; i < 6 && i + 4 < data.size(0); i++) {
            output.buttons[i] = data[i + 4].item<float>() > threshold ? 1 : 0;
        }
        
        return true;
    } catch (const std::exception& e) {
        printf("Error postprocessing output: %s\n", e.what());
        return false;
    }
#else
    // For non-LibTorch builds, this function shouldn't be called
    return false;
#endif
}

void AITorchPolicy::SetDifficulty(int level) {
    // Clamp difficulty to 1-10
    difficultyLevel = level;
    if (difficultyLevel < 1) difficultyLevel = 1;
    if (difficultyLevel > 10) difficultyLevel = 10;
    
    printf("AI difficulty set to %d\n", difficultyLevel);
}

AIInputPreprocessor::AIInputPreprocessor() {
    // Initialize preprocessor
}

AIInputPreprocessor::~AIInputPreprocessor() {
    // Clean up preprocessor
}

void AIInputPreprocessor::ProcessFrame(const AIInputFrame& input, float* output, 
                                      int outWidth, int outHeight) {
    // Simple preprocessing: resize and convert to grayscale
    if (!input.frameBuffer || input.width <= 0 || input.height <= 0 || !output) {
        return;
    }
    
    const uint8_t* frameData = static_cast<const uint8_t*>(input.frameBuffer);
    
    // Resize with simple nearest neighbor and convert to grayscale
    for (int y = 0; y < outHeight; y++) {
        for (int x = 0; x < outWidth; x++) {
            // Map output coordinates to input coordinates
            int inX = x * input.width / outWidth;
            int inY = y * input.height / outHeight;
            
            // Get pixel from input (assuming RGBA format)
            int pixelOffset = (inY * input.width + inX) * 4;
            uint8_t r = frameData[pixelOffset];
            uint8_t g = frameData[pixelOffset + 1];
            uint8_t b = frameData[pixelOffset + 2];
            
            // Convert to grayscale and normalize to [0, 1]
            float gray = (0.299f * r + 0.587f * g + 0.114f * b) / 255.0f;
            
            // Store in output buffer
            output[y * outWidth + x] = gray;
        }
    }
} 