#include "ai_rl_algorithms.h"
#include "ai_torch_policy.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <iostream>
#include <fstream>
#include <random>
#pragma once

#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>

namespace fbneo {
namespace ai {

//------------------------------------------------------------------------------
// PPO Algorithm Implementation
//------------------------------------------------------------------------------

PPOAlgorithm::PPOAlgorithm(AITorchPolicy* policy)
    : RLAlgorithm(policy), clipEpsilon(0.2f), vfCoeff(0.5f), entropyCoeff(0.01f),
      lambda(0.95f), epochs(4) {
    
    // Create target policy (clone of the policy)
    if (policy) {
        targetPolicy = std::unique_ptr<AITorchPolicy>(policy->clone());
        
        // Initialize Metal-specific components
        initializeMetalCompute();
    }
}

PPOAlgorithm::~PPOAlgorithm() {
    // Cleanup Metal resources
    cleanupMetalCompute();
}

void PPOAlgorithm::train(const std::vector<Experience>& batch) {
    if (batch.empty()) {
        return;
    }
    
    // Make a copy of the batch that we can modify
    std::vector<Experience> trajectory = batch;
    
    // Compute advantages and returns using GAE
    computeGAE(trajectory);
    
    // Use Metal acceleration if available, otherwise fall back to CPU
    if (metalCommandQueue && metalPolicyPipeline && metalValuePipeline) {
        trainEpochsMetal(trajectory, epochs);
    } else {
        trainEpochs(trajectory, epochs);
    }
    
    // Update the target network periodically
    updateTargetNetwork();
}

void PPOAlgorithm::processStep(const AIInputFrame& state, const AIOutputAction& action, 
                             float reward, const AIInputFrame& next_state, bool done) {
    // First process step as in base class
    RLAlgorithm::processStep(state, action, reward, next_state, done);
    
    // For PPO, we additionally compute the log probability of the action
    float logProb = 0.0f;
    if (policy) {
        logProb = policy->computeLogProb(state, action);
    } else {
        // Fallback to a default value if policy is not available
        logProb = -1.0f;
    }
    
    // Get the value estimate
    float valueEst = 0.0f;
    if (policy) {
        valueEst = policy->computeValue(state);
    } else {
        // Fallback to zero if policy is not available
        valueEst = 0.0f;
    }
    
    // Add log prob and value to the most recent experience
    if (!buffer.empty()) {
        Experience& exp = buffer.getBuffer().back();
        exp.log_prob = logProb;
        exp.value = valueEst;
    }
    
    // Trigger training if we have enough steps and this is the end of an episode
    if (done && buffer.size() >= 128) {
        // Sample a batch of experiences
        auto batch = buffer.sample(std::min(buffer.size(), size_t(1024)));
        
        // Train on the batch
        train(batch);
        
        // Clear buffer after training (PPO is on-policy)
        buffer.clear();
    }
}

void PPOAlgorithm::setHyperparameters(const std::unordered_map<std::string, float>& params) {
    // First set base class parameters
    RLAlgorithm::setHyperparameters(params);
    
    // Set PPO-specific parameters
    if (params.count("clip_epsilon")) {
        clipEpsilon = params.at("clip_epsilon");
    }
    if (params.count("vf_coeff")) {
        vfCoeff = params.at("vf_coeff");
    }
    if (params.count("entropy_coeff")) {
        entropyCoeff = params.at("entropy_coeff");
    }
    if (params.count("lambda")) {
        lambda = params.at("lambda");
    }
    if (params.count("epochs")) {
        epochs = static_cast<int>(params.at("epochs"));
    }
}

bool PPOAlgorithm::save(const std::string& path) {
    // In a real implementation, this would save:
    // 1. Policy network weights
    // 2. Target network weights
    // 3. Optimizer state
    // 4. Hyperparameters
    
    std::cout << "Saving PPO model to " << path << std::endl;
    
    // Save hyperparameters
    std::string hyperparamsPath = path + ".params";
    std::ofstream file(hyperparamsPath);
    if (file.is_open()) {
        file << "learning_rate=" << learningRate << std::endl;
        file << "gamma=" << gamma << std::endl;
        file << "clip_epsilon=" << clipEpsilon << std::endl;
        file << "vf_coeff=" << vfCoeff << std::endl;
        file << "entropy_coeff=" << entropyCoeff << std::endl;
        file << "lambda=" << lambda << std::endl;
        file << "epochs=" << epochs << std::endl;
        file.close();
    }
    
    // Save networks (would call policy->save())
    std::string policyPath = path + ".policy";
    // policy->save(policyPath);
    
    std::string targetPath = path + ".target";
    // targetPolicy->save(targetPath);
    
    return true;
}

bool PPOAlgorithm::load(const std::string& path) {
    // In a real implementation, this would load:
    // 1. Policy network weights
    // 2. Target network weights
    // 3. Optimizer state
    // 4. Hyperparameters
    
    std::cout << "Loading PPO model from " << path << std::endl;
    
    // Load hyperparameters
    std::string hyperparamsPath = path + ".params";
    std::ifstream file(hyperparamsPath);
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            size_t pos = line.find('=');
            if (pos != std::string::npos) {
                std::string key = line.substr(0, pos);
                std::string value = line.substr(pos + 1);
                
                if (key == "learning_rate") {
                    learningRate = std::stof(value);
                } else if (key == "gamma") {
                    gamma = std::stof(value);
                } else if (key == "clip_epsilon") {
                    clipEpsilon = std::stof(value);
                } else if (key == "vf_coeff") {
                    vfCoeff = std::stof(value);
                } else if (key == "entropy_coeff") {
                    entropyCoeff = std::stof(value);
                } else if (key == "lambda") {
                    lambda = std::stof(value);
                } else if (key == "epochs") {
                    epochs = std::stoi(value);
                }
            }
        }
        file.close();
    }
    
