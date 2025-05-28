#include "ai_rl_algorithms.h"
#include "ai_torch_policy.h"
#include <cmath>
#include <iostream>
#include <fstream>
#include <algorithm>

// Add Metal imports
#pragma once
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>

namespace fbneo {
namespace ai {

//------------------------------------------------------------------------------
// ICM Module Implementation
//------------------------------------------------------------------------------

class ICMModule {
public:
    ICMModule(AITorchPolicy* policy);
    ~ICMModule();
    
    bool initialize();
    float calculateIntrinsicReward(const AIInputFrame& state, const AIOutputAction& action, 
                                  const AIInputFrame& next_state);
    void update(const std::vector<Experience>& batch);
    void setRewardScale(float scale);
    float getRewardScale() const;
    bool save(const std::string& path);
    bool load(const std::string& path);
    
private:
    AITorchPolicy* policy;
    float rewardScale;
    float forwardLossWeight;
    float inverseLossWeight;
    
    // Metal-specific members
    id<MTLDevice> metalDevice = nil;
    id<MTLCommandQueue> metalCommandQueue = nil;
    id<MTLLibrary> metalLibrary = nil;
    id<MTLComputePipelineState> forwardModelPipeline = nil;
    id<MTLComputePipelineState> inverseModelPipeline = nil;
    id<MTLComputePipelineState> encodeStatePipeline = nil;
    id<MTLComputePipelineState> forwardLossPipeline = nil;
    id<MTLComputePipelineState> inverseLossPipeline = nil;
    
    // Metal buffers
    id<MTLBuffer> stateFeaturesBuffer = nil;
    id<MTLBuffer> nextStateFeaturesBuffer = nil;
    id<MTLBuffer> predictedNextFeaturesBuffer = nil;
    id<MTLBuffer> actionBuffer = nil;
    id<MTLBuffer> predictedActionBuffer = nil;
    id<MTLBuffer> dimensionsBuffer = nil;
    id<MTLBuffer> forwardLossBuffer = nil;
    id<MTLBuffer> inverseLossBuffer = nil;
    
    // Metal acceleration methods
    void initializeMetalResources();
    void cleanupMetalResources();
    std::vector<float> encodeStateWithMetal(const AIInputFrame& state);
    std::vector<float> predictNextStateWithMetal(const std::vector<float>& stateFeatures, const AIOutputAction& action);
    std::vector<float> predictActionWithMetal(const std::vector<float>& stateFeatures, const std::vector<float>& nextStateFeatures);
    float calculateIntrinsicRewardWithMetal(const AIInputFrame& state, const AIOutputAction& action, const AIInputFrame& next_state);
    void updateWithMetal(const std::vector<Experience>& batch);
    float calculateForwardLossWithMetal(const std::vector<float>& nextStateFeatures, const std::vector<float>& predictedNextStateFeatures);
    float calculateInverseLossWithMetal(const std::vector<float>& action, const std::vector<float>& predictedAction);
    
