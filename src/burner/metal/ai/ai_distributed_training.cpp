#include "ai_distributed_training.h"
#include <algorithm>
#include <iostream>
#include <fstream>
#include <numeric>
#include <chrono>
#include <sstream>
#include <random>

namespace fbneo {
namespace ai {

DistributedTrainer::DistributedTrainer(AITorchPolicy* globalPolicy, int numWorkers)
    : globalPolicy(globalPolicy), shouldStop(false), totalEpisodesCompleted(0),
      totalTrainingReward(0.0f), totalTrainingSteps(0),
      learningRate(0.0003f), gamma(0.99f), syncFrequency(5),
      useExperienceSharing(true), sharedBufferSize(10000), algorithmType("a3c") {
    
    // Initialize workers
    workers.reserve(numWorkers);
    for (int i = 0; i < numWorkers; ++i) {
        auto worker = std::make_unique<WorkerState>();
        worker->id = i;
        worker->running = false;
        worker->episodesCompleted = 0;
        worker->totalReward = 0.0f;
        worker->stepsCompleted = 0;
        
        // Create a copy of the global policy for this worker
        worker->policy = std::unique_ptr<AITorchPolicy>(globalPolicy->clone());
        
        workers.push_back(std::move(worker));
    }
    
    // Initialize shared experience buffer
    sharedBuffer = ExperienceBuffer(sharedBufferSize);
}

DistributedTrainer::~DistributedTrainer() {
    // Stop training if it's still running
    stopTraining();
}

bool DistributedTrainer::startTraining(int episodesPerWorker) {
    // Reset counters
    totalEpisodesCompleted = 0;
    totalTrainingReward = 0.0f;
    totalTrainingSteps = 0;
    shouldStop = false;
    
    // Start worker threads
    for (auto& worker : workers) {
        worker->running = true;
        worker->thread = std::thread(&DistributedTrainer::workerFunction, this, worker.get(), episodesPerWorker);
    }
    
    std::cout << "Started distributed training with " << workers.size() << " workers" << std::endl;
    std::cout << "Algorithm: " << algorithmType << ", Experience sharing: " << (useExperienceSharing ? "enabled" : "disabled") << std::endl;
    
    return true;
}

void DistributedTrainer::stopTraining() {
    // Signal all workers to stop
    shouldStop = true;
    
    // Wait for worker threads to finish
    for (auto& worker : workers) {
        if (worker->running && worker->thread.joinable()) {
            worker->thread.join();
            worker->running = false;
        }
    }
    
    std::cout << "Stopped distributed training" << std::endl;
    std::cout << "Total episodes completed: " << totalEpisodesCompleted << std::endl;
    std::cout << "Total training steps: " << totalTrainingSteps << std::endl;
    std::cout << "Average reward per episode: " << (totalEpisodesCompleted > 0 ? totalTrainingReward / totalEpisodesCompleted : 0.0f) << std::endl;
}

bool DistributedTrainer::saveModel(const std::string& path) {
    // Lock global policy while saving
    std::lock_guard<std::mutex> lock(globalMutex);
    
    // Save the global policy
    bool success = globalPolicy->save(path);
    
    // Save hyperparameters
    std::string hyperparamsPath = path + ".dist_params";
    std::ofstream file(hyperparamsPath);
    if (file.is_open()) {
        file << "learning_rate=" << learningRate << std::endl;
        file << "gamma=" << gamma << std::endl;
        file << "sync_frequency=" << syncFrequency << std::endl;
        file << "experience_sharing=" << (useExperienceSharing ? "true" : "false") << std::endl;
        file << "shared_buffer_size=" << sharedBufferSize << std::endl;
        file << "algorithm=" << algorithmType << std::endl;
        file << "num_workers=" << workers.size() << std::endl;
        file.close();
    }
    
    return success;
}

bool DistributedTrainer::loadModel(const std::string& path) {
    // Load hyperparameters
    std::string hyperparamsPath = path + ".dist_params";
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
                } else if (key == "sync_frequency") {
                    syncFrequency = std::stoi(value);
                } else if (key == "experience_sharing") {
                    useExperienceSharing = (value == "true");
                } else if (key == "shared_buffer_size") {
                    sharedBufferSize = std::stoi(value);
                    sharedBuffer = ExperienceBuffer(sharedBufferSize);
                } else if (key == "algorithm") {
                    algorithmType = value;
                }
            }
        }
        file.close();
    }
    
    // Lock global policy while loading
    std::lock_guard<std::mutex> lock(globalMutex);
    
    // Load the global policy
    bool success = globalPolicy->load(path);
    
    // Update worker policies
    if (success) {
        for (auto& worker : workers) {
            synchronizeWorker(worker.get());
        }
    }
    
    return success;
}

