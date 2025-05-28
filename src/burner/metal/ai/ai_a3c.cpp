#include "ai_rl_algorithms.h"
#include "ai_torch_policy.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <random>

// Add Metal imports at the top of the file
#pragma once

#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>

namespace fbneo {
namespace ai {

//------------------------------------------------------------------------------
// A3C Algorithm Implementation
//------------------------------------------------------------------------------

A3CAlgorithm::A3CAlgorithm(AITorchPolicy* globalPolicy, int numWorkers)
    : RLAlgorithm(globalPolicy), globalPolicy(globalPolicy), numWorkers(numWorkers), shouldStop(false) {
    
    // Initialize Metal resources
    initializeMetalResources();
    
    // Initialize workers
    workers.reserve(numWorkers);
    for (int i = 0; i < numWorkers; ++i) {
        auto worker = std::make_unique<WorkerState>();
        worker->id = i;
        worker->running = false;
        
        // Create a copy of the global policy for this worker
        worker->policy = std::unique_ptr<AITorchPolicy>(globalPolicy->clone());
        
        // Create Metal buffer for this worker's gradients
        if (metalDevice) {
            size_t gradientSize = calculateGradientBufferSize(worker->policy.get());
            worker->gradientBuffer = [metalDevice newBufferWithLength:gradientSize 
                                                             options:MTLResourceStorageModeShared];
        }
        
        workers.push_back(std::move(worker));
    }
}

A3CAlgorithm::~A3CAlgorithm() {
    // Stop all workers
    stopWorkers();
    
    // Clean up Metal resources
    cleanupMetalResources();
}

void A3CAlgorithm::train(const std::vector<Experience>& batch) {
    if (batch.empty()) {
        return;
    }
    
    // In A3C, each worker updates the global network independently
    // This method is called by the main thread to manually train
    // on a batch of experiences (e.g., for testing)
    
    // Use Metal acceleration if available
    if (metalCommandQueue && aggregateGradientsPipeline && applyGradientsPipeline) {
        trainWithMetal(batch);
    } else {
        // Original CPU implementation
        // Calculate advantages and returns
        std::vector<float> advantages;
        std::vector<float> returns;
        advantages.reserve(batch.size());
        returns.reserve(batch.size());
        
        // Last value is 0 if done, otherwise bootstrap from the value function
        float nextValue = 0.0f;
        if (!batch.back().done) {
            // Get value estimate from the global policy
            nextValue = globalPolicy->getValue(convertVectorToInputFrame(batch.back().next_state));
        }
        
        // Compute advantages and returns in reverse order (like GAE, but with lambda=1)
        for (int i = batch.size() - 1; i >= 0; --i) {
            // Extract information from current experience
            float reward = batch[i].reward;
            float value = batch[i].value;
            bool done = batch[i].done;
            
            // Compute the return (discounted sum of rewards)
            float ret = reward + gamma * nextValue * (1.0f - done);
            returns.push_back(ret);
            
            // Compute the advantage
            float advantage = ret - value;
            advantages.push_back(advantage);
            
            // Update nextValue for the next iteration
            nextValue = ret;
        }
        
        // Reverse the vectors since we computed them in reverse order
        std::reverse(advantages.begin(), advantages.end());
        std::reverse(returns.begin(), returns.end());
        
        // Lock the global model for update
        std::lock_guard<std::mutex> lock(globalMutex);
        
        // Compute total gradients
        float totalPolicyLoss = 0.0f;
        float totalValueLoss = 0.0f;
        float totalEntropyLoss = 0.0f;
        
        // Process each experience
        for (size_t i = 0; i < batch.size(); ++i) {
            const auto& exp = batch[i];
            float advantage = advantages[i];
            float targetValue = returns[i];
            
            // Compute current log probability and value
            AIInputFrame stateFrame = convertVectorToInputFrame(exp.state);
            AIOutputAction action = convertVectorToOutputAction(exp.action);
            float logProb = exp.log_prob;
            float value = exp.value;
            
            // Compute losses (simpler than PPO)
            float policyLoss = -logProb * advantage;
            float valueLoss = 0.5f * (value - targetValue) * (value - targetValue);
            float entropy = -logProb * 0.1f; // Simulated entropy
            
            // Accumulate gradients
            totalPolicyLoss += policyLoss;
            totalValueLoss += valueLoss;
            totalEntropyLoss += entropy;
        }
        
        // Compute average losses
        float avgPolicyLoss = totalPolicyLoss / batch.size();
        float avgValueLoss = totalValueLoss / batch.size();
        float avgEntropyLoss = totalEntropyLoss / batch.size();
        
        // Compute combined loss
        float totalLoss = avgPolicyLoss + 0.5f * avgValueLoss - 0.01f * avgEntropyLoss;
        
        // Print losses
        std::cout << "A3C Update"
                  << ", Policy Loss: " << avgPolicyLoss
                  << ", Value Loss: " << avgValueLoss
                  << ", Entropy: " << avgEntropyLoss
                  << ", Total Loss: " << totalLoss << std::endl;
        
        // Update the global policy with the computed gradients
        // This would involve applying the accumulated gradients to the policy network
        // For our implementation, simulate updating the global network
        // globalPolicy->update(batch, advantages, returns, learningRate);
        
        // After updating, synchronize all worker models with the global model
        synchronizeWorkers();
    }
}

void A3CAlgorithm::processStep(const AIInputFrame& state, const AIOutputAction& action, 
                             float reward, const AIInputFrame& next_state, bool done) {
    // First process step as in base class
    RLAlgorithm::processStep(state, action, reward, next_state, done);
    
    // In A3C, processStep is mainly used to collect experience in the main thread
    // The actual learning happens in the worker threads
}

void A3CAlgorithm::setHyperparameters(const std::unordered_map<std::string, float>& params) {
    // First set base class parameters
    RLAlgorithm::setHyperparameters(params);
    
    // No specific A3C parameters yet
}

bool A3CAlgorithm::save(const std::string& path) {
    // Save global policy
    std::cout << "Saving A3C model to " << path << std::endl;
    
    // Save hyperparameters
    std::string hyperparamsPath = path + ".params";
    std::ofstream file(hyperparamsPath);
    if (file.is_open()) {
        file << "learning_rate=" << learningRate << std::endl;
        file << "gamma=" << gamma << std::endl;
        file << "num_workers=" << numWorkers << std::endl;
        file.close();
    }
    
    // Save global policy (would call policy->save())
    std::string policyPath = path + ".policy";
    // globalPolicy->save(policyPath);
    
    return true;
}

bool A3CAlgorithm::load(const std::string& path) {
    // Load global policy
    std::cout << "Loading A3C model from " << path << std::endl;
    
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
                } else if (key == "num_workers") {
                    // Number of workers can't change after initialization
                    int loadedWorkers = std::stoi(value);
                    if (loadedWorkers != numWorkers) {
                        std::cout << "Warning: Loaded model has " << loadedWorkers 
                                 << " workers, but current instance has " << numWorkers 
                                 << ". Using current number." << std::endl;
                    }
                }
            }
        }
        file.close();
    }
    
    // Load global policy
    std::string policyPath = path + ".policy";
    bool success = globalPolicy->load(policyPath);
    
    if (success) {
        // Update worker policies with the loaded global policy
        synchronizeWorkers();
    }
    
    return success;
}