    // Helper methods
    std::vector<float> encodeState(const AIInputFrame& state);
    std::vector<float> predictNextState(const std::vector<float>& stateFeatures, const AIOutputAction& action);
    std::vector<float> predictAction(const std::vector<float>& stateFeatures, const std::vector<float>& nextStateFeatures);
    AIInputFrame convertVectorToInputFrame(const std::vector<float>& vec);
    AIOutputAction convertVectorToAction(const std::vector<float>& vec);
};

ICMModule::ICMModule(AITorchPolicy* policy)
    : policy(policy), rewardScale(0.01f), forwardLossWeight(0.8f), inverseLossWeight(0.2f) {
    // ICM constructor
    
    // Initialize Metal resources
    initializeMetalResources();
}

ICMModule::~ICMModule() {
    // Cleanup Metal resources
    cleanupMetalResources();
}

bool ICMModule::initialize() {
    // Create feature network (shared encoder)
    // In a real implementation, this would be initialized with actual policy
    // featureNetwork = std::make_unique<AITorchPolicy>();
    
    // Create forward model
    // forwardModel = std::make_unique<AITorchPolicy>();
    
    // Create inverse model
    // inverseModel = std::make_unique<AITorchPolicy>();
    
    // Initialize Metal resources if not already done
    if (metalDevice == nil) {
        initializeMetalResources();
    }
    
    return true;
}

float ICMModule::calculateIntrinsicReward(const AIInputFrame& state, const AIOutputAction& action, 
                                         const AIInputFrame& next_state) {
    // Check if Metal resources are available
    if (metalCommandQueue && forwardModelPipeline && forwardLossPipeline) {
        return calculateIntrinsicRewardWithMetal(state, action, next_state);
    }
    
    // Fall back to CPU implementation if Metal not available
    
    // Encode current state
    std::vector<float> stateFeatures = encodeState(state);
    
    // Encode next state
    std::vector<float> nextStateFeatures = encodeState(next_state);
    
    // Predict next state features
    std::vector<float> predictedNextStateFeatures = predictNextState(stateFeatures, action);
    
    // Calculate forward loss (prediction error)
    float forwardLoss = 0.0f;
    for (size_t i = 0; i < nextStateFeatures.size() && i < predictedNextStateFeatures.size(); ++i) {
        float diff = nextStateFeatures[i] - predictedNextStateFeatures[i];
        forwardLoss += diff * diff;
    }
    forwardLoss = std::sqrt(forwardLoss) / nextStateFeatures.size();
    
    // Prediction error is the intrinsic reward
    return forwardLoss * rewardScale;
}

float ICMModule::calculateIntrinsicRewardWithMetal(const AIInputFrame& state, const AIOutputAction& action, 
                                                const AIInputFrame& next_state) {
    // Encode states using Metal
    std::vector<float> stateFeatures = encodeStateWithMetal(state);
    std::vector<float> nextStateFeatures = encodeStateWithMetal(next_state);
    
    // Predict next state features using Metal
    std::vector<float> predictedNextStateFeatures = predictNextStateWithMetal(stateFeatures, action);
    
    // Copy next state features to Metal buffer
    memcpy([nextStateFeaturesBuffer contents], nextStateFeatures.data(), nextStateFeatures.size() * sizeof(float));
    
    // Copy predicted next state features to Metal buffer
    memcpy([predictedNextFeaturesBuffer contents], predictedNextStateFeatures.data(), 
           predictedNextStateFeatures.size() * sizeof(float));
    
    // Create command buffer for forward loss calculation
    id<MTLCommandBuffer> commandBuffer = [metalCommandQueue commandBuffer];
    id<MTLComputeCommandEncoder> encoder = [commandBuffer computeCommandEncoder];
    
    // Set the compute pipeline
    [encoder setComputePipelineState:forwardLossPipeline];
    
    // Set the buffers
    [encoder setBuffer:nextStateFeaturesBuffer offset:0 atIndex:0];
    [encoder setBuffer:predictedNextFeaturesBuffer offset:0 atIndex:1];
    [encoder setBuffer:forwardLossBuffer offset:0 atIndex:2];
    
    // Calculate thread groups
    NSUInteger threadGroupSize = forwardLossPipeline.maxTotalThreadsPerThreadgroup;
    if (threadGroupSize > 256) threadGroupSize = 256;
    
    // Dispatch threads
    [encoder dispatchThreads:MTLSizeMake(256, 1, 1)
      threadsPerThreadgroup:MTLSizeMake(threadGroupSize, 1, 1)];
    
    // End encoding and commit
    [encoder endEncoding];
    [commandBuffer commit];
    [commandBuffer waitUntilCompleted];
    
    // Read back loss values
    float* lossValues = static_cast<float*>([forwardLossBuffer contents]);
    
    // Calculate total loss
    float totalLoss = 0.0f;
    for (int i = 0; i < 256; ++i) {
        totalLoss += lossValues[i];
    }
    
    // Compute mean squared error
    float mse = totalLoss / 256.0f;
    
    // Return RMSE scaled by reward scale
    return std::sqrt(mse) * rewardScale;
}

void ICMModule::update(const std::vector<Experience>& batch) {
    // Check if Metal resources are available
    if (metalCommandQueue && forwardModelPipeline && inverseModelPipeline && 
        forwardLossPipeline && inverseLossPipeline && !batch.empty()) {
        updateWithMetal(batch);
        return;
    }
    
    // Fall back to CPU implementation if Metal not available
    
    // Calculate total losses for batch
    float totalForwardLoss = 0.0f;
    float totalInverseLoss = 0.0f;
    
    for (const auto& exp : batch) {
        // Encode states
        std::vector<float> stateFeatures;
        std::vector<float> nextStateFeatures;
        
        // In a real implementation, this would use the actual feature network
        // stateFeatures = featureNetwork->predict(exp.state);
        // nextStateFeatures = featureNetwork->predict(exp.next_state);
        
        // For this stub, we'll just use the first few elements
        const size_t featureSize = std::min<size_t>(100, exp.state.size());
        stateFeatures.resize(featureSize);
        nextStateFeatures.resize(featureSize);
        
        for (size_t i = 0; i < featureSize; ++i) {
            stateFeatures[i] = i < exp.state.size() ? exp.state[i] : 0.0f;
            nextStateFeatures[i] = i < exp.next_state.size() ? exp.next_state[i] : 0.0f;
        }
        
        // Convert action vector to AIOutputAction
        AIOutputAction action;
        if (exp.action.size() >= 10) {
            action.up = exp.action[0] > 0.5f;
            action.down = exp.action[1] > 0.5f;
            action.left = exp.action[2] > 0.5f;
            action.right = exp.action[3] > 0.5f;
            for (int i = 0; i < 6 && i + 4 < exp.action.size(); ++i) {
                action.buttons[i] = exp.action[i + 4] > 0.5f;
            }
        }
        
        // Predict next state features using forward model
        std::vector<float> predictedNextStateFeatures = predictNextState(stateFeatures, action);
        
        // Calculate forward loss (prediction error)
        float forwardLoss = 0.0f;
        for (size_t i = 0; i < nextStateFeatures.size() && i < predictedNextStateFeatures.size(); ++i) {
            float diff = nextStateFeatures[i] - predictedNextStateFeatures[i];
            forwardLoss += diff * diff;
        }
        forwardLoss = forwardLoss / nextStateFeatures.size();
        
        // Predict action using inverse model
        std::vector<float> predictedAction = predictAction(stateFeatures, nextStateFeatures);
        
        // Calculate inverse loss
        float inverseLoss = 0.0f;
        for (size_t i = 0; i < exp.action.size() && i < predictedAction.size(); ++i) {
            float diff = exp.action[i] - predictedAction[i];
            inverseLoss += diff * diff;
        }
        inverseLoss = inverseLoss / exp.action.size();
        
        // Accumulate losses
        totalForwardLoss += forwardLoss;
        totalInverseLoss += inverseLoss;
    }
    
    // Calculate average losses
    float avgForwardLoss = totalForwardLoss / batch.size();
    float avgInverseLoss = totalInverseLoss / batch.size();
    
    // Calculate combined loss
    float combinedLoss = forwardLossWeight * avgForwardLoss + inverseLossWeight * avgInverseLoss;
    
    // In a real implementation, we would update the feature, forward, and inverse networks
    // using the gradients of the combined loss
    
    std::cout << "ICM Update - Forward Loss: " << avgForwardLoss 
              << ", Inverse Loss: " << avgInverseLoss 
              << ", Combined Loss: " << combinedLoss << std::endl;
}

void ICMModule::updateWithMetal(const std::vector<Experience>& batch) {
    float totalForwardLoss = 0.0f;
    float totalInverseLoss = 0.0f;
    
    // Process each experience in the batch
    for (const auto& exp : batch) {
        // Convert states to AIInputFrame
        AIInputFrame state = convertVectorToInputFrame(exp.state);
        AIInputFrame nextState = convertVectorToInputFrame(exp.next_state);
        
        // Convert action vector to AIOutputAction
        AIOutputAction action = convertVectorToAction(exp.action);
        
        // Encode states using Metal
        std::vector<float> stateFeatures = encodeStateWithMetal(state);
        std::vector<float> nextStateFeatures = encodeStateWithMetal(nextState);
        
        // Predict next state features using Metal
        std::vector<float> predictedNextStateFeatures = predictNextStateWithMetal(stateFeatures, action);
        
        // Predict action using Metal
        std::vector<float> predictedAction = predictActionWithMetal(stateFeatures, nextStateFeatures);
        
        // Calculate forward loss
        float forwardLoss = calculateForwardLossWithMetal(nextStateFeatures, predictedNextStateFeatures);
        
        // Calculate inverse loss
        float inverseLoss = calculateInverseLossWithMetal(exp.action, predictedAction);
        
        // Accumulate losses
        totalForwardLoss += forwardLoss;
        totalInverseLoss += inverseLoss;
        
        // Clean up temporary frames
        if (state.frameBuffer) free(state.frameBuffer);
        if (nextState.frameBuffer) free(nextState.frameBuffer);
    }
    
    // Calculate average losses
    float avgForwardLoss = totalForwardLoss / batch.size();
    float avgInverseLoss = totalInverseLoss / batch.size();
    
    // Calculate combined loss
    float combinedLoss = forwardLossWeight * avgForwardLoss + inverseLossWeight * avgInverseLoss;
    
    std::cout << "ICM Update (Metal) - Forward Loss: " << avgForwardLoss 
              << ", Inverse Loss: " << avgInverseLoss 
              << ", Combined Loss: " << combinedLoss << std::endl;
    
    // In a real implementation, we would update the feature, forward, and inverse networks
    // using the gradients of the combined loss
}

// Helper method to calculate forward loss with Metal
float ICMModule::calculateForwardLossWithMetal(const std::vector<float>& nextStateFeatures, 
                                             const std::vector<float>& predictedNextStateFeatures) {
    // Copy next state features to Metal buffer
    memcpy([nextStateFeaturesBuffer contents], nextStateFeatures.data(), 
           std::min(nextStateFeatures.size(), static_cast<size_t>(256)) * sizeof(float));
    
    // Copy predicted next state features to Metal buffer
    memcpy([predictedNextFeaturesBuffer contents], predictedNextStateFeatures.data(), 
           std::min(predictedNextStateFeatures.size(), static_cast<size_t>(256)) * sizeof(float));
    
    // Create command buffer
    id<MTLCommandBuffer> commandBuffer = [metalCommandQueue commandBuffer];
    id<MTLComputeCommandEncoder> encoder = [commandBuffer computeCommandEncoder];
    
    // Set the compute pipeline
    [encoder setComputePipelineState:forwardLossPipeline];
    
    // Set the buffers
    [encoder setBuffer:nextStateFeaturesBuffer offset:0 atIndex:0];
    [encoder setBuffer:predictedNextFeaturesBuffer offset:0 atIndex:1];
    [encoder setBuffer:forwardLossBuffer offset:0 atIndex:2];
    
    // Calculate thread groups
    NSUInteger threadGroupSize = forwardLossPipeline.maxTotalThreadsPerThreadgroup;
    if (threadGroupSize > 256) threadGroupSize = 256;
    
    // Dispatch threads
    [encoder dispatchThreads:MTLSizeMake(256, 1, 1)
      threadsPerThreadgroup:MTLSizeMake(threadGroupSize, 1, 1)];
    
    // End encoding and commit
    [encoder endEncoding];
    [commandBuffer commit];
    [commandBuffer waitUntilCompleted];
    
    // Read back loss values
    float* lossValues = static_cast<float*>([forwardLossBuffer contents]);
    
    // Calculate total loss
    float totalLoss = 0.0f;
    for (int i = 0; i < 256; ++i) {
        totalLoss += lossValues[i];
    }
    
    // Return mean squared error
    return totalLoss / 256.0f;
}

// Helper method to calculate inverse loss with Metal
float ICMModule::calculateInverseLossWithMetal(const std::vector<float>& action, 
                                             const std::vector<float>& predictedAction) {
    // Copy action to Metal buffer
    memcpy([actionBuffer contents], action.data(), 
           std::min(action.size(), static_cast<size_t>(10)) * sizeof(float));
    
    // Copy predicted action to Metal buffer
    memcpy([predictedActionBuffer contents], predictedAction.data(), 
           std::min(predictedAction.size(), static_cast<size_t>(10)) * sizeof(float));
    
    // Create command buffer
    id<MTLCommandBuffer> commandBuffer = [metalCommandQueue commandBuffer];
    id<MTLComputeCommandEncoder> encoder = [commandBuffer computeCommandEncoder];
    
    // Set the compute pipeline
    [encoder setComputePipelineState:inverseLossPipeline];
    
    // Set the buffers
    [encoder setBuffer:actionBuffer offset:0 atIndex:0];
    [encoder setBuffer:predictedActionBuffer offset:0 atIndex:1];
    [encoder setBuffer:inverseLossBuffer offset:0 atIndex:2];
    
    // Calculate thread groups
    NSUInteger threadGroupSize = inverseLossPipeline.maxTotalThreadsPerThreadgroup;
    if (threadGroupSize > 10) threadGroupSize = 10;
    
    // Dispatch threads
    [encoder dispatchThreads:MTLSizeMake(10, 1, 1)
      threadsPerThreadgroup:MTLSizeMake(threadGroupSize, 1, 1)];
    
    // End encoding and commit
    [encoder endEncoding];
    [commandBuffer commit];
    [commandBuffer waitUntilCompleted];
    
    // Read back loss values
    float* lossValues = static_cast<float*>([inverseLossBuffer contents]);
    
    // Calculate total loss
    float totalLoss = 0.0f;
    for (int i = 0; i < 10; ++i) {
        totalLoss += lossValues[i];
    }
    
    // Return mean squared error
    return totalLoss / 10.0f;
}

void ICMModule::setRewardScale(float scale) {
    rewardScale = scale;
}

float ICMModule::getRewardScale() const {
    return rewardScale;
}

bool ICMModule::save(const std::string& path) {
    // Save module parameters
    std::string hyperparamsPath = path + ".icm.params";
    std::ofstream file(hyperparamsPath);
    if (file.is_open()) {
        file << "reward_scale=" << rewardScale << std::endl;
        file << "forward_loss_weight=" << forwardLossWeight << std::endl;
        file << "inverse_loss_weight=" << inverseLossWeight << std::endl;
        file.close();
    }
    
    // Save models
    // In a real implementation, we would save the feature, forward, and inverse networks
    // std::string featurePath = path + ".icm.feature";
    // std::string forwardPath = path + ".icm.forward";
    // std::string inversePath = path + ".icm.inverse";
    // featureNetwork->save(featurePath);
    // forwardModel->save(forwardPath);
    // inverseModel->save(inversePath);
    
    return true;
}

bool ICMModule::load(const std::string& path) {
    // Load module parameters
    std::string hyperparamsPath = path + ".icm.params";
    std::ifstream file(hyperparamsPath);
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            size_t pos = line.find('=');
            if (pos != std::string::npos) {
                std::string key = line.substr(0, pos);
                std::string value = line.substr(pos + 1);
                
                if (key == "reward_scale") {
                    rewardScale = std::stof(value);
                } else if (key == "forward_loss_weight") {
                    forwardLossWeight = std::stof(value);
                } else if (key == "inverse_loss_weight") {
                    inverseLossWeight = std::stof(value);
                }
            }
        }
        file.close();
    }
    
