#include "ai_hyperparameter_tuning.h"
#include "ai_rl_algorithms.h"
#include <random>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

// Metal imports
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>

namespace fbneo {
namespace ai {

//------------------------------------------------------------------------------
// Hyperparameter Tuning Implementation
//------------------------------------------------------------------------------

HyperparameterTuner::HyperparameterTuner() 
    : isRunning(false), bestReward(-std::numeric_limits<float>::max()) {
    // Initialize random number generator with seed
    std::random_device rd;
    rng = std::mt19937(rd());
    
    // Initialize Metal resources
    initializeMetalResources();
}

HyperparameterTuner::~HyperparameterTuner() {
    // Ensure tuning is stopped
    stopTuning();
    
    // Clean up Metal resources
    cleanupMetalResources();
}

void HyperparameterTuner::initializeMetalResources() {
    // Get Metal device
    metalDevice = MTLCreateSystemDefaultDevice();
    if (!metalDevice) {
        std::cerr << "Hyperparameter Tuner: Metal is not supported on this device" << std::endl;
        return;
    }
    
    // Create command queue
    metalCommandQueue = [metalDevice newCommandQueue];
    
    // Load Metal compute kernels for hyperparameter tuning
    NSError* error = nil;
    NSString* kernelSource = @R"(
        #include <metal_stdlib>
        using namespace metal;
        
        // Kernel for computing parallel hyperparameter evaluations
        kernel void hyperparameter_evaluation(
            device const float* hyperparameters [[buffer(0)]],
            device const float* rewards [[buffer(1)]],
            device float* scores [[buffer(2)]],
            device const uint* config [[buffer(3)]],
            uint id [[thread_position_in_grid]]
        ) {
            // Get configuration
            uint num_params = config[0];
            uint num_eval_points = config[1];
            
            // Only process within range
            if (id >= num_eval_points) return;
            
            // Calculate base index for this thread's hyperparameters
            uint param_base_idx = id * num_params;
            
            // Calculate score based on reward
            float reward = rewards[id];
            
            // Apply regularization: penalize extreme hyperparameter values
            float regularization = 0.0f;
            for (uint i = 0; i < num_params; ++i) {
                float param = hyperparameters[param_base_idx + i];
                // Penalize values too close to boundaries (0 or 1)
                regularization += 0.01f * (param < 0.1f ? (0.1f - param) : 0.0f);
                regularization += 0.01f * (param > 0.9f ? (param - 0.9f) : 0.0f);
            }
            
            // Final score is reward minus regularization penalty
            scores[id] = reward - regularization;
        }
        
        // Kernel for generating new hyperparameter combinations
        kernel void hyperparameter_generation(
            device const float* best_hyperparameters [[buffer(0)]],
            device float* new_hyperparameters [[buffer(1)]],
            device const uint* config [[buffer(2)]],
            device const uint* seeds [[buffer(3)]],
            uint id [[thread_position_in_grid]]
        ) {
            // Get configuration
            uint num_params = config[0];
            uint num_points = config[1];
            uint generation = config[2];
            float exploration_rate = as_type<float>(config[3]); // Reinterpret as float
            
            // Only process within range
            if (id >= num_points) return;
            
            // Calculate base indices
            uint param_base_idx = id * num_params;
            
            // Simple hash-based RNG
            uint seed = seeds[id] + generation;
            seed = (seed ^ 61) ^ (seed >> 16);
            seed *= 9;
            seed = seed ^ (seed >> 4);
            seed *= 0x27d4eb2d;
            seed = seed ^ (seed >> 15);
            
            // Generate new hyperparameters
            for (uint i = 0; i < num_params; ++i) {
                // Get current parameter
                float base_param = best_hyperparameters[i];
                
                // Generate random perturbation
                uint param_seed = seed + i * 1000;
                param_seed = (param_seed ^ 61) ^ (param_seed >> 16);
                param_seed *= 9;
                param_seed = param_seed ^ (param_seed >> 4);
                param_seed *= 0x27d4eb2d;
                param_seed = param_seed ^ (param_seed >> 15);
                
                // Convert to float in [0,1]
                float rand_val = float(param_seed % 10000) / 10000.0f;
                
                // Perturb value based on exploration rate
                float new_param;
                
                // Apply different strategies based on thread ID
                if (id % 4 == 0) {
                    // Strategy 1: Small perturbation around best value
                    new_param = base_param + (2.0f * rand_val - 1.0f) * 0.1f * exploration_rate;
                } else if (id % 4 == 1) {
                    // Strategy 2: Larger perturbation
                    new_param = base_param + (2.0f * rand_val - 1.0f) * 0.3f * exploration_rate;
                } else if (id % 4 == 2) {
                    // Strategy 3: Random value in full range
                    new_param = rand_val;
                } else {
                    // Strategy 4: Crossover with previous best
                    float prev_best = best_hyperparameters[i + num_params];
                    new_param = rand_val < 0.5f ? base_param : prev_best;
                    new_param += (2.0f * rand_val - 1.0f) * 0.05f * exploration_rate;
                }
                
                // Clamp to valid range [0,1]
                new_hyperparameters[param_base_idx + i] = clamp(new_param, 0.0f, 1.0f);
            }
        }
    )";
    