void DistributedTrainer::setHyperparameters(const std::unordered_map<std::string, float>& params) {
    // Set hyperparameters
    if (params.count("learning_rate")) {
        learningRate = params.at("learning_rate");
    }
    if (params.count("gamma")) {
        gamma = params.at("gamma");
    }
    if (params.count("sync_frequency")) {
        syncFrequency = static_cast<int>(params.at("sync_frequency"));
    }
}

bool DistributedTrainer::setAlgorithm(const std::string& algorithm) {
    // Check if algorithm is valid
    if (algorithm == "ppo" || algorithm == "a3c") {
        algorithmType = algorithm;
        return true;
    }
    return false;
}

void DistributedTrainer::setExperienceSharing(bool enable, int bufferSize) {
    useExperienceSharing = enable;
    if (bufferSize > 0) {
        sharedBufferSize = bufferSize;
        sharedBuffer = ExperienceBuffer(sharedBufferSize);
    }
}

void DistributedTrainer::setSynchronizationFrequency(int frequency) {
    if (frequency > 0) {
        syncFrequency = frequency;
    }
}

std::string DistributedTrainer::getStatus() const {
    std::stringstream ss;
    ss << "Distributed Training Status:" << std::endl;
    ss << "Algorithm: " << algorithmType << std::endl;
    ss << "Workers: " << workers.size() << std::endl;
    ss << "Experience Sharing: " << (useExperienceSharing ? "Enabled" : "Disabled") << std::endl;
    ss << "Sync Frequency: Every " << syncFrequency << " steps" << std::endl;
    ss << "Episodes Completed: " << totalEpisodesCompleted << std::endl;
    ss << "Total Steps: " << totalTrainingSteps << std::endl;
    
    // Add per-worker information
    ss << "Worker Status:" << std::endl;
    for (const auto& worker : workers) {
        ss << "  Worker " << worker->id 
           << ": Episodes=" << worker->episodesCompleted 
           << ", Steps=" << worker->stepsCompleted
           << ", Avg.Reward=" << (worker->episodesCompleted > 0 ? worker->totalReward / worker->episodesCompleted : 0.0f)
           << std::endl;
    }
    
    return ss.str();
}