    // Load models
    // In a real implementation, we would load the feature, forward, and inverse networks
    // std::string featurePath = path + ".icm.feature";
    // std::string forwardPath = path + ".icm.forward";
    // std::string inversePath = path + ".icm.inverse";
    // featureNetwork->load(featurePath);
    // forwardModel->load(forwardPath);
    // inverseModel->load(inversePath);
    
    return true;
}

std::vector<float> ICMModule::encodeState(const AIInputFrame& state) {
    // Check if Metal resources are available
    if (metalCommandQueue && encodeStatePipeline && state.frameBuffer && state.width > 0 && state.height > 0) {
        return encodeStateWithMetal(state);
    }
    
    // Fall back to CPU implementation if Metal not available
    
    // Use feature network to encode state
    // In a real implementation, this would use the actual feature network
    // return featureNetwork->predict(stateToVector(state));
    
    // For this stub, we'll create a simple encoding
    std::vector<float> features;
    
    // Extract game state from frame buffer (simple downsampling)
    if (state.frameBuffer && state.width > 0 && state.height > 0) {
        const uint8_t* frameData = static_cast<const uint8_t*>(state.frameBuffer);
        
        // Downsample to 16x16 grayscale
        const int featureWidth = 16;
        const int featureHeight = 16;
        features.resize(featureWidth * featureHeight);
        
        for (int y = 0; y < featureHeight; ++y) {
            for (int x = 0; x < featureWidth; ++x) {
                // Calculate source pixel coordinates (with scaling)
                int srcX = x * state.width / featureWidth;
                int srcY = y * state.height / featureHeight;
                
                // Calculate source pixel offset
                size_t srcOffset = (srcY * state.width + srcX) * 4; // RGBA
                
                // Calculate grayscale value
                float gray = 0.299f * frameData[srcOffset] + 
                             0.587f * frameData[srcOffset + 1] + 
                             0.114f * frameData[srcOffset + 2];
                
                // Normalize to [0,1]
                features[y * featureWidth + x] = gray / 255.0f;
            }
        }
    }
    
    return features;
}