    // Load networks (would call policy->load())
    std::string policyPath = path + ".policy";
    // policy->load(policyPath);
    
    std::string targetPath = path + ".target";
    // if (targetPolicy) {
    //     targetPolicy->load(targetPath);
    // }
    
    return true;
}

void PPOAlgorithm::computeGAE(std::vector<Experience>& trajectory, float lambda) {
    if (trajectory.empty()) {
        return;
    }
    
    // Get trajectory length
    size_t n = trajectory.size();
    
    // Arrays to store returns and advantages
    std::vector<float> returns(n);
    std::vector<float> advantages(n);
    
    // Last value is 0 if done, otherwise bootstrap from the value function
    float nextValue = 0.0f;
    if (!trajectory.back().done) {
        // In a real implementation, this would be computed from the value function
        // nextValue = policy->computeValue(trajectory.back().next_state);
    }
    
    // Initialize the next advantage
    float nextAdvantage = 0.0f;
    
    // Compute advantages and returns in reverse order
    for (int i = n - 1; i >= 0; --i) {
        // Extract information from current experience
        float reward = trajectory[i].reward;
        float value = trajectory[i].value;
        bool done = trajectory[i].done;
        
        // Compute the temporal difference error
        float delta = reward + gamma * nextValue * (1.0f - done) - value;
        
        // Compute the GAE advantage
        advantages[i] = delta + gamma * lambda * nextAdvantage * (1.0f - done);
        
        // Compute the return (value target)
        returns[i] = advantages[i] + value;
        
        // Update nextValue and nextAdvantage for the next iteration
        nextValue = value;
        nextAdvantage = advantages[i];
    }
    
    // Normalize advantages
    float meanAdv = std::accumulate(advantages.begin(), advantages.end(), 0.0f) / n;
    float varAdv = 0.0f;
    for (const auto& adv : advantages) {
        varAdv += (adv - meanAdv) * (adv - meanAdv);
    }
    varAdv /= n;
    float stdAdv = std::sqrt(varAdv) + 1e-8f;
    
    // Store normalized advantages and returns in the trajectory
    for (size_t i = 0; i < n; ++i) {
        trajectory[i].advantage = (advantages[i] - meanAdv) / stdAdv;
        trajectory[i].value = returns[i]; // Overwrite value with the return
    }
}