void DistributedTrainer::workerFunction(WorkerState* state, int episodesPerWorker) {
    std::cout << "Worker " << state->id << " started training" << std::endl;
    
    // Random number generation
    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    
    // Reset worker state
    state->episodesCompleted = 0;
    state->totalReward = 0.0f;
    state->stepsCompleted = 0;
    
    // Counter for synchronization
    int stepsSinceSync = 0;
    
    // Run episodes until we reach the target or are told to stop
    while (state->episodesCompleted < episodesPerWorker && !shouldStop) {
        // Start a new episode
        std::vector<Experience> trajectory;
        trajectory.reserve(1000); // Reserve space for a typical episode
        
        // Initial state setup (simplified for demonstration)
        AIInputFrame inputState{};
        inputState.width = 320;
        inputState.height = 240;
        inputState.frameBuffer = malloc(320 * 240 * 4); // RGBA
        
        // Fill with random data for simulation
        uint8_t* buffer = static_cast<uint8_t*>(inputState.frameBuffer);
        for (int i = 0; i < 320 * 240 * 4; ++i) {
            buffer[i] = static_cast<uint8_t>(dist(rng) * 255);
        }
        
        // Episode variables
        float episodeReward = 0.0f;
        bool done = false;
        int timestep = 0;
        
        // Run the episode
        while (!done && !shouldStop && timestep < 10000) { // 10000 step limit
            // Get the current state
            AIInputFrame currState = inputState;
            
            // Get action using worker's policy
            AIOutputAction action;
            state->policy->predict(currState, action, false); // Exploration enabled
            
            // Step environment (simulated)
            AIInputFrame nextState = currState;
            
            // Modify next state based on action (simulated dynamics)
            if (action.up) {
                // Move up in simulated environment
                for (int y = 0; y < 240 - 1; ++y) {
                    for (int x = 0; x < 320; ++x) {
                        int destIdx = (y * 320 + x) * 4;
                        int srcIdx = ((y + 1) * 320 + x) * 4;
                        for (int c = 0; c < 4; ++c) {
                            buffer[destIdx + c] = buffer[srcIdx + c];
                        }
                    }
                }
            } else if (action.down) {
                // Move down in simulated environment
                for (int y = 240 - 1; y > 0; --y) {
                    for (int x = 0; x < 320; ++x) {
                        int destIdx = (y * 320 + x) * 4;
                        int srcIdx = ((y - 1) * 320 + x) * 4;
                        for (int c = 0; c < 4; ++c) {
                            buffer[destIdx + c] = buffer[srcIdx + c];
                        }
                    }
                }
            } else if (action.left) {
                // Move left in simulated environment
                for (int y = 0; y < 240; ++y) {
                    for (int x = 0; x < 320 - 1; ++x) {
                        int destIdx = (y * 320 + x) * 4;
                        int srcIdx = (y * 320 + (x + 1)) * 4;
                        for (int c = 0; c < 4; ++c) {
                            buffer[destIdx + c] = buffer[srcIdx + c];
                        }
                    }
                }
            } else if (action.right) {
                // Move right in simulated environment
                for (int y = 0; y < 240; ++y) {
                    for (int x = 320 - 1; x > 0; --x) {
                        int destIdx = (y * 320 + x) * 4;
                        int srcIdx = (y * 320 + (x - 1)) * 4;
                        for (int c = 0; c < 4; ++c) {
                            buffer[destIdx + c] = buffer[srcIdx + c];
                        }
                    }
                }
            }
            
            // Calculate reward (simulated)
            float reward = -0.01f; // Small negative reward to encourage quick completion
            
            // Add reward for button presses (simulated game objective)
            for (int i = 0; i < 6; ++i) {
                if (action.buttons[i]) {
                    reward += 0.1f; // Reward for pressing buttons
                }
            }
            
            // Check if episode is done (simulated)
            done = (timestep >= 1000) || (dist(rng) < 0.01f); // 1% chance of random termination
            
            // Success is reaching the step limit without random termination
            bool success = timestep >= 1000;
            
            // Add reward for successful completion
            if (done && success) {
                reward += 10.0f;
            }
            
            // Update episode reward
            episodeReward += reward;
            
            // Convert states and action to vectors for experience buffer
            std::vector<float> stateVec;
            std::vector<float> nextStateVec;
            std::vector<float> actionVec;
            
            // Simple conversion of state to vector (grayscale average of RGBA)
            stateVec.reserve(currState.width * currState.height);
            nextStateVec.reserve(nextState.width * nextState.height);
            
            // Convert currState to vector
            uint8_t* currBuffer = static_cast<uint8_t*>(currState.frameBuffer);
            for (int i = 0; i < currState.width * currState.height; ++i) {
                int idx = i * 4;
                float avg = (currBuffer[idx] + currBuffer[idx + 1] + currBuffer[idx + 2]) / (3.0f * 255.0f);
                stateVec.push_back(avg);
            }
            
            // Convert nextState to vector
            uint8_t* nextBuffer = static_cast<uint8_t*>(nextState.frameBuffer);
            for (int i = 0; i < nextState.width * nextState.height; ++i) {
                int idx = i * 4;
                float avg = (nextBuffer[idx] + nextBuffer[idx + 1] + nextBuffer[idx + 2]) / (3.0f * 255.0f);
                nextStateVec.push_back(avg);
            }
            
            // Convert action to vector (10 dimensions: up, down, left, right, 6 buttons)
            actionVec.resize(10, 0.0f);
            actionVec[0] = action.up ? 1.0f : 0.0f;
            actionVec[1] = action.down ? 1.0f : 0.0f;
            actionVec[2] = action.left ? 1.0f : 0.0f;
            actionVec[3] = action.right ? 1.0f : 0.0f;
            for (int i = 0; i < 6; ++i) {
                actionVec[i + 4] = action.buttons[i] ? 1.0f : 0.0f;
            }
            
            // Store experience
            Experience exp{
                stateVec,
                actionVec,
                reward,
                nextStateVec,
                done
            };
            
            // Add to trajectory
            trajectory.push_back(exp);
            
            // Add to shared buffer if experience sharing is enabled
            if (useExperienceSharing) {
                addToSharedBuffer(exp);
            }
            
            // Update state for next step
            currState = nextState;
            
            // Increment step counter
            timestep++;
            state->stepsCompleted++;
            
            // Periodic gradient synchronization
            stepsSinceSync++;
            if (stepsSinceSync >= syncFrequency) {
                // If algorithm is A3C, push gradients to global policy
                if (algorithmType == "a3c") {
                    pushWorkerGradients(state);
                }
                // If algorithm is PPO or other, sync weights from global policy
                else {
                    synchronizeWorker(state);
                }
                stepsSinceSync = 0;
            }
            
            // Update global counters (atomic)
            totalTrainingSteps++;
        }
        
        // Free buffer memory
        free(inputState.frameBuffer);
        
        // Process the completed episode
        if (!shouldStop) {
            // Update worker statistics
            state->episodesCompleted++;
            state->totalReward += episodeReward;
            
            // Update global statistics (atomic)
            totalEpisodesCompleted++;
            totalTrainingReward += episodeReward;
            
            // Train on trajectory
            if (algorithmType == "ppo") {
                // For PPO, we train on the collected trajectory
                trainPPO(state, trajectory);
                
                // After training, synchronize with global policy
                if (state->episodesCompleted % 5 == 0) {
                    pushWorkerUpdates(state);
                    synchronizeWorker(state);
                }
            }
            else if (algorithmType == "a3c") {
                // For A3C, we push gradients after each episode
                trainA3C(state, trajectory);
                pushWorkerGradients(state);
                synchronizeWorker(state);
            }
            
            // Log progress
            if (state->episodesCompleted % 10 == 0) {
                float avgReward = state->totalReward / state->episodesCompleted;
                std::cout << "Worker " << state->id 
                          << " completed " << state->episodesCompleted << "/" << episodesPerWorker 
                          << " episodes, avg reward: " << avgReward << std::endl;
                
                // Update global policy more frequently for high-performing workers
                if (avgReward > 0 && state->episodesCompleted % 20 == 0) {
                    pushWorkerUpdates(state);
                }
            }
        }
    }
    
    std::cout << "Worker " << state->id << " finished training. "
              << "Episodes: " << state->episodesCompleted
              << ", Steps: " << state->stepsCompleted
              << ", Avg Reward: " << (state->episodesCompleted > 0 ? state->totalReward / state->episodesCompleted : 0.0f)
              << std::endl;
}