std::vector<float> ICMModule::encodeStateWithMetal(const AIInputFrame& state) {
    // Result vector (16x16 feature map)
    std::vector<float> features(256, 0.0f);
    
    // Set dimensions
    uint32_t dimensions[2] = {static_cast<uint32_t>(state.width), static_cast<uint32_t>(state.height)};
    memcpy([dimensionsBuffer contents], dimensions, sizeof(dimensions));
    
    // Create command buffer
    id<MTLCommandBuffer> commandBuffer = [metalCommandQueue commandBuffer];
    id<MTLComputeCommandEncoder> encoder = [commandBuffer computeCommandEncoder];
    
    // Set the compute pipeline
    [encoder setComputePipelineState:encodeStatePipeline];
    
    // Create a temporary buffer for the frame data if needed
    id<MTLBuffer> frameBuffer = [metalDevice newBufferWithBytes:state.frameBuffer
                                                         length:state.width * state.height * 4
                                                        options:MTLResourceStorageModeShared];
    
    // Set the buffers
    [encoder setBuffer:frameBuffer offset:0 atIndex:0];
    [encoder setBuffer:stateFeaturesBuffer offset:0 atIndex:1];
    [encoder setBuffer:dimensionsBuffer offset:0 atIndex:2];
    
    // Calculate thread groups
    NSUInteger threadGroupSize = encodeStatePipeline.maxTotalThreadsPerThreadgroup;
    if (threadGroupSize > 256) threadGroupSize = 256;
    
    // Dispatch threads
    [encoder dispatchThreads:MTLSizeMake(256, 1, 1)
      threadsPerThreadgroup:MTLSizeMake(threadGroupSize, 1, 1)];
    
    // End encoding and commit
    [encoder endEncoding];
    [commandBuffer commit];
    [commandBuffer waitUntilCompleted];
    
    // Copy results from Metal buffer to vector
    memcpy(features.data(), [stateFeaturesBuffer contents], 256 * sizeof(float));
    
    return features;
}