void PPOAlgorithm::trainEpochs(std::vector<Experience>& trajectory, int epochs) {
    if (trajectory.empty() || !policy) {
        return;
    }
    
    // Get trajectory size
    size_t n = trajectory.size();
    
    // Mini-batch size
    size_t miniBatchSize = std::min(n, size_t(64));
    
    // For each epoch
    for (int epoch = 0; epoch < epochs; ++epoch) {
        // Shuffle the trajectory
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(trajectory.begin(), trajectory.end(), g);
        
        // Total losses
        float policyLoss = 0.0f;
        float valueLoss = 0.0f;
        float entropyLoss = 0.0f;
        
        // Process mini-batches
        for (size_t i = 0; i < n; i += miniBatchSize) {
            // Get mini-batch
            size_t batchEnd = std::min(i + miniBatchSize, n);
            std::vector<Experience> miniBatch(trajectory.begin() + i, trajectory.begin() + batchEnd);
            
            // Prepare batch tensors
            std::vector<AIInputFrame> states;
            std::vector<AIOutputAction> actions;
            std::vector<float> advantages;
            std::vector<float> returns;
            std::vector<float> oldLogProbs;
            
            for (const auto& exp : miniBatch) {
                states.push_back(exp.state);
                actions.push_back(exp.action);
                advantages.push_back(exp.advantage);
                returns.push_back(exp.value); // Already contains return from computeGAE
                oldLogProbs.push_back(exp.log_prob);
            }
            
            // Perform actual policy update using the policy network
            float batchPolicyLoss = 0.0f;
            float batchValueLoss = 0.0f;
            float batchEntropyLoss = 0.0f;
            
            // Update the policy using the mini-batch
            policy->updatePolicy(
                states, 
                actions, 
                oldLogProbs, 
                advantages, 
                returns, 
                clipEpsilon, 
                vfCoeff, 
                entropyCoeff, 
                learningRate,
                &batchPolicyLoss,
                &batchValueLoss,
                &batchEntropyLoss
            );
            
            // Accumulate losses
            policyLoss += batchPolicyLoss;
            valueLoss += batchValueLoss;
            entropyLoss += batchEntropyLoss;
        }
        
        // Compute average losses
        float avgPolicyLoss = policyLoss / n;
        float avgValueLoss = valueLoss / n;
        float avgEntropyLoss = entropyLoss / n;
        
        // Compute combined loss
        float totalLoss = avgPolicyLoss + vfCoeff * avgValueLoss - entropyCoeff * avgEntropyLoss;
        
        // Print losses for this epoch
        std::cout << "PPO Epoch " << (epoch + 1) << "/" << epochs
                  << ", Policy Loss: " << avgPolicyLoss
                  << ", Value Loss: " << avgValueLoss
                  << ", Entropy: " << avgEntropyLoss
                  << ", Total Loss: " << totalLoss << std::endl;
    }
}

void PPOAlgorithm::updateTargetNetwork() {
    // Copy the policy network to the target network
    if (targetPolicy && policy) {
        targetPolicy->copyFrom(policy);
        
        // Metal-specific update logic for target network weights
        if (metalCommandQueue) {
            // In a real implementation, we would optimize this with Metal buffer copies
            // and potentially run this operation on the GPU
            
            // Get Metal device
            id<MTLDevice> device = metalCommandQueue.device;
            
            // Sync target network weights using Metal buffers if needed
            policy->syncMetalWeights();
            targetPolicy->syncMetalWeights();
        }
    }
}