    // Create Metal library
    metalLibrary = [metalDevice newLibraryWithSource:kernelSource options:nil error:&error];
    if (!metalLibrary) {
        std::cerr << "Hyperparameter Tuner: Failed to create Metal library: " 
                  << [error.localizedDescription UTF8String] << std::endl;
        return;
    }
    
    // Create compute pipelines
    id<MTLFunction> evalFunction = [metalLibrary newFunctionWithName:@"hyperparameter_evaluation"];
    id<MTLFunction> genFunction = [metalLibrary newFunctionWithName:@"hyperparameter_generation"];
    
    if (!evalFunction || !genFunction) {
        std::cerr << "Hyperparameter Tuner: Failed to create Metal compute functions" << std::endl;
        return;
    }
    
    evalPipeline = [metalDevice newComputePipelineStateWithFunction:evalFunction error:&error];
    genPipeline = [metalDevice newComputePipelineStateWithFunction:genFunction error:&error];
    
    if (!evalPipeline || !genPipeline) {
        std::cerr << "Hyperparameter Tuner: Failed to create Metal compute pipelines: " 
                  << [error.localizedDescription UTF8String] << std::endl;
        return;
    }
    
    std::cout << "Hyperparameter Tuner: Successfully initialized Metal compute resources" << std::endl;
}

void HyperparameterTuner::cleanupMetalResources() {
    // Release Metal resources
    metalLibrary = nil;
    evalPipeline = nil;
    genPipeline = nil;
    hyperparametersBuffer = nil;
    rewardsBuffer = nil;
    scoresBuffer = nil;
    configBuffer = nil;
    seedsBuffer = nil;
    metalCommandQueue = nil;
    metalDevice = nil;
}

void HyperparameterTuner::addHyperparameter(const std::string& name, float minValue, float maxValue) {
    HyperparameterDefinition param;
    param.name = name;
    param.minValue = minValue;
    param.maxValue = maxValue;
    
    // Lock to prevent concurrent modification
    std::lock_guard<std::mutex> lock(parametersMutex);
    hyperparameters.push_back(param);
}

void HyperparameterTuner::setEvaluationFunction(EvaluationFunction evalFunc) {
    evaluationFunction = evalFunc;
}

void HyperparameterTuner::startTuning(int numGenerations, int populationSize) {
    if (isRunning) {
        std::cerr << "Hyperparameter tuning is already running" << std::endl;
        return;
    }
    
    if (hyperparameters.empty()) {
        std::cerr << "No hyperparameters defined" << std::endl;
        return;
    }
    
    if (!evaluationFunction) {
        std::cerr << "No evaluation function set" << std::endl;
        return;
    }
    
    isRunning = true;
    
    // Initialize buffers for Metal acceleration
    initializeMetalBuffers(populationSize);
    
    // Start tuning thread
    tuningThread = std::thread(&HyperparameterTuner::tuningLoop, this, numGenerations, populationSize);
}

void HyperparameterTuner::stopTuning() {
    if (!isRunning) return;
    
    isRunning = false;
    
    if (tuningThread.joinable()) {
        tuningThread.join();
    }
}

void HyperparameterTuner::initializeMetalBuffers(int populationSize) {
    // Only initialize if Metal is available
    if (!metalDevice || !metalCommandQueue) return;
    
    // Get the number of hyperparameters
    std::lock_guard<std::mutex> lock(parametersMutex);
    int numParams = hyperparameters.size();
    
    // Create or resize buffers
    // Hyperparameters buffer: stores all parameter combinations for the population
    hyperparametersBuffer = [metalDevice newBufferWithLength:populationSize * numParams * sizeof(float)
                                                    options:MTLResourceStorageModeShared];
    
    // Best hyperparameters buffer: stores the best and second best parameters
    bestHyperparametersBuffer = [metalDevice newBufferWithLength:2 * numParams * sizeof(float)
                                                        options:MTLResourceStorageModeShared];
    
    // Rewards buffer: stores the rewards for each parameter combination
    rewardsBuffer = [metalDevice newBufferWithLength:populationSize * sizeof(float)
                                            options:MTLResourceStorageModeShared];
    
    // Scores buffer: stores the scores (reward with regularization) for each combination
    scoresBuffer = [metalDevice newBufferWithLength:populationSize * sizeof(float)
                                           options:MTLResourceStorageModeShared];
    
    // Config buffer: stores configuration parameters
    configBuffer = [metalDevice newBufferWithLength:4 * sizeof(uint32_t)
                                           options:MTLResourceStorageModeShared];
    
    // Seeds buffer: stores random seeds for each parameter combination
    seedsBuffer = [metalDevice newBufferWithLength:populationSize * sizeof(uint32_t)
                                          options:MTLResourceStorageModeShared];
    
    // Initialize config
    uint32_t config[4] = {
        static_cast<uint32_t>(numParams),
        static_cast<uint32_t>(populationSize),
        0, // Generation (will be updated in each loop)
        *reinterpret_cast<uint32_t*>(&initialExplorationRate) // Exploration rate as float
    };
    memcpy([configBuffer contents], config, sizeof(config));
    
    // Initialize seeds
    std::vector<uint32_t> seeds(populationSize);
    std::generate(seeds.begin(), seeds.end(), [&]() { return static_cast<uint32_t>(rng()); });
    memcpy([seedsBuffer contents], seeds.data(), seeds.size() * sizeof(uint32_t));
    
    // Initialize hyperparameters with random values
    float* hyperparamData = static_cast<float*>([hyperparametersBuffer contents]);
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    
    for (int i = 0; i < populationSize; ++i) {
        for (int j = 0; j < numParams; ++j) {
            hyperparamData[i * numParams + j] = dist(rng);
        }
    }
    
    // Initialize best hyperparameters
    float* bestHyperparamData = static_cast<float*>([bestHyperparametersBuffer contents]);
    for (int j = 0; j < numParams; ++j) {
        // Start with random values in each slot of the best params
        bestHyperparamData[j] = dist(rng);
        bestHyperparamData[numParams + j] = dist(rng);
    }
}

void HyperparameterTuner::tuningLoop(int numGenerations, int populationSize) {
    std::cout << "Starting hyperparameter tuning with " << numGenerations 
              << " generations and population size " << populationSize << std::endl;
    
    // Initialize best values
    bestReward = -std::numeric_limits<float>::max();
    bestHyperparameterValues.clear();
    
    // Lock parameters to ensure they don't change during tuning
    std::lock_guard<std::mutex> lock(parametersMutex);
    int numParams = hyperparameters.size();
    
    // Generate initial random population if not using Metal
    if (!metalDevice || !metalCommandQueue) {
        generateRandomPopulation(populationSize);
    }
    
    for (int generation = 0; generation < numGenerations && isRunning; ++generation) {
        float explorationRate = initialExplorationRate * (1.0f - static_cast<float>(generation) / numGenerations);
        
        // Update config with current generation and exploration rate
        if (metalDevice && configBuffer) {
            uint32_t* config = static_cast<uint32_t*>([configBuffer contents]);
            config[2] = generation;
            config[3] = *reinterpret_cast<uint32_t*>(&explorationRate);
        }
        
        // If using Metal, generate new parameter combinations
        if (metalDevice && metalCommandQueue && genPipeline && 
            hyperparametersBuffer && bestHyperparametersBuffer && generation > 0) {
            generatePopulationWithMetal(populationSize);
        }
        
        // Evaluate population
        auto startTime = std::chrono::high_resolution_clock::now();
        
        std::vector<float> rewards;
        if (metalDevice && rewardsBuffer) {
            rewards.resize(populationSize);
        }
        
        // Evaluate each parameter combination
        float generationBestReward = -std::numeric_limits<float>::max();
        std::unordered_map<std::string, float> generationBestParams;
        
        for (int i = 0; i < populationSize && isRunning; ++i) {
            // Convert normalized parameters to actual values
            std::unordered_map<std::string, float> params;
            
            if (metalDevice && hyperparametersBuffer) {
                // Extract from Metal buffer
                float* hyperparamData = static_cast<float*>([hyperparametersBuffer contents]);
                for (int j = 0; j < numParams; ++j) {
                    float normalizedValue = hyperparamData[i * numParams + j];
                    float actualValue = hyperparameters[j].minValue + 
                        normalizedValue * (hyperparameters[j].maxValue - hyperparameters[j].minValue);
                    params[hyperparameters[j].name] = actualValue;
                }
            } else {
                // Use CPU population
                for (int j = 0; j < numParams; ++j) {
                    float normalizedValue = population[i][j];
                    float actualValue = hyperparameters[j].minValue + 
                        normalizedValue * (hyperparameters[j].maxValue - hyperparameters[j].minValue);
                    params[hyperparameters[j].name] = actualValue;
                }
            }
            
            // Evaluate parameters
            float reward = evaluationFunction(params);
            
            // Store reward in Metal buffer if available
            if (metalDevice && rewardsBuffer) {
                rewards[i] = reward;
            }
            
            // Track best in this generation
            if (reward > generationBestReward) {
                generationBestReward = reward;
                generationBestParams = params;
                
                // If this is also the global best, update that
                if (reward > bestReward) {
                    bestReward = reward;
                    bestHyperparameterValues = params;
                    
                    // Update best hyperparameters in Metal buffer
                    if (metalDevice && bestHyperparametersBuffer) {
                        // Shift current best to second position
                        float* bestData = static_cast<float*>([bestHyperparametersBuffer contents]);
                        for (int j = 0; j < numParams; ++j) {
                            bestData[numParams + j] = bestData[j];
                        }
                        
                        // Set new best in first position
                        for (int j = 0; j < numParams; ++j) {
                            float normalizedValue = (params[hyperparameters[j].name] - hyperparameters[j].minValue) / 
                                (hyperparameters[j].maxValue - hyperparameters[j].minValue);
                            bestData[j] = normalizedValue;
                        }
                    }
                }
            }
        }
        
        // If using Metal, copy rewards to buffer and compute scores
        if (metalDevice && metalCommandQueue && evalPipeline && 
            rewardsBuffer && scoresBuffer && isRunning) {
            // Copy rewards
            memcpy([rewardsBuffer contents], rewards.data(), rewards.size() * sizeof(float));
            
            // Compute scores
            id<MTLCommandBuffer> commandBuffer = [metalCommandQueue commandBuffer];
            id<MTLComputeCommandEncoder> encoder = [commandBuffer computeCommandEncoder];
            
            [encoder setComputePipelineState:evalPipeline];
            [encoder setBuffer:hyperparametersBuffer offset:0 atIndex:0];
            [encoder setBuffer:rewardsBuffer offset:0 atIndex:1];
            [encoder setBuffer:scoresBuffer offset:0 atIndex:2];
            [encoder setBuffer:configBuffer offset:0 atIndex:3];
            
            NSUInteger threadGroupSize = evalPipeline.maxTotalThreadsPerThreadgroup;
            if (threadGroupSize > populationSize) threadGroupSize = populationSize;
            
            [encoder dispatchThreads:MTLSizeMake(populationSize, 1, 1)
              threadsPerThreadgroup:MTLSizeMake(threadGroupSize, 1, 1)];
            
            [encoder endEncoding];
            [commandBuffer commit];
            [commandBuffer waitUntilCompleted];
        }
        
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
        
        // Log progress
        std::cout << "Generation " << generation + 1 << "/" << numGenerations 
                  << ", Best reward: " << generationBestReward
                  << ", Global best: " << bestReward
                  << ", Time: " << duration << "ms" << std::endl;
        
        // Log best parameters for this generation
        std::cout << "Best parameters this generation:" << std::endl;
        for (const auto& param : generationBestParams) {
            std::cout << "  " << param.first << ": " << param.second << std::endl;
        }
    }
    
    std::cout << "Hyperparameter tuning completed." << std::endl;
    std::cout << "Best hyperparameters found (reward: " << bestReward << "):" << std::endl;
    
    for (const auto& param : bestHyperparameterValues) {
        std::cout << "  " << param.first << ": " << param.second << std::endl;
    }
    
    isRunning = false;
}

void HyperparameterTuner::generateRandomPopulation(int populationSize) {
    // Clear existing population
    population.clear();
    population.resize(populationSize);
    
    // Lock parameters
    std::lock_guard<std::mutex> lock(parametersMutex);
    int numParams = hyperparameters.size();
    
    // Generate random parameter combinations
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    
    for (int i = 0; i < populationSize; ++i) {
        population[i].resize(numParams);
        for (int j = 0; j < numParams; ++j) {
            // Generate random normalized value [0,1]
            population[i][j] = dist(rng);
        }
    }
}

void HyperparameterTuner::generatePopulationWithMetal(int populationSize) {
    // Skip if Metal not available
    if (!metalDevice || !metalCommandQueue || !genPipeline) return;
    
    // Create command buffer
    id<MTLCommandBuffer> commandBuffer = [metalCommandQueue commandBuffer];
    id<MTLComputeCommandEncoder> encoder = [commandBuffer computeCommandEncoder];
    
    // Set the compute pipeline
    [encoder setComputePipelineState:genPipeline];
    
    // Set the buffers
    [encoder setBuffer:bestHyperparametersBuffer offset:0 atIndex:0];
    [encoder setBuffer:hyperparametersBuffer offset:0 atIndex:1];
    [encoder setBuffer:configBuffer offset:0 atIndex:2];
    [encoder setBuffer:seedsBuffer offset:0 atIndex:3];
    
    // Calculate thread groups
    NSUInteger threadGroupSize = genPipeline.maxTotalThreadsPerThreadgroup;
    if (threadGroupSize > populationSize) threadGroupSize = populationSize;
    
    // Dispatch threads
    [encoder dispatchThreads:MTLSizeMake(populationSize, 1, 1)
      threadsPerThreadgroup:MTLSizeMake(threadGroupSize, 1, 1)];
    
    // End encoding and commit
    [encoder endEncoding];
    [commandBuffer commit];
    [commandBuffer waitUntilCompleted];
}

std::unordered_map<std::string, float> HyperparameterTuner::getBestHyperparameters() const {
    return bestHyperparameterValues;
}

float HyperparameterTuner::getBestReward() const {
    return bestReward;
}

bool HyperparameterTuner::saveToFile(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for writing: " << filename << std::endl;
        return false;
    }
    