std::vector<float> ICMModule::predictNextState(const std::vector<float>& stateFeatures, const AIOutputAction& action) {
    // Check if Metal resources are available
    if (metalCommandQueue && forwardModelPipeline && !stateFeatures.empty()) {
        return predictNextStateWithMetal(stateFeatures, action);
    }
    
    // Fall back to CPU implementation if Metal not available
    
    // Use forward model to predict next state features
    // In a real implementation, this would use the actual forward model
    // return forwardModel->predict(combineStateAndAction(stateFeatures, action));
    
    // For this stub, we'll create a simple prediction
    std::vector<float> nextStateFeatures = stateFeatures;
    
    // Add some simple dynamics based on action
    if (action.up) {
        // Shift features up
        for (size_t i = 0; i < nextStateFeatures.size() - 16; ++i) {
            nextStateFeatures[i] = nextStateFeatures[i + 16];
        }
    } else if (action.down) {
        // Shift features down
        for (int i = nextStateFeatures.size() - 1; i >= 16; --i) {
            nextStateFeatures[i] = nextStateFeatures[i - 16];
        }
    } else if (action.left) {
        // Shift features left
        for (size_t i = 0; i < nextStateFeatures.size() - 1; ++i) {
            if ((i + 1) % 16 != 0) {
                nextStateFeatures[i] = nextStateFeatures[i + 1];
            }
        }
    } else if (action.right) {
        // Shift features right
        for (int i = nextStateFeatures.size() - 1; i > 0; --i) {
            if (i % 16 != 0) {
                nextStateFeatures[i] = nextStateFeatures[i - 1];
            }
        }
    }
    
    // Add some noise to simulate prediction error
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<float> dist(0.0f, 0.05f);
    
    for (auto& feature : nextStateFeatures) {
        feature += dist(gen);
        feature = std::max(0.0f, std::min(1.0f, feature)); // Clamp to [0,1]
    }
    
    return nextStateFeatures;
}

std::vector<float> ICMModule::predictNextStateWithMetal(const std::vector<float>& stateFeatures, const AIOutputAction& action) {
    // Result vector (same size as state features)
    std::vector<float> nextStateFeatures(256, 0.0f);
    
    // Convert action to vector
    float actionVector[10] = {
        action.up ? 1.0f : 0.0f,
        action.down ? 1.0f : 0.0f,
        action.left ? 1.0f : 0.0f,
        action.right ? 1.0f : 0.0f,
        action.buttons[0] ? 1.0f : 0.0f,
        action.buttons[1] ? 1.0f : 0.0f,
        action.buttons[2] ? 1.0f : 0.0f,
        action.buttons[3] ? 1.0f : 0.0f,
        action.buttons[4] ? 1.0f : 0.0f,
        action.buttons[5] ? 1.0f : 0.0f
    };
    
    // Copy state features to Metal buffer
    size_t featureSize = std::min(stateFeatures.size(), static_cast<size_t>(256)) * sizeof(float);
    memcpy([stateFeaturesBuffer contents], stateFeatures.data(), featureSize);
    
    // Copy action vector to Metal buffer
    memcpy([actionBuffer contents], actionVector, sizeof(actionVector));
    
    // Create command buffer
    id<MTLCommandBuffer> commandBuffer = [metalCommandQueue commandBuffer];
    id<MTLComputeCommandEncoder> encoder = [commandBuffer computeCommandEncoder];
    
    // Set the compute pipeline
    [encoder setComputePipelineState:forwardModelPipeline];
    
    // Set the buffers
    [encoder setBuffer:stateFeaturesBuffer offset:0 atIndex:0];
    [encoder setBuffer:actionBuffer offset:0 atIndex:1];
    [encoder setBuffer:predictedNextFeaturesBuffer offset:0 atIndex:2];
    
    // Calculate thread groups
    NSUInteger threadGroupSize = forwardModelPipeline.maxTotalThreadsPerThreadgroup;
    if (threadGroupSize > 256) threadGroupSize = 256;
    
    // Dispatch threads
    [encoder dispatchThreads:MTLSizeMake(256, 1, 1)
      threadsPerThreadgroup:MTLSizeMake(threadGroupSize, 1, 1)];
    
    // End encoding and commit
    [encoder endEncoding];
    [commandBuffer commit];
    [commandBuffer waitUntilCompleted];
    
    // Copy results from Metal buffer to vector
    memcpy(nextStateFeatures.data(), [predictedNextFeaturesBuffer contents], 256 * sizeof(float));
    
    return nextStateFeatures;
}