// Initialize Metal compute resources for PPO
void PPOAlgorithm::initializeMetalCompute() {
    // Get Metal device and command queue
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    if (!device) {
        std::cerr << "PPO: Metal is not supported on this device" << std::endl;
        return;
    }
    
    // Create command queue
    metalCommandQueue = [device newCommandQueue];
    
    // Load compute kernels for PPO
    NSError* error = nil;
    NSString* kernelSource = @R"(
        #include <metal_stdlib>
        using namespace metal;
        
        // PPO algorithm parameters
        constant float clip_epsilon [[function_constant(0)]];
        constant float vf_coeff [[function_constant(1)]];
        constant float entropy_coeff [[function_constant(2)]];
        
        // PPO update kernel for policy network
        kernel void ppo_policy_update(
            device const float* old_probs [[buffer(0)]],
            device const float* new_probs [[buffer(1)]],
            device const float* advantages [[buffer(2)]],
            device float* policy_loss [[buffer(3)]],
            device float* entropy [[buffer(4)]],
            uint id [[thread_position_in_grid]]
        ) {
            // Calculate policy ratio (new / old)
            float ratio = new_probs[id] / max(old_probs[id], 1e-6f);
            
            // Calculate surrogate objectives
            float advantage = advantages[id];
            float surr1 = ratio * advantage;
            float surr2 = clamp(ratio, 1.0f - clip_epsilon, 1.0f + clip_epsilon) * advantage;
            
            // Policy loss (negative for gradient ascent)
            policy_loss[id] = -min(surr1, surr2);
            
            // Entropy loss for exploration
            entropy[id] = -new_probs[id] * log(max(new_probs[id], 1e-6f));
        }
        
        // PPO update kernel for value network
        kernel void ppo_value_update(
            device const float* values [[buffer(0)]],
            device const float* returns [[buffer(1)]],
            device float* value_loss [[buffer(2)]],
            uint id [[thread_position_in_grid]]
        ) {
            // Simple MSE loss
            float diff = values[id] - returns[id];
            value_loss[id] = 0.5f * diff * diff;
        }
    )";
    
    // Create Metal library
    metalLibrary = [device newLibraryWithSource:kernelSource options:nil error:&error];
    if (!metalLibrary) {
        std::cerr << "PPO: Failed to create Metal library: " << [error.localizedDescription UTF8String] << std::endl;
        return;
    }
    
    // Get compute functions
    id<MTLFunction> policyFunction = [metalLibrary newFunctionWithName:@"ppo_policy_update"];
    id<MTLFunction> valueFunction = [metalLibrary newFunctionWithName:@"ppo_value_update"];
    
    if (!policyFunction || !valueFunction) {
        std::cerr << "PPO: Failed to create Metal compute functions" << std::endl;
        return;
    }
    
    // Create compute pipelines
    metalPolicyPipeline = [device newComputePipelineStateWithFunction:policyFunction error:&error];
    metalValuePipeline = [device newComputePipelineStateWithFunction:valueFunction error:&error];
    
    if (!metalPolicyPipeline || !metalValuePipeline) {
        std::cerr << "PPO: Failed to create Metal compute pipelines: " << [error.localizedDescription UTF8String] << std::endl;
        return;
    }
    
    std::cout << "PPO: Successfully initialized Metal compute resources" << std::endl;
}

// Cleanup Metal compute resources
void PPOAlgorithm::cleanupMetalCompute() {
    // Release Metal resources (ARC will handle most of the cleanup)
    metalLibrary = nil;
    metalPolicyPipeline = nil;
    metalValuePipeline = nil;
    metalCommandQueue = nil;
}