void A3CAlgorithm::startWorkers() {
    // Start worker threads
    shouldStop = false;
    
    for (auto& worker : workers) {
        if (!worker->running) {
            worker->running = true;
            worker->thread = std::thread(&A3CAlgorithm::workerFunction, this, worker.get());
        }
    }
    
    std::cout << "Started " << numWorkers << " A3C worker threads" << std::endl;
}

void A3CAlgorithm::stopWorkers() {
    // Signal all workers to stop
    shouldStop = true;
    
    // Wait for all workers to finish
    for (auto& worker : workers) {
        if (worker->running && worker->thread.joinable()) {
            worker->thread.join();
            worker->running = false;
        }
    }
    
    std::cout << "Stopped all A3C worker threads" << std::endl;
}

void A3CAlgorithm::workerFunction(WorkerState* state) {
    std::cout << "A3C Worker " << state->id << " started" << std::endl;
    
    // Create a random number generator for this worker
    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    
    // Each worker runs episodes until signaled to stop
    while (!shouldStop) {
        try {
            // Start a new episode
            std::vector<Experience> trajectory;
            trajectory.reserve(1000); // Reserve space for a typical episode
            
            // Initial state
            AIInputFrame state{};
            state.width = 320;
            state.height = 240;
            state.frameBuffer = malloc(320 * 240 * 4); // RGBA
            
            // Fill with random data for simulation
            uint8_t* buffer = static_cast<uint8_t*>(state.frameBuffer);
            for (int i = 0; i < 320 * 240 * 4; ++i) {
                buffer[i] = static_cast<uint8_t>(dist(rng) * 255);
            }
            
            // Simulate an episode
            float episodeReward = 0.0f;
            bool done = false;
            int timestep = 0;
            
            while (!done && !shouldStop && timestep < 1000) {
                // Select action using worker's policy
                AIOutputAction action;
                if (state->policy) {
                    state->policy->predict(state, action, false); // Use exploration
                } else {
                    // Fallback to random actions if no policy
                    action.up = dist(rng) > 0.8f;
                    action.down = dist(rng) > 0.8f;
                    action.left = dist(rng) > 0.8f;
                    action.right = dist(rng) > 0.8f;
                    for (int i = 0; i < 6; ++i) {
                        action.buttons[i] = dist(rng) > 0.8f;
                    }
                }
                
                // Create next state (copy of current with some changes)
                AIInputFrame nextState = state;
                nextState.frameBuffer = malloc(320 * 240 * 4);
                uint8_t* nextBuffer = static_cast<uint8_t*>(nextState.frameBuffer);
                memcpy(nextBuffer, buffer, 320 * 240 * 4);
                
                // Simulate some changes based on action
                for (int i = 0; i < 320 * 240 * 4; ++i) {
                    nextBuffer[i] = static_cast<uint8_t>((nextBuffer[i] + 5) % 256);
                }
                
                // Calculate reward
                float reward = dist(rng) * 2.0f - 1.0f; // Random reward between -1 and 1
                episodeReward += reward;
                
                // Check if episode is done
                done = (timestep >= 999) || (dist(rng) < 0.01f); // 1% chance of termination each step
                
                // Compute log probability and value using worker's policy
                float logProb = -1.0f; // Default value
                float value = 0.0f;    // Default value
                
                if (state->policy) {
                    // For actual implementation, get values from policy
                    // logProb = state->policy->computeLogProb(state, action);
                    // value = state->policy->getValue(state);
                }
                
                // Create experience
                Experience exp;
                
                // Convert state to vector
                exp.state.resize(100); // Simplified representation
                for (int i = 0; i < 100; ++i) {
                    exp.state[i] = buffer[i * 100] / 255.0f;
                }
                
                // Convert action to vector
                exp.action.resize(10);
                exp.action[0] = action.up ? 1.0f : 0.0f;
                exp.action[1] = action.down ? 1.0f : 0.0f;
                exp.action[2] = action.left ? 1.0f : 0.0f;
                exp.action[3] = action.right ? 1.0f : 0.0f;
                for (int i = 0; i < 6; ++i) {
                    exp.action[i + 4] = action.buttons[i] ? 1.0f : 0.0f;
                }
                
                // Convert next state to vector
                exp.next_state.resize(100);
                for (int i = 0; i < 100; ++i) {
                    exp.next_state[i] = nextBuffer[i * 100] / 255.0f;
                }
                
                // Set other fields
                exp.reward = reward;
                exp.done = done;
                exp.log_prob = logProb;
                exp.value = value;
                
                // Add to trajectory
                trajectory.push_back(exp);
                
                // Update state
                free(state.frameBuffer);
                state = nextState;
                buffer = nextBuffer;
                
                // Increment timestep
                timestep++;
                
                // Short sleep to avoid consuming too much CPU
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                
                // Update global network every N steps
                if (trajectory.size() >= 20) {
                    updateGlobalNetwork(state, trajectory);
                    trajectory.clear();
                }
            }
            
            // Free any remaining buffers
            if (state.frameBuffer) {
                free(state.frameBuffer);
                state.frameBuffer = nullptr;
            }
            
            // Update global network with any remaining experiences
            if (!trajectory.empty()) {
                updateGlobalNetwork(state, trajectory);
            }
            
            std::cout << "A3C Worker " << state->id << " completed episode, reward: " << episodeReward << std::endl;
            
            // Small sleep between episodes
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        } catch (const std::exception& e) {
            std::cerr << "A3C Worker " << state->id << " error: " << e.what() << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    
    std::cout << "A3C Worker " << state->id << " stopped" << std::endl;
}

void A3CAlgorithm::updateGlobalNetwork(WorkerState* state, const std::vector<Experience>& trajectory) {
    if (trajectory.empty()) {
        return;
    }
    
    // Use Metal acceleration if available
    if (metalCommandQueue && aggregateGradientsPipeline && applyGradientsPipeline && state->gradientBuffer) {
        updateGlobalNetworkWithMetal(state, trajectory);
    } else {
        // Original CPU implementation
        // Calculate advantages and returns
        std::vector<float> advantages;
        std::vector<float> returns;
        advantages.reserve(trajectory.size());
        returns.reserve(trajectory.size());
        
        // Last value is 0 if done, otherwise bootstrap from the value function
        float nextValue = 0.0f;
        if (!trajectory.back().done) {
            // Get value estimate from the worker's policy
            if (state->policy) {
                AIInputFrame nextState = convertVectorToInputFrame(trajectory.back().next_state);
                nextValue = state->policy->getValue(nextState);
            }
        }
        
        // Compute advantages and returns in reverse order
        for (int i = trajectory.size() - 1; i >= 0; --i) {
            // Calculate returns and advantages (same as original implementation)
            float reward = trajectory[i].reward;
            float value = trajectory[i].value;
            bool done = trajectory[i].done;
            
            // Compute the return (discounted sum of rewards)
            float ret = reward + gamma * nextValue * (1.0f - done);
            returns.push_back(ret);
            
            // Compute the advantage
            float advantage = ret - value;
            advantages.push_back(advantage);
            
            // Update nextValue for the next iteration
            nextValue = ret;
        }
        
        // Reverse the vectors since we computed them in reverse order
        std::reverse(advantages.begin(), advantages.end());
        std::reverse(returns.begin(), returns.end());
        
        // Lock the global model for update
        std::lock_guard<std::mutex> lock(globalMutex);
        
        // Compute total gradients
        float totalPolicyLoss = 0.0f;
        float totalValueLoss = 0.0f;
        float totalEntropyLoss = 0.0f;
        
        // Process each experience
        for (size_t i = 0; i < trajectory.size(); ++i) {
            const auto& exp = trajectory[i];
            float advantage = advantages[i];
            float targetValue = returns[i];
            
            // Compute current log probability and value
            AIInputFrame stateFrame = convertVectorToInputFrame(exp.state);
            AIOutputAction action = convertVectorToOutputAction(exp.action);
            float logProb = exp.log_prob;
            float value = exp.value;
            
            // Compute losses
            float policyLoss = -logProb * advantage;
            float valueLoss = 0.5f * (value - targetValue) * (value - targetValue);
            float entropy = -logProb * 0.1f; // Simulated entropy
            
            // Accumulate gradients
            totalPolicyLoss += policyLoss;
            totalValueLoss += valueLoss;
            totalEntropyLoss += entropy;
        }
        
        // Compute average losses
        float avgPolicyLoss = totalPolicyLoss / trajectory.size();
        float avgValueLoss = totalValueLoss / trajectory.size();
        float avgEntropyLoss = totalEntropyLoss / trajectory.size();
        
        // Compute combined loss
        float totalLoss = avgPolicyLoss + 0.5f * avgValueLoss - 0.01f * avgEntropyLoss;
        
        // Print losses (less frequently to avoid spam)
        if (state->id == 0 || dist(rng) < 0.05f) {
            std::cout << "A3C Worker " << state->id << " Update"
                      << ", Policy Loss: " << avgPolicyLoss
                      << ", Value Loss: " << avgValueLoss
                      << ", Entropy: " << avgEntropyLoss
                      << ", Batch Size: " << trajectory.size() << std::endl;
        }
        
        // Update the global policy with the computed gradients
        // This would involve applying the accumulated gradients to the policy network
        // For our implementation, simulate updating the global network
        // globalPolicy->update(trajectory, advantages, returns, learningRate);
        
        // Update worker's policy with the global policy
        if (state->policy) {
            state->policy->copyFrom(globalPolicy);
        }
    }
}

// Add a helper method to convert between vector representation and AIInputFrame
AIInputFrame A3CAlgorithm::convertVectorToInputFrame(const std::vector<float>& vec) {
    AIInputFrame frame{};
    // For now, this is a simple placeholder implementation
    // In a real implementation, this would convert the vector to a proper frame
    frame.width = 100;
    frame.height = 1;
    frame.frameBuffer = nullptr;
    return frame;
}

// Add a helper method to convert between vector representation and AIOutputAction
AIOutputAction A3CAlgorithm::convertVectorToOutputAction(const std::vector<float>& vec) {
    AIOutputAction action{};
    // Simple conversion from vector to action
    if (vec.size() >= 4) {
        action.up = vec[0] > 0.5f;
        action.down = vec[1] > 0.5f;
        action.left = vec[2] > 0.5f;
        action.right = vec[3] > 0.5f;
        
        for (int i = 0; i < 6 && i + 4 < vec.size(); ++i) {
            action.buttons[i] = vec[i + 4] > 0.5f;
        }
    }
    return action;
}

// Add a method to synchronize all worker policies with the global policy
void A3CAlgorithm::synchronizeWorkers() {
    // Use Metal acceleration if available
    if (metalCommandQueue && syncWeightsPipeline && globalPolicy) {
        // Get global weights buffer
        id<MTLBuffer> globalWeightsBuffer = globalPolicy->getMetalWeightsBuffer();
        if (!globalWeightsBuffer) {
            // If policy doesn't support direct Metal buffer access,
            // extract weights and create a temporary buffer
            std::vector<float> weights;
            globalPolicy->getWeights(weights);
            globalWeightsBuffer = [metalDevice newBufferWithBytes:weights.data()
                                                         length:weights.size() * sizeof(float)
                                                        options:MTLResourceStorageModeShared];
        }
        
        // Synchronize each worker
        for (auto& worker : workers) {
            if (worker->policy) {
                id<MTLCommandBuffer> syncBuffer = [metalCommandQueue commandBuffer];
                id<MTLComputeCommandEncoder> syncEncoder = [syncBuffer computeCommandEncoder];
                [syncEncoder setComputePipelineState:syncWeightsPipeline];
                [syncEncoder setBuffer:globalWeightsBuffer offset:0 atIndex:0];
                
                // Get worker's weights buffer
                id<MTLBuffer> workerWeightsBuffer = worker->policy->getMetalWeightsBuffer();
                if (!workerWeightsBuffer) {
                    // If policy doesn't support direct Metal buffer access,
                    // create a temporary buffer
                    workerWeightsBuffer = [metalDevice newBufferWithLength:globalWeightsBuffer.length
                                                                 options:MTLResourceStorageModeShared];
                }
                
                [syncEncoder setBuffer:workerWeightsBuffer offset:0 atIndex:1];
                
                // Execute the compute kernel
                NSUInteger threadGroupSize = syncWeightsPipeline.maxTotalThreadsPerThreadgroup;
                NSUInteger numThreads = globalWeightsBuffer.length / sizeof(float);
                if (threadGroupSize > numThreads) threadGroupSize = numThreads;
                
                [syncEncoder dispatchThreads:MTLSizeMake(numThreads, 1, 1)
                  threadsPerThreadgroup:MTLSizeMake(threadGroupSize, 1, 1)];
                [syncEncoder endEncoding];
                
                [syncBuffer commit];
                [syncBuffer waitUntilCompleted];
                
                // If we created a temporary buffer, update the worker's policy
                if (workerWeightsBuffer != worker->policy->getMetalWeightsBuffer()) {
                    std::vector<float> syncedWeights(numThreads);
                    memcpy(syncedWeights.data(), [workerWeightsBuffer contents], numThreads * sizeof(float));
                    worker->policy->setWeights(syncedWeights);
                }
            }
        }
    } else {
        // Fallback to CPU sync
        for (auto& worker : workers) {
            if (worker->policy) {
                worker->policy->copyFrom(globalPolicy);
            }
        }
    }
}

// Initialize Metal resources
void A3CAlgorithm::initializeMetalResources() {
    // Get Metal device
    metalDevice = MTLCreateSystemDefaultDevice();
    if (!metalDevice) {
        std::cerr << "A3C: Metal is not supported on this device" << std::endl;
        return;
    }
    
    // Create command queue
    metalCommandQueue = [metalDevice newCommandQueue];
    
    // Load Metal compute kernels for A3C
    NSError* error = nil;
    NSString* kernelSource = @R"(
        #include <metal_stdlib>
        using namespace metal;
        
        // A3C gradient aggregation kernel
        kernel void a3c_aggregate_gradients(
            device const float* worker_gradients [[buffer(0)]],
            device float* global_gradients [[buffer(1)]],
            device const float* worker_counts [[buffer(2)]],
            uint id [[thread_position_in_grid]]
        ) {
            // Add worker gradient to global gradient buffer
            global_gradients[id] += worker_gradients[id];
        }
        
        // A3C apply gradients kernel
        kernel void a3c_apply_gradients(
            device float* weights [[buffer(0)]],
            device const float* gradients [[buffer(1)]],
            device const float* learning_rates [[buffer(2)]],
            uint id [[thread_position_in_grid]]
        ) {
            // Apply gradient with learning rate
            weights[id] -= learning_rates[0] * gradients[id];
        }
        
        // A3C sync weights kernel
        kernel void a3c_sync_weights(
            device const float* global_weights [[buffer(0)]],
            device float* worker_weights [[buffer(1)]],
            uint id [[thread_position_in_grid]]
        ) {
            // Copy global weights to worker weights
            worker_weights[id] = global_weights[id];
        }
    )";
    
    // Create Metal library
    metalLibrary = [metalDevice newLibraryWithSource:kernelSource options:nil error:&error];
    if (!metalLibrary) {
        std::cerr << "A3C: Failed to create Metal library: " << [error.localizedDescription UTF8String] << std::endl;
        return;
    }
    
    // Get compute functions
    id<MTLFunction> aggregateFunction = [metalLibrary newFunctionWithName:@"a3c_aggregate_gradients"];
    id<MTLFunction> applyFunction = [metalLibrary newFunctionWithName:@"a3c_apply_gradients"];
    id<MTLFunction> syncFunction = [metalLibrary newFunctionWithName:@"a3c_sync_weights"];
    
    if (!aggregateFunction || !applyFunction || !syncFunction) {
        std::cerr << "A3C: Failed to create Metal compute functions" << std::endl;
        return;
    }
    
    // Create compute pipelines
    aggregateGradientsPipeline = [metalDevice newComputePipelineStateWithFunction:aggregateFunction error:&error];
    applyGradientsPipeline = [metalDevice newComputePipelineStateWithFunction:applyFunction error:&error];
    syncWeightsPipeline = [metalDevice newComputePipelineStateWithFunction:syncFunction error:&error];
    
    if (!aggregateGradientsPipeline || !applyGradientsPipeline || !syncWeightsPipeline) {
        std::cerr << "A3C: Failed to create Metal compute pipelines: " << [error.localizedDescription UTF8String] << std::endl;
        return;
    }
    
    // Create global gradients buffer
    if (globalPolicy) {
        size_t gradientSize = calculateGradientBufferSize(globalPolicy);
        globalGradientsBuffer = [metalDevice newBufferWithLength:gradientSize 
                                                         options:MTLResourceStorageModeShared];
        globalWeightsBuffer = [metalDevice newBufferWithLength:gradientSize 
                                                      options:MTLResourceStorageModeShared];
        
        // Initialize global gradients to zero
        memset([globalGradientsBuffer contents], 0, gradientSize);
        
        // Initialize learning rate buffer
        learningRateBuffer = [metalDevice newBufferWithLength:sizeof(float) 
                                                     options:MTLResourceStorageModeShared];
        float initialLR = learningRate;
        memcpy([learningRateBuffer contents], &initialLR, sizeof(float));
        
        // Initialize worker count buffer
        workerCountBuffer = [metalDevice newBufferWithLength:sizeof(float) 
                                                    options:MTLResourceStorageModeShared];
        float workerCount = static_cast<float>(numWorkers);
        memcpy([workerCountBuffer contents], &workerCount, sizeof(float));
    }
    
    std::cout << "A3C: Successfully initialized Metal compute resources" << std::endl;
}

// Clean up Metal resources
void A3CAlgorithm::cleanupMetalResources() {
    // Release Metal resources (ARC will handle most of the cleanup)
    metalLibrary = nil;
    aggregateGradientsPipeline = nil;
    applyGradientsPipeline = nil;
    syncWeightsPipeline = nil;
    globalGradientsBuffer = nil;
    globalWeightsBuffer = nil;
    learningRateBuffer = nil;
    workerCountBuffer = nil;
    metalCommandQueue = nil;
    metalDevice = nil;
}

// Calculate gradient buffer size for a policy
size_t A3CAlgorithm::calculateGradientBufferSize(const AITorchPolicy* policy) {
    // In a real implementation, this would get the actual model parameter count
    // For this stub, we'll estimate a reasonable size for a neural network
    const size_t estimatedParamCount = 100000; // ~100K parameters
    return estimatedParamCount * sizeof(float);
}

// Metal-accelerated version of train
void A3CAlgorithm::trainWithMetal(const std::vector<Experience>& batch) {
    // Calculate advantages and returns
    std::vector<float> advantages;
    std::vector<float> returns;
    advantages.reserve(batch.size());
    returns.reserve(batch.size());
    
    // Last value is 0 if done, otherwise bootstrap from the value function
    float nextValue = 0.0f;
    if (!batch.back().done) {
        // Get value estimate from the global policy
        nextValue = globalPolicy->getValue(convertVectorToInputFrame(batch.back().next_state));
    }
    
    // Compute advantages and returns in reverse order
    for (int i = batch.size() - 1; i >= 0; --i) {
        // Extract information from current experience
        float reward = batch[i].reward;
        float value = batch[i].value;
        bool done = batch[i].done;
        
        // Compute the return (discounted sum of rewards)
        float ret = reward + gamma * nextValue * (1.0f - done);
        returns.push_back(ret);
        
        // Compute the advantage
        float advantage = ret - value;
        advantages.push_back(advantage);
        
        // Update nextValue for the next iteration
        nextValue = ret;
    }
    
    // Reverse the vectors since we computed them in reverse order
    std::reverse(advantages.begin(), advantages.end());
    std::reverse(returns.begin(), returns.end());
    
    // Extract gradients to Metal buffer
    std::vector<float> gradients;
    globalPolicy->calculateGradients(batch, advantages, returns, gradients);
    
    // Copy gradients to Metal buffer
    size_t gradientSize = gradients.size() * sizeof(float);
    if (gradientSize > 0 && gradientSize <= globalGradientsBuffer.length) {
        memcpy([globalGradientsBuffer contents], gradients.data(), gradientSize);
    }
    
    // Apply gradients using Metal
    id<MTLCommandBuffer> commandBuffer = [metalCommandQueue commandBuffer];
    id<MTLComputeCommandEncoder> encoder = [commandBuffer computeCommandEncoder];
    [encoder setComputePipelineState:applyGradientsPipeline];
    
    // Get weights buffer from policy
    id<MTLBuffer> weightsBuffer = globalPolicy->getMetalWeightsBuffer();
    if (!weightsBuffer) {
        // If policy doesn't support direct Metal buffer access,
        // extract weights and create a temporary buffer
        std::vector<float> weights;
        globalPolicy->getWeights(weights);
        weightsBuffer = [metalDevice newBufferWithBytes:weights.data()
                                                 length:weights.size() * sizeof(float)
                                                options:MTLResourceStorageModeShared];
    }
    
    [encoder setBuffer:weightsBuffer offset:0 atIndex:0];
    [encoder setBuffer:globalGradientsBuffer offset:0 atIndex:1];
    [encoder setBuffer:learningRateBuffer offset:0 atIndex:2];
    
    // Execute the compute kernel
    NSUInteger threadGroupSize = applyGradientsPipeline.maxTotalThreadsPerThreadgroup;
    NSUInteger numThreads = gradientSize / sizeof(float);
    if (threadGroupSize > numThreads) threadGroupSize = numThreads;
    
    [encoder dispatchThreads:MTLSizeMake(numThreads, 1, 1)
      threadsPerThreadgroup:MTLSizeMake(threadGroupSize, 1, 1)];
    [encoder endEncoding];
    
    [commandBuffer commit];
    [commandBuffer waitUntilCompleted];
    
    // If we created a temporary weights buffer, update the policy
    if (weightsBuffer != globalPolicy->getMetalWeightsBuffer()) {
        std::vector<float> updatedWeights(numThreads);
        memcpy(updatedWeights.data(), [weightsBuffer contents], numThreads * sizeof(float));
        globalPolicy->setWeights(updatedWeights);
    }
    
    // After updating, synchronize all worker models with the global model
    synchronizeWorkers();
}

// Metal-accelerated version of updateGlobalNetwork
void A3CAlgorithm::updateGlobalNetworkWithMetal(WorkerState* state, const std::vector<Experience>& trajectory) {
    // Compute advantages and returns
    std::vector<float> advantages;
    std::vector<float> returns;
    advantages.reserve(trajectory.size());
    returns.reserve(trajectory.size());
    
    // Last value is 0 if done, otherwise bootstrap from the value function
    float nextValue = 0.0f;
    if (!trajectory.back().done) {
        // Get value estimate from the worker's policy
        if (state->policy) {
            AIInputFrame nextState = convertVectorToInputFrame(trajectory.back().next_state);
            nextValue = state->policy->getValue(nextState);
        }
    }
    
    // Compute advantages and returns in reverse order
    for (int i = trajectory.size() - 1; i >= 0; --i) {
        // Calculate returns and advantages (same as original implementation)
        float reward = trajectory[i].reward;
        float value = trajectory[i].value;
        bool done = trajectory[i].done;
        
        // Compute the return (discounted sum of rewards)
        float ret = reward + gamma * nextValue * (1.0f - done);
        returns.push_back(ret);
        
        // Compute the advantage
        float advantage = ret - value;
        advantages.push_back(advantage);
        
        // Update nextValue for the next iteration
        nextValue = ret;
    }
    
    // Reverse the vectors since we computed them in reverse order
    std::reverse(advantages.begin(), advantages.end());
    std::reverse(returns.begin(), returns.end());
    
    // Extract gradients from worker's policy
    std::vector<float> gradients;
    state->policy->calculateGradients(trajectory, advantages, returns, gradients);
    
    // Copy gradients to worker's Metal buffer
    size_t gradientSize = gradients.size() * sizeof(float);
    if (gradientSize > 0 && gradientSize <= state->gradientBuffer.length) {
        memcpy([state->gradientBuffer contents], gradients.data(), gradientSize);
    }
    
    // Lock the global model for update
    std::lock_guard<std::mutex> lock(globalMutex);
    
    // Aggregate worker gradients to global gradients using Metal
    id<MTLCommandBuffer> commandBuffer = [metalCommandQueue commandBuffer];
    id<MTLComputeCommandEncoder> encoder = [commandBuffer computeCommandEncoder];
    [encoder setComputePipelineState:aggregateGradientsPipeline];
    [encoder setBuffer:state->gradientBuffer offset:0 atIndex:0];
    [encoder setBuffer:globalGradientsBuffer offset:0 atIndex:1];
    [encoder setBuffer:workerCountBuffer offset:0 atIndex:2];
    
    // Execute the compute kernel
    NSUInteger threadGroupSize = aggregateGradientsPipeline.maxTotalThreadsPerThreadgroup;
    NSUInteger numThreads = gradientSize / sizeof(float);
    if (threadGroupSize > numThreads) threadGroupSize = numThreads;
    
    [encoder dispatchThreads:MTLSizeMake(numThreads, 1, 1)
      threadsPerThreadgroup:MTLSizeMake(threadGroupSize, 1, 1)];
    [encoder endEncoding];
    
    [commandBuffer commit];
    [commandBuffer waitUntilCompleted];
    
    // Apply gradients to global policy
    id<MTLCommandBuffer> applyBuffer = [metalCommandQueue commandBuffer];
    id<MTLComputeCommandEncoder> applyEncoder = [applyBuffer computeCommandEncoder];
    [applyEncoder setComputePipelineState:applyGradientsPipeline];
    
    // Get weights buffer from global policy
    id<MTLBuffer> weightsBuffer = globalPolicy->getMetalWeightsBuffer();
    if (!weightsBuffer) {
        // If policy doesn't support direct Metal buffer access,
        // extract weights and create a temporary buffer
        std::vector<float> weights;
        globalPolicy->getWeights(weights);
        weightsBuffer = [metalDevice newBufferWithBytes:weights.data()
                                                 length:weights.size() * sizeof(float)
                                                options:MTLResourceStorageModeShared];
    }
    
    [applyEncoder setBuffer:weightsBuffer offset:0 atIndex:0];
    [applyEncoder setBuffer:globalGradientsBuffer offset:0 atIndex:1];
    [applyEncoder setBuffer:learningRateBuffer offset:0 atIndex:2];
    
    // Execute the compute kernel
    [applyEncoder dispatchThreads:MTLSizeMake(numThreads, 1, 1)
          threadsPerThreadgroup:MTLSizeMake(threadGroupSize, 1, 1)];
    [applyEncoder endEncoding];
    
    [applyBuffer commit];
    [applyBuffer waitUntilCompleted];
    
    // If we created a temporary weights buffer, update the global policy
    if (weightsBuffer != globalPolicy->getMetalWeightsBuffer()) {
        std::vector<float> updatedWeights(numThreads);
        memcpy(updatedWeights.data(), [weightsBuffer contents], numThreads * sizeof(float));
        globalPolicy->setWeights(updatedWeights);
    }
    
    // Reset global gradients buffer for next worker
    memset([globalGradientsBuffer contents], 0, globalGradientsBuffer.length);
    
    // Update worker's policy with the global policy
    if (state->policy) {
        if (metalCommandQueue && syncWeightsPipeline) {
            // Use Metal to sync weights
            id<MTLCommandBuffer> syncBuffer = [metalCommandQueue commandBuffer];
            id<MTLComputeCommandEncoder> syncEncoder = [syncBuffer computeCommandEncoder];
            [syncEncoder setComputePipelineState:syncWeightsPipeline];
            [syncEncoder setBuffer:weightsBuffer offset:0 atIndex:0];
            
            // Get worker's weights buffer
            id<MTLBuffer> workerWeightsBuffer = state->policy->getMetalWeightsBuffer();
            if (!workerWeightsBuffer) {
                // If policy doesn't support direct Metal buffer access,
                // create a temporary buffer
                workerWeightsBuffer = [metalDevice newBufferWithLength:numThreads * sizeof(float)
                                                             options:MTLResourceStorageModeShared];
            }
            
            [syncEncoder setBuffer:workerWeightsBuffer offset:0 atIndex:1];
            
            // Execute the compute kernel
            [syncEncoder dispatchThreads:MTLSizeMake(numThreads, 1, 1)
              threadsPerThreadgroup:MTLSizeMake(threadGroupSize, 1, 1)];
            [syncEncoder endEncoding];
            
            [syncBuffer commit];
            [syncBuffer waitUntilCompleted];
            
            // If we created a temporary buffer, update the worker's policy
            if (workerWeightsBuffer != state->policy->getMetalWeightsBuffer()) {
                std::vector<float> syncedWeights(numThreads);
                memcpy(syncedWeights.data(), [workerWeightsBuffer contents], numThreads * sizeof(float));
                state->policy->setWeights(syncedWeights);
            }
        } else {
            // Fallback to CPU sync
            state->policy->copyFrom(globalPolicy);
        }
    }
}

} // namespace ai
} // namespace fbneo 