std::vector<float> ICMModule::predictAction(const std::vector<float>& stateFeatures, const std::vector<float>& nextStateFeatures) {
    // Check if Metal resources are available
    if (metalCommandQueue && inverseModelPipeline && !stateFeatures.empty() && !nextStateFeatures.empty()) {
        return predictActionWithMetal(stateFeatures, nextStateFeatures);
    }
    
    // Fall back to CPU implementation if Metal not available
    
    // Use inverse model to predict action
    // In a real implementation, this would use the actual inverse model
    // return inverseModel->predict(combineStates(stateFeatures, nextStateFeatures));
    
    // For this stub, we'll create a simple prediction
    std::vector<float> action(10, 0.0f); // 10 action dimensions
    
    // Calculate feature differences
    std::vector<float> diff;
    diff.resize(stateFeatures.size());
    
    for (size_t i = 0; i < stateFeatures.size() && i < nextStateFeatures.size(); ++i) {
        diff[i] = nextStateFeatures[i] - stateFeatures[i];
    }
    
    // Predict direction based on feature differences
    float upDiff = 0.0f;
    float downDiff = 0.0f;
    float leftDiff = 0.0f;
    float rightDiff = 0.0f;
    
    // Simple heuristic for direction prediction
    for (size_t i = 0; i < diff.size(); ++i) {
        int x = i % 16;
        int y = i / 16;
        
        if (y < 8) {
            upDiff += diff[i];
        } else {
            downDiff += diff[i];
        }
        
        if (x < 8) {
            leftDiff += diff[i];
        } else {
            rightDiff += diff[i];
        }
    }
    
    // Set predicted actions based on differences
    action[0] = upDiff > 0.1f ? 1.0f : 0.0f;    // Up
    action[1] = downDiff > 0.1f ? 1.0f : 0.0f;  // Down
    action[2] = leftDiff > 0.1f ? 1.0f : 0.0f;  // Left
    action[3] = rightDiff > 0.1f ? 1.0f : 0.0f; // Right
    
    // Add some random button presses
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    
    for (int i = 4; i < 10; ++i) {
        action[i] = dist(gen) > 0.8f ? 1.0f : 0.0f;
    }
    
    return action;
}

std::vector<float> ICMModule::predictActionWithMetal(const std::vector<float>& stateFeatures, const std::vector<float>& nextStateFeatures) {
    // Result vector (10 action dimensions)
    std::vector<float> predictedAction(10, 0.0f);
    
    // Copy state features to Metal buffer
    size_t featureSize = std::min(stateFeatures.size(), static_cast<size_t>(256)) * sizeof(float);
    memcpy([stateFeaturesBuffer contents], stateFeatures.data(), featureSize);
    
    // Copy next state features to Metal buffer
    size_t nextFeatureSize = std::min(nextStateFeatures.size(), static_cast<size_t>(256)) * sizeof(float);
    memcpy([nextStateFeaturesBuffer contents], nextStateFeatures.data(), nextFeatureSize);
    
    // Create command buffer
    id<MTLCommandBuffer> commandBuffer = [metalCommandQueue commandBuffer];
    id<MTLComputeCommandEncoder> encoder = [commandBuffer computeCommandEncoder];
    
    // Set the compute pipeline
    [encoder setComputePipelineState:inverseModelPipeline];
    
    // Set the buffers
    [encoder setBuffer:stateFeaturesBuffer offset:0 atIndex:0];
    [encoder setBuffer:nextStateFeaturesBuffer offset:0 atIndex:1];
    [encoder setBuffer:predictedActionBuffer offset:0 atIndex:2];
    
    // Calculate thread groups
    NSUInteger threadGroupSize = inverseModelPipeline.maxTotalThreadsPerThreadgroup;
    if (threadGroupSize > 10) threadGroupSize = 10;
    
    // Dispatch threads
    [encoder dispatchThreads:MTLSizeMake(10, 1, 1)
      threadsPerThreadgroup:MTLSizeMake(threadGroupSize, 1, 1)];
    
    // End encoding and commit
    [encoder endEncoding];
    [commandBuffer commit];
    [commandBuffer waitUntilCompleted];
    
    // Copy results from Metal buffer to vector
    memcpy(predictedAction.data(), [predictedActionBuffer contents], 10 * sizeof(float));
    
    return predictedAction;
}