// Metal-accelerated version of trainEpochs
void PPOAlgorithm::trainEpochsMetal(std::vector<Experience>& trajectory, int epochs) {
    if (trajectory.empty() || !policy) {
        return;
    }
    
    // Get trajectory size
    size_t n = trajectory.size();
    
    // Mini-batch size
    size_t miniBatchSize = std::min(n, size_t(64));
    
    // Create Metal buffers
    // These will be reused for each mini-batch
    id<MTLBuffer> oldProbsBuffer = nil;
    id<MTLBuffer> newProbsBuffer = nil;
    id<MTLBuffer> advantagesBuffer = nil;
    id<MTLBuffer> valuesBuffer = nil;
    id<MTLBuffer> returnsBuffer = nil;
    id<MTLBuffer> policyLossBuffer = nil;
    id<MTLBuffer> valueLossBuffer = nil;
    id<MTLBuffer> entropyBuffer = nil;
    
    // Get Metal device
    id<MTLDevice> device = metalCommandQueue.device;
    
    // Create temporary CPU buffers
    std::vector<float> oldProbs(miniBatchSize);
    std::vector<float> newProbs(miniBatchSize);
    std::vector<float> advantages(miniBatchSize);
    std::vector<float> values(miniBatchSize);
    std::vector<float> returns(miniBatchSize);
    std::vector<float> policyLoss(miniBatchSize);
    std::vector<float> valueLoss(miniBatchSize);
    std::vector<float> entropy(miniBatchSize);
    
    // For each epoch
    for (int epoch = 0; epoch < epochs; ++epoch) {
        // Shuffle the trajectory
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(trajectory.begin(), trajectory.end(), g);
        
        // Total losses
        float totalPolicyLoss = 0.0f;
        float totalValueLoss = 0.0f;
        float totalEntropyLoss = 0.0f;
        
        // Process mini-batches
        for (size_t i = 0; i < n; i += miniBatchSize) {
            // Get mini-batch
            size_t batchEnd = std::min(i + miniBatchSize, n);
            size_t currentBatchSize = batchEnd - i;
            
            // Prepare tensors
            std::vector<AIInputFrame> states;
            std::vector<AIOutputAction> actions;
            
            // Fill in CPU buffers
            for (size_t j = 0; j < currentBatchSize; ++j) {
                const auto& exp = trajectory[i + j];
                
                // Store states and actions for the policy
                states.push_back(exp.state);
                actions.push_back(exp.action);
                
                // Extract data from experience
                oldProbs[j] = std::exp(exp.log_prob); // Convert log prob to prob
                advantages[j] = exp.advantage;
                returns[j] = exp.value; // This contains the return (value target)
            }
            
            // Get new probabilities and values from the policy
            if (policy) {
                policy->batchInference(states, actions, newProbs.data(), values.data(), currentBatchSize);
            }
            
            // Create or update Metal buffers
            if (!oldProbsBuffer || oldProbsBuffer.length < currentBatchSize * sizeof(float)) {
                oldProbsBuffer = [device newBufferWithBytes:oldProbs.data() 
                                                     length:currentBatchSize * sizeof(float)
                                                    options:MTLResourceStorageModeShared];
                newProbsBuffer = [device newBufferWithBytes:newProbs.data()
                                                     length:currentBatchSize * sizeof(float)
                                                    options:MTLResourceStorageModeShared];
                advantagesBuffer = [device newBufferWithBytes:advantages.data()
                                                      length:currentBatchSize * sizeof(float)
                                                     options:MTLResourceStorageModeShared];
                valuesBuffer = [device newBufferWithBytes:values.data()
                                                   length:currentBatchSize * sizeof(float)
                                                  options:MTLResourceStorageModeShared];
                returnsBuffer = [device newBufferWithBytes:returns.data()
                                                    length:currentBatchSize * sizeof(float)
                                                   options:MTLResourceStorageModeShared];
                policyLossBuffer = [device newBufferWithLength:currentBatchSize * sizeof(float)
                                                      options:MTLResourceStorageModeShared];
                valueLossBuffer = [device newBufferWithLength:currentBatchSize * sizeof(float)
                                                     options:MTLResourceStorageModeShared];
                entropyBuffer = [device newBufferWithLength:currentBatchSize * sizeof(float)
                                                   options:MTLResourceStorageModeShared];
            } else {
                // Update existing buffers
                memcpy([oldProbsBuffer contents], oldProbs.data(), currentBatchSize * sizeof(float));
                memcpy([newProbsBuffer contents], newProbs.data(), currentBatchSize * sizeof(float));
                memcpy([advantagesBuffer contents], advantages.data(), currentBatchSize * sizeof(float));
                memcpy([valuesBuffer contents], values.data(), currentBatchSize * sizeof(float));
                memcpy([returnsBuffer contents], returns.data(), currentBatchSize * sizeof(float));
            }
            
            // Create command buffer
            id<MTLCommandBuffer> commandBuffer = [metalCommandQueue commandBuffer];
            commandBuffer.label = @"PPO Update";
            
            // Policy update
            {
                id<MTLComputeCommandEncoder> encoder = [commandBuffer computeCommandEncoder];
                encoder.label = @"PPO Policy Update";
                
                // Set constants
                float constants[] = {clipEpsilon, vfCoeff, entropyCoeff};
                [encoder setBytes:constants length:sizeof(constants) atIndex:5];
                
                [encoder setComputePipelineState:metalPolicyPipeline];
                [encoder setBuffer:oldProbsBuffer offset:0 atIndex:0];
                [encoder setBuffer:newProbsBuffer offset:0 atIndex:1];
                [encoder setBuffer:advantagesBuffer offset:0 atIndex:2];
                [encoder setBuffer:policyLossBuffer offset:0 atIndex:3];
                [encoder setBuffer:entropyBuffer offset:0 atIndex:4];
                
                // Execute the compute kernel
                NSUInteger threadGroupSize = metalPolicyPipeline.maxTotalThreadsPerThreadgroup;
                if (threadGroupSize > currentBatchSize) threadGroupSize = currentBatchSize;
                [encoder dispatchThreads:MTLSizeMake(currentBatchSize, 1, 1)
                  threadsPerThreadgroup:MTLSizeMake(threadGroupSize, 1, 1)];
                [encoder endEncoding];
            }
            
            // Value update
            {
                id<MTLComputeCommandEncoder> encoder = [commandBuffer computeCommandEncoder];
                encoder.label = @"PPO Value Update";
                [encoder setComputePipelineState:metalValuePipeline];
                [encoder setBuffer:valuesBuffer offset:0 atIndex:0];
                [encoder setBuffer:returnsBuffer offset:0 atIndex:1];
                [encoder setBuffer:valueLossBuffer offset:0 atIndex:2];
                
                // Execute the compute kernel
                NSUInteger threadGroupSize = metalValuePipeline.maxTotalThreadsPerThreadgroup;
                if (threadGroupSize > currentBatchSize) threadGroupSize = currentBatchSize;
                [encoder dispatchThreads:MTLSizeMake(currentBatchSize, 1, 1)
                  threadsPerThreadgroup:MTLSizeMake(threadGroupSize, 1, 1)];
                [encoder endEncoding];
            }
            
            // Commit and wait
            [commandBuffer commit];
            [commandBuffer waitUntilCompleted];
            
            // Read back results
            memcpy(policyLoss.data(), [policyLossBuffer contents], currentBatchSize * sizeof(float));
            memcpy(valueLoss.data(), [valueLossBuffer contents], currentBatchSize * sizeof(float));
            memcpy(entropy.data(), [entropyBuffer contents], currentBatchSize * sizeof(float));
            
            // Compute average losses
            float batchPolicyLoss = 0.0f;
            float batchValueLoss = 0.0f;
            float batchEntropyLoss = 0.0f;
            
            for (size_t j = 0; j < currentBatchSize; ++j) {
                batchPolicyLoss += policyLoss[j];
                batchValueLoss += valueLoss[j];
                batchEntropyLoss += entropy[j];
            }
            
            batchPolicyLoss /= currentBatchSize;
            batchValueLoss /= currentBatchSize;
            batchEntropyLoss /= currentBatchSize;
            
            // Update total losses
            totalPolicyLoss += batchPolicyLoss;
            totalValueLoss += batchValueLoss;
            totalEntropyLoss += batchEntropyLoss;
            
            // Apply gradients to update the policy model
            if (policy) {
                // Calculate gradient updates for the policy model
                std::vector<float> gradients;
                gradients.reserve(currentBatchSize * 3); // Policy, value, and entropy gradients
                
                // Combine all gradients
                for (size_t j = 0; j < currentBatchSize; ++j) {
                    gradients.push_back(policyLoss[j]);
                    gradients.push_back(valueLoss[j]);
                    gradients.push_back(entropy[j]);
                }
                
                // Apply gradients to the model
                policy->applyGradients(gradients.data(), currentBatchSize, learningRate);
            }
        }
        
        // Compute average losses across all batches
        float avgPolicyLoss = totalPolicyLoss / (n / miniBatchSize);
        float avgValueLoss = totalValueLoss / (n / miniBatchSize);
        float avgEntropyLoss = totalEntropyLoss / (n / miniBatchSize);
        
        // Compute combined loss
        float totalLoss = avgPolicyLoss + vfCoeff * avgValueLoss - entropyCoeff * avgEntropyLoss;
        
        // Print losses for this epoch
        std::cout << "PPO Metal Epoch " << (epoch + 1) << "/" << epochs
                  << ", Policy Loss: " << avgPolicyLoss
                  << ", Value Loss: " << avgValueLoss
                  << ", Entropy: " << avgEntropyLoss
                  << ", Total Loss: " << totalLoss << std::endl;
    }
}

} // namespace ai
} // namespace fbneo 