void DistributedTrainer::addToSharedBuffer(const Experience& exp) {
    std::lock_guard<std::mutex> lock(bufferMutex);
    
    // Add to circular buffer
    if (sharedBuffer.size() >= sharedBufferSize) {
        // Remove oldest experience
        sharedBuffer.pop_front();
    }
    
    // Add new experience
    sharedBuffer.push_back(exp);
}

std::vector<Experience> DistributedTrainer::sampleFromSharedBuffer(int batchSize) {
    std::lock_guard<std::mutex> lock(bufferMutex);
    
    std::vector<Experience> batch;
    
    // Check if buffer has enough experiences
    if (sharedBuffer.size() < batchSize) {
        return batch;
    }
    
    // Random sampling
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, sharedBuffer.size() - 1);
    
    // Sample experiences
    for (int i = 0; i < batchSize; ++i) {
        int idx = dist(gen);
        batch.push_back(sharedBuffer[idx]);
    }
    
    return batch;
}

void DistributedTrainer::synchronizeWorker(WorkerState* state) {
    // Lock global policy while copying
    std::lock_guard<std::mutex> lock(globalMutex);
    
    // Copy global policy weights to worker policy
    if (state->policy && globalPolicy) {
        state->policy->copyFrom(globalPolicy);
    }
}

void DistributedTrainer::pushWorkerGradients(WorkerState* state) {
    // This is a simplified implementation for A3C
    // In a real implementation, we would:
    // 1. Calculate gradients in the worker policy
    // 2. Lock the global policy
    // 3. Apply gradients to the global policy
    // 4. Unlock the global policy
    
    // For this stub, we'll just copy worker policy to global policy occasionally
    // based on performance
    
    // Simplified implementation
    if (state->episodesCompleted % 10 == 0) {
        pushWorkerUpdates(state);
    }
}

void DistributedTrainer::pushWorkerUpdates(WorkerState* state) {
    // Lock global policy while updating
    std::lock_guard<std::mutex> lock(globalMutex);
    
    // Copy worker policy weights to global policy
    // In a real implementation, we would use a more sophisticated update
    // that accounts for multiple workers updating simultaneously
    if (state->policy && globalPolicy) {
        // The globalPolicy's copyFrom would be implemented to do a proper update
        // e.g., weighted averaging of parameters based on worker performance
        globalPolicy->copyFrom(state->policy.get());
    }
}