void ICMModule::initializeMetalResources() {
    // Get Metal device
    metalDevice = MTLCreateSystemDefaultDevice();
    if (!metalDevice) {
        std::cerr << "ICM: Metal is not supported on this device" << std::endl;
        return;
    }
    
    // Create command queue
    metalCommandQueue = [metalDevice newCommandQueue];
    
    // Load Metal compute kernels for ICM
    NSError* error = nil;
    NSString* kernelSource = @R"(
        #include <metal_stdlib>
        using namespace metal;
        
        // ICM forward model kernel - predicts next state features from current state and action
        kernel void icm_forward_model(
            device const float* state_features [[buffer(0)]],
            device const float* action_vector [[buffer(1)]],
            device float* next_state_features [[buffer(2)]],
            uint id [[thread_position_in_grid]]
        ) {
            // Copy original feature
            next_state_features[id] = state_features[id];
            
            // Apply simple dynamics based on action
            // (This is a simplified implementation; a real model would be more sophisticated)
            
            // Get dimensions
            uint feature_width = 16; // Assuming 16x16 feature maps
            uint feature_height = 16;
            uint x = id % feature_width;
            uint y = id / feature_width;
            
            // Check action (up, down, left, right)
            if (action_vector[0] > 0.5f) { // Up
                // Shift features up
                if (y >= 1 && y < feature_height) {
                    uint src_idx = ((y - 1) * feature_width) + x;
                    next_state_features[id] = state_features[src_idx];
                }
            } else if (action_vector[1] > 0.5f) { // Down
                // Shift features down
                if (y < feature_height - 1) {
                    uint src_idx = ((y + 1) * feature_width) + x;
                    next_state_features[id] = state_features[src_idx];
                }
            } else if (action_vector[2] > 0.5f) { // Left
                // Shift features left
                if (x >= 1 && x < feature_width) {
                    uint src_idx = (y * feature_width) + (x - 1);
                    next_state_features[id] = state_features[src_idx];
                }
            } else if (action_vector[3] > 0.5f) { // Right
                // Shift features right
                if (x < feature_width - 1) {
                    uint src_idx = (y * feature_width) + (x + 1);
                    next_state_features[id] = state_features[src_idx];
                }
            }
            
            // Add small random perturbation using hash function
            uint seed = id ^ 0xdeadbeef;
            seed = (seed ^ 61) ^ (seed >> 16);
            seed *= 9;
            seed = seed ^ (seed >> 4);
            seed *= 0x27d4eb2d;
            seed = seed ^ (seed >> 15);
            float random_value = float(seed % 1000) / 1000.0f;
            
            // Apply small noise (0-0.1)
            next_state_features[id] += (random_value * 0.1f) - 0.05f;
            
            // Clamp to [0,1]
            next_state_features[id] = clamp(next_state_features[id], 0.0f, 1.0f);
        }
        
        // ICM inverse model kernel - predicts action from current and next state features
        kernel void icm_inverse_model(
            device const float* state_features [[buffer(0)]],
            device const float* next_state_features [[buffer(1)]],
            device float* predicted_action [[buffer(2)]],
            uint id [[thread_position_in_grid]]
        ) {
            // Only process for the first 10 threads (action dimensions)
            if (id >= 10) return;
            
            // Default to zero
            predicted_action[id] = 0.0f;
            
            // Calculate features for directional actions
            if (id < 4) { // Directional actions (up, down, left, right)
                uint feature_size = 256; // 16x16 feature maps
                float total_diff = 0.0f;
                
                // Process all feature differences
                for (uint i = 0; i < feature_size; i++) {
                    float diff = next_state_features[i] - state_features[i];
                    uint x = i % 16;
                    uint y = i / 16;
                    
                    if (id == 0) { // Up
                        if (y < 8) total_diff += diff;
                    } else if (id == 1) { // Down
                        if (y >= 8) total_diff += diff;
                    } else if (id == 2) { // Left
                        if (x < 8) total_diff += diff;
                    } else if (id == 3) { // Right
                        if (x >= 8) total_diff += diff;
                    }
                }
                
                // Set action based on threshold
                predicted_action[id] = (total_diff > 0.1f) ? 1.0f : 0.0f;
            } else {
                // For button presses, use a hash function to generate pseudo-random values
                uint seed = id ^ 0xabcdef;
                seed = (seed ^ 61) ^ (seed >> 16);
                seed *= 9;
                seed = seed ^ (seed >> 4);
                seed *= 0x27d4eb2d;
                seed = seed ^ (seed >> 15);
                float random_value = float(seed % 1000) / 1000.0f;
                
                // Set random button presses
                predicted_action[id] = (random_value > 0.8f) ? 1.0f : 0.0f;
            }
        }
        
        // ICM feature encoding kernel - encodes input frame to feature representation
        kernel void icm_encode_state(
            device const uchar4* frame_buffer [[buffer(0)]],
            device float* features [[buffer(1)]],
            device const uint* dimensions [[buffer(2)]],
            uint id [[thread_position_in_grid]]
        ) {
            // Map thread ID to feature coordinate
            uint feature_width = 16;  // Output feature width
            uint feature_height = 16; // Output feature height
            
            uint x = id % feature_width;
            uint y = id / feature_width;
            
            if (x >= feature_width || y >= feature_height) return;
            
            // Get source dimensions
            uint src_width = dimensions[0];
            uint src_height = dimensions[1];
            
            // Calculate source pixel coordinates (with scaling)
            uint src_x = x * src_width / feature_width;
            uint src_y = y * src_height / feature_height;
            
            // Calculate source pixel offset
            uint src_offset = (src_y * src_width) + src_x;
            
            // Get RGBA components
            uchar4 pixel = frame_buffer[src_offset];
            
            // Calculate grayscale value
            float gray = 0.299f * float(pixel.r) + 
                         0.587f * float(pixel.g) + 
                         0.114f * float(pixel.b);
            
            // Normalize to [0,1]
            features[id] = gray / 255.0f;
        }
        
        // ICM forward loss kernel - computes MSE between predicted and actual next features
        kernel void icm_forward_loss(
            device const float* next_state_features [[buffer(0)]],
            device const float* predicted_next_features [[buffer(1)]],
            device float* loss_output [[buffer(2)]],
            uint id [[thread_position_in_grid]]
        ) {
            // Calculate squared difference
            float diff = next_state_features[id] - predicted_next_features[id];
            loss_output[id] = diff * diff;
        }
        
        // ICM inverse loss kernel - computes MSE between predicted and actual actions
        kernel void icm_inverse_loss(
            device const float* action [[buffer(0)]],
            device const float* predicted_action [[buffer(1)]],
            device float* loss_output [[buffer(2)]],
            uint id [[thread_position_in_grid]]
        ) {
            // Calculate squared difference
            float diff = action[id] - predicted_action[id];
            loss_output[id] = diff * diff;
        }
    )";
    
    // Create Metal library
    metalLibrary = [metalDevice newLibraryWithSource:kernelSource options:nil error:&error];
    if (!metalLibrary) {
        std::cerr << "ICM: Failed to create Metal library: " << [error.localizedDescription UTF8String] << std::endl;
        return;
    }
    
    // Create compute pipelines for each function
    id<MTLFunction> forwardModelFunction = [metalLibrary newFunctionWithName:@"icm_forward_model"];
    id<MTLFunction> inverseModelFunction = [metalLibrary newFunctionWithName:@"icm_inverse_model"];
    id<MTLFunction> encodeStateFunction = [metalLibrary newFunctionWithName:@"icm_encode_state"];
    id<MTLFunction> forwardLossFunction = [metalLibrary newFunctionWithName:@"icm_forward_loss"];
    id<MTLFunction> inverseLossFunction = [metalLibrary newFunctionWithName:@"icm_inverse_loss"];
    
    if (!forwardModelFunction || !inverseModelFunction || !encodeStateFunction || 
        !forwardLossFunction || !inverseLossFunction) {
        std::cerr << "ICM: Failed to create Metal compute functions" << std::endl;
        return;
    }
    
    // Create compute pipelines
    forwardModelPipeline = [metalDevice newComputePipelineStateWithFunction:forwardModelFunction error:&error];
    inverseModelPipeline = [metalDevice newComputePipelineStateWithFunction:inverseModelFunction error:&error];
    encodeStatePipeline = [metalDevice newComputePipelineStateWithFunction:encodeStateFunction error:&error];
    forwardLossPipeline = [metalDevice newComputePipelineStateWithFunction:forwardLossFunction error:&error];
    inverseLossPipeline = [metalDevice newComputePipelineStateWithFunction:inverseLossFunction error:&error];
    
    if (!forwardModelPipeline || !inverseModelPipeline || !encodeStatePipeline || 
        !forwardLossPipeline || !inverseLossPipeline) {
        std::cerr << "ICM: Failed to create Metal compute pipelines: " 
                  << [error.localizedDescription UTF8String] << std::endl;
        return;
    }
    
    // Create reusable buffers
    stateFeaturesBuffer = [metalDevice newBufferWithLength:256 * sizeof(float) options:MTLResourceStorageModeShared];
    nextStateFeaturesBuffer = [metalDevice newBufferWithLength:256 * sizeof(float) options:MTLResourceStorageModeShared];
    predictedNextFeaturesBuffer = [metalDevice newBufferWithLength:256 * sizeof(float) options:MTLResourceStorageModeShared];
    actionBuffer = [metalDevice newBufferWithLength:10 * sizeof(float) options:MTLResourceStorageModeShared];
    predictedActionBuffer = [metalDevice newBufferWithLength:10 * sizeof(float) options:MTLResourceStorageModeShared];
    dimensionsBuffer = [metalDevice newBufferWithLength:2 * sizeof(uint32_t) options:MTLResourceStorageModeShared];
    forwardLossBuffer = [metalDevice newBufferWithLength:256 * sizeof(float) options:MTLResourceStorageModeShared];
    inverseLossBuffer = [metalDevice newBufferWithLength:10 * sizeof(float) options:MTLResourceStorageModeShared];
    
    std::cout << "ICM: Successfully initialized Metal compute resources" << std::endl;
}