    // Write header
    file << "# Best hyperparameters found (reward: " << bestReward << ")" << std::endl;
    
    // Write parameters
    for (const auto& param : bestHyperparameterValues) {
        file << param.first << " = " << std::setprecision(8) << param.second << std::endl;
    }
    
    file.close();
    return true;
}

bool HyperparameterTuner::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for reading: " << filename << std::endl;
        return false;
    }
    
    bestHyperparameterValues.clear();
    
    std::string line;
    while (std::getline(file, line)) {
        // Skip comments and empty lines
        if (line.empty() || line[0] == '#') continue;
        
        // Parse parameter
        size_t equalsPos = line.find('=');
        if (equalsPos == std::string::npos) continue;
        
        std::string name = line.substr(0, equalsPos);
        std::string valueStr = line.substr(equalsPos + 1);
        
        // Trim whitespace
        name.erase(0, name.find_first_not_of(" \t"));
        name.erase(name.find_last_not_of(" \t") + 1);
        valueStr.erase(0, valueStr.find_first_not_of(" \t"));
        valueStr.erase(valueStr.find_last_not_of(" \t") + 1);
        
        // Parse value
        try {
            float value = std::stof(valueStr);
            bestHyperparameterValues[name] = value;
        } catch (const std::exception& e) {
            std::cerr << "Error parsing value for parameter '" << name << "': " << e.what() << std::endl;
            continue;
        }
    }
    
    file.close();
    return !bestHyperparameterValues.empty();
}

} // namespace ai
} // namespace fbneo 