void DistributedTrainer::trainPPO(WorkerState* state, const std::vector<Experience>& trajectory) {
    // This is a simplified implementation of PPO training
    // In a real implementation, we would:
    // 1. Calculate advantages and returns
    // 2. Perform multiple epochs of updates
    // 3. Calculate PPO-specific losses
    
    // For this stub, we'll just perform a simple update
    
    // Skip if trajectory is empty
    if (trajectory.empty()) {
        return;
    }
    
    // Extract states, actions, rewards, etc.
    std::vector<std::vector<float>> states;
    std::vector<std::vector<float>> actions;
    std::vector<float> rewards;
    std::vector<float> dones;
    
    for (const auto& exp : trajectory) {
        states.push_back(exp.state);
        actions.push_back(exp.action);
        rewards.push_back(exp.reward);
        dones.push_back(exp.done ? 1.0f : 0.0f);
    }
    
    // Calculate returns
    std::vector<float> returns;
    returns.resize(rewards.size());
    
    // Calculate returns with discounting
    float running_return = 0.0f;
    for (int i = static_cast<int>(rewards.size()) - 1; i >= 0; --i) {
        running_return = rewards[i] + gamma * running_return * (1.0f - dones[i]);
        returns[i] = running_return;
    }
    
    // Simple advantages (returns - baseline)
    std::vector<float> advantages = returns;
    
    // Dummy old log probs
    std::vector<float> oldLogProbs(actions.size(), 0.0f);
    
    // Update policy
    float loss = state->policy->update(states, actions, oldLogProbs, advantages, returns, learningRate);
    
    // Periodically sample from shared buffer for additional training
    if (useExperienceSharing && state->episodesCompleted % 5 == 0) {
        std::vector<Experience> sharedExperiences = sampleFromSharedBuffer(64);
        if (!sharedExperiences.empty()) {
            // Extract data for update
            std::vector<std::vector<float>> sharedStates;
            std::vector<std::vector<float>> sharedActions;
            std::vector<float> sharedReturns;
            
            for (const auto& exp : sharedExperiences) {
                sharedStates.push_back(exp.state);
                sharedActions.push_back(exp.action);
                sharedReturns.push_back(exp.reward); // Simplified, should be actual returns
            }
            
            // Dummy advantages and old log probs
            std::vector<float> sharedAdvantages(sharedReturns);
            std::vector<float> sharedOldLogProbs(sharedActions.size(), 0.0f);
            
            // Update policy with shared experiences
            float sharedLoss = state->policy->update(
                sharedStates, sharedActions, sharedOldLogProbs, 
                sharedAdvantages, sharedReturns, learningRate * 0.5f // Lower learning rate for shared data
            );
        }
    }
}

void DistributedTrainer::trainA3C(WorkerState* state, const std::vector<Experience>& trajectory) {
    // This is a simplified implementation of A3C training
    // In a real implementation, we would:
    // 1. Calculate advantages and returns
    // 2. Calculate policy gradients and value function gradients
    // 3. Apply gradients asynchronously
    
    // For this stub, we'll just perform a simple update similar to PPO
    trainPPO(state, trajectory);
}

// Metal-specific optimizations for distributed training
void DistributedTrainer::optimizeForMetal() {
    #if defined(__APPLE__)
    // Create Metal compute device for parallel processing
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    if (device) {
        // Set up command queue
        id<MTLCommandQueue> commandQueue = [device newCommandQueue];
        
        // Create compute pipelines for parallel training operations
        // Create libraries
        NSError* error = nil;
        id<MTLLibrary> library = [device newDefaultLibraryWithBundle:[NSBundle mainBundle] error:&error];
        
        if (library) {
            // Create function for parallel batch processing
            id<MTLFunction> batchProcessFunction = [library newFunctionWithName:@"process_experience_batch"];
            if (batchProcessFunction) {
                // Create pipeline
                pipelineState = [device newComputePipelineStateWithFunction:batchProcessFunction error:&error];
                
                if (pipelineState) {
                    std::cout << "Successfully created Metal compute pipeline for distributed training" << std::endl;
                    
                    // Create buffers for experience data
                    // These would be used to process batches in parallel on the GPU
                    stateBuffer = [device newBufferWithLength:sizeof(float) * 1024 * 1024 options:MTLResourceStorageModeShared];
                    actionBuffer = [device newBufferWithLength:sizeof(float) * 1024 * 1024 options:MTLResourceStorageModeShared];
                    rewardBuffer = [device newBufferWithLength:sizeof(float) * 1024 * 1024 options:MTLResourceStorageModeShared];
                    
                    // Set Metal-optimized flag
                    metalOptimized = true;
                } else {
                    std::cerr << "Failed to create Metal pipeline state: " << [error.localizedDescription UTF8String] << std::endl;
                }
            } else {
                std::cerr << "Failed to create Metal function" << std::endl;
            }
        } else {
            std::cerr << "Failed to create Metal library: " << [error.localizedDescription UTF8String] << std::endl;
        }
        
        // Clean up
        commandQueue = nil;
    } else {
        std::cerr << "Metal device not available" << std::endl;
    }
    #endif
}