void ICMModule::cleanupMetalResources() {
    // Release Metal resources (ARC will handle most of the cleanup)
    metalLibrary = nil;
    forwardModelPipeline = nil;
    inverseModelPipeline = nil;
    encodeStatePipeline = nil;
    forwardLossPipeline = nil;
    inverseLossPipeline = nil;
    stateFeaturesBuffer = nil;
    nextStateFeaturesBuffer = nil;
    predictedNextFeaturesBuffer = nil;
    actionBuffer = nil;
    predictedActionBuffer = nil;
    dimensionsBuffer = nil;
    forwardLossBuffer = nil;
    inverseLossBuffer = nil;
    metalCommandQueue = nil;
    metalDevice = nil;
}

// Helper function to convert vector to input frame
AIInputFrame ICMModule::convertVectorToInputFrame(const std::vector<float>& vec) {
    AIInputFrame frame{};
    
    // Simple 16x16 RGBA frame
    const int width = 16;
    const int height = 16;
    frame.width = width;
    frame.height = height;
    
    // Allocate frame buffer
    frame.frameBuffer = malloc(width * height * 4);
    if (!frame.frameBuffer) return frame;
    
    // Fill frame buffer with vector data
    uint8_t* buffer = static_cast<uint8_t*>(frame.frameBuffer);
    const size_t vecSize = std::min(vec.size(), static_cast<size_t>(width * height));
    
    for (size_t i = 0; i < vecSize; ++i) {
        // Unnormalize from [0,1] to [0,255]
        uint8_t value = static_cast<uint8_t>(vec[i] * 255.0f);
        
        // Set RGBA (use grayscale)
        buffer[i * 4 + 0] = value; // R
        buffer[i * 4 + 1] = value; // G
        buffer[i * 4 + 2] = value; // B
        buffer[i * 4 + 3] = 255;   // A
    }
    
    return frame;
}

// Helper function to convert vector to action
AIOutputAction ICMModule::convertVectorToAction(const std::vector<float>& vec) {
    AIOutputAction action{};
    
    // Set action values from vector
    if (vec.size() >= 4) {
        action.up = vec[0] > 0.5f;
        action.down = vec[1] > 0.5f;
        action.left = vec[2] > 0.5f;
        action.right = vec[3] > 0.5f;
        
        // Button presses
        for (int i = 0; i < 6 && i + 4 < vec.size(); ++i) {
            action.buttons[i] = vec[i + 4] > 0.5f;
        }
    }
    
    return action;
}

} // namespace ai
} // namespace fbneo 