std::string DistributedTrainer::getHardwareInfo() const {
    std::stringstream ss;
    
    #if defined(__APPLE__)
    // Get Metal device information
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    if (device) {
        ss << "Metal Device: " << [[device name] UTF8String] << std::endl;
        ss << "Metal Maximum Buffer Length: " << [device maxBufferLength] << std::endl;
        ss << "Metal Unified Memory: " << ([device hasUnifiedMemory] ? "Yes" : "No") << std::endl;
        ss << "Metal Maximum Threads Per ThreadGroup: " << device.maxThreadsPerThreadgroup.width << std::endl;
        
        // Check for Apple Neural Engine
        bool hasANE = false;
        #if defined(__arm64__)
        // Apple Silicon likely has ANE
        hasANE = true;
        #endif
        
        ss << "Apple Neural Engine: " << (hasANE ? "Available" : "Not available") << std::endl;
        
        device = nil;
    } else {
        ss << "Metal not available" << std::endl;
    }
    #else
    ss << "Metal not supported on this platform" << std::endl;
    #endif
    
    return ss.str();
}

void DistributedTrainer::processExperienceBatchWithMetal(const std::vector<Experience>& batch) {
    #if defined(__APPLE__)
    if (!metalOptimized || !pipelineState || !stateBuffer || !actionBuffer || !rewardBuffer) {
        return;
    }
    
    // Check batch size
    if (batch.empty()) {
        return;
    }
    
    // Prepare data for metal
    std::vector<float> stateData;
    std::vector<float> actionData;
    std::vector<float> rewardData;
    
    // Extract data from batch
    const size_t stateSize = batch[0].state.size();
    const size_t actionSize = batch[0].action.size();
    
    for (const auto& exp : batch) {
        // Add state data
        stateData.insert(stateData.end(), exp.state.begin(), exp.state.end());
        
        // Add action data
        actionData.insert(actionData.end(), exp.action.begin(), exp.action.end());
        
        // Add reward data
        rewardData.push_back(exp.reward);
        rewardData.push_back(exp.done ? 1.0f : 0.0f);
    }
    
    // Copy data to Metal buffers
    memcpy([stateBuffer contents], stateData.data(), stateData.size() * sizeof(float));
    memcpy([actionBuffer contents], actionData.data(), actionData.size() * sizeof(float));
    memcpy([rewardBuffer contents], rewardData.data(), rewardData.size() * sizeof(float));
    
    // Create command buffer and encoder
    id<MTLCommandBuffer> commandBuffer = [metalCommandQueue commandBuffer];
    id<MTLComputeCommandEncoder> encoder = [commandBuffer computeCommandEncoder];
    
    // Set the compute pipeline
    [encoder setComputePipelineState:pipelineState];
    
    // Set the buffers
    [encoder setBuffer:stateBuffer offset:0 atIndex:0];
    [encoder setBuffer:actionBuffer offset:0 atIndex:1];
    [encoder setBuffer:rewardBuffer offset:0 atIndex:2];
    
    // Set the batch parameters as constants
    uint32_t params[3] = {
        static_cast<uint32_t>(batch.size()),
        static_cast<uint32_t>(stateSize),
        static_cast<uint32_t>(actionSize)
    };
    [encoder setBytes:params length:sizeof(params) atIndex:3];
    
    // Calculate thread count
    NSUInteger threadGroupSize = pipelineState.maxTotalThreadsPerThreadgroup;
    if (threadGroupSize > batch.size()) {
        threadGroupSize = batch.size();
    }
    
    // Dispatch threads
    [encoder dispatchThreads:MTLSizeMake(batch.size(), 1, 1)
      threadsPerThreadgroup:MTLSizeMake(threadGroupSize, 1, 1)];
    
    // End encoding
    [encoder endEncoding];
    
    // Commit and wait
    [commandBuffer commit];
    [commandBuffer waitUntilCompleted];
    
    // Process results (would read back any computed values)
    // ...
    #endif
}

} // namespace ai
} // namespace fbneo 