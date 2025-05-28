#include "ai_reinforcement_learning.h"
#include <algorithm>
#include <numeric>
#include <cmath>
#include <fstream>
#include <iostream>
#include <random>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>

// For JSON serialization if nlohmann/json is available
#ifdef USE_NLOHMANN_JSON
#include <nlohmann/json.hpp>
using json = nlohmann::json;
#endif

namespace AI {

AIReinforcementLearning::AIReinforcementLearning(
    std::shared_ptr<AITorchPolicyModel> policyModel,
    size_t bufferSize,
    size_t batchSize,
    float gamma,
    float learningRate)
    : m_policyModel(policyModel),
      m_bufferSize(bufferSize),
      m_batchSize(batchSize),
      m_gamma(gamma),
      m_learningRate(learningRate),
      m_clipRatio(0.2f) {
    // Initialize random generator with seed
    std::random_device rd;
    m_rng = std::mt19937(rd());
    
    // Reserve space for experience buffer
    m_experienceBuffer.reserve(m_bufferSize);
}

AIReinforcementLearning::~AIReinforcementLearning() {
    // Cleanup if needed
}

void AIReinforcementLearning::addExperience(
    const AIInputFrame& state,
    const AIOutputAction& action,
    float reward,
    const AIInputFrame& nextState,
    bool done) {
    
    // Add experience to buffer
    m_experienceBuffer.emplace_back(state, action, reward, nextState, done);
    
    // If buffer exceeds max size, remove oldest entries
    if (m_experienceBuffer.size() > m_bufferSize) {
        m_experienceBuffer.pop_front();
    }
}

float AIReinforcementLearning::train(int epochs) {
    if (!m_policyModel || !m_policyModel->isModelLoaded()) {
        std::cerr << "Cannot train: Policy model not loaded" << std::endl;
        return -1.0f;
    }
    
    if (m_experienceBuffer.size() < m_batchSize) {
        std::cerr << "Cannot train: Not enough experiences (" 
                  << m_experienceBuffer.size() << " < " << m_batchSize << ")" << std::endl;
        return -1.0f;
    }
    
    float totalLoss = 0.0f;
    
    for (int epoch = 0; epoch < epochs; ++epoch) {
        // Sample batch of experiences
        std::vector<Experience> batch = sampleBatch();
        
        // Calculate advantages
        std::vector<float> advantages;
        float gae = calculateGAE(batch, advantages);
        
        // Calculate PPO loss
        float loss = calculatePPOLoss(batch, advantages);
        totalLoss += loss;
        
        // Update policy model
        for (const auto& exp : batch) {
            m_policyModel->update(exp.state, exp.action, exp.reward, exp.nextState, exp.done);
        }
    }
    
    return totalLoss / epochs;
}

std::shared_ptr<AITorchPolicyModel> AIReinforcementLearning::getPolicyModel() const {
    return m_policyModel;
}

bool AIReinforcementLearning::saveModel(const std::string& path) const {
    if (!m_policyModel) {
        std::cerr << "Cannot save: Policy model not initialized" << std::endl;
        return false;
    }
    
    try {
        m_policyModel->saveModel(path);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error saving model: " << e.what() << std::endl;
        return false;
    }
}

bool AIReinforcementLearning::loadModel(const std::string& path) {
    if (!m_policyModel) {
        std::cerr << "Cannot load: Policy model not initialized" << std::endl;
        return false;
    }
    
    try {
        m_policyModel->loadModel(path);
        return m_policyModel->isModelLoaded();
    } catch (const std::exception& e) {
        std::cerr << "Error loading model: " << e.what() << std::endl;
        return false;
    }
}

void AIReinforcementLearning::clearExperiences() {
    m_experienceBuffer.clear();
}

size_t AIReinforcementLearning::getExperienceCount() const {
    return m_experienceBuffer.size();
}

float AIReinforcementLearning::getClipRatio() const {
    return m_clipRatio;
}

void AIReinforcementLearning::setClipRatio(float ratio) {
    if (ratio > 0.0f && ratio < 1.0f) {
        m_clipRatio = ratio;
    }
}

bool AIReinforcementLearning::exportExperiencesToJson(const std::string& filename) const {
#ifdef USE_NLOHMANN_JSON
    try {
        json experiencesJson = json::array();
        
        for (const auto& exp : m_experienceBuffer) {
            json expJson;
            
            // Convert state to JSON
            std::string stateJson = exp.state.toJson();
            expJson["state"] = json::parse(stateJson);
            
            // Convert action to JSON
            std::string actionJson = exp.action.toJson();
            expJson["action"] = json::parse(actionJson);
            
            // Add reward and done flag
            expJson["reward"] = exp.reward;
            expJson["done"] = exp.done;
            
            // Convert next state to JSON
            std::string nextStateJson = exp.nextState.toJson();
            expJson["nextState"] = json::parse(nextStateJson);
            
            experiencesJson.push_back(expJson);
        }
        
        // Write to file
        std::ofstream outFile(filename);
        if (!outFile.is_open()) {
            std::cerr << "Failed to open file for writing: " << filename << std::endl;
            return false;
        }
        
        outFile << experiencesJson.dump(4);
        outFile.close();
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error exporting experiences to JSON: " << e.what() << std::endl;
        return false;
    }
#else
    std::cerr << "JSON serialization not available. Compile with USE_NLOHMANN_JSON defined." << std::endl;
    return false;
#endif
}

int AIReinforcementLearning::importExperiencesFromJson(const std::string& filename) {
#ifdef USE_NLOHMANN_JSON
    try {
        // Open file
        std::ifstream inFile(filename);
        if (!inFile.is_open()) {
            std::cerr << "Failed to open file for reading: " << filename << std::endl;
            return 0;
        }
        
        // Parse JSON
        json experiencesJson;
        inFile >> experiencesJson;
        inFile.close();
        
        if (!experiencesJson.is_array()) {
            std::cerr << "Invalid JSON format: expected array" << std::endl;
            return 0;
        }
        
        int imported = 0;
        
        for (const auto& expJson : experiencesJson) {
            try {
                // Parse state
                AIInputFrame state;
                state.fromJson(expJson["state"].dump());
                
                // Parse action
                AIOutputAction action;
                action.fromJson(expJson["action"].dump());
                
                // Get reward and done flag
                float reward = expJson["reward"].get<float>();
                bool done = expJson["done"].get<bool>();
                
                // Parse next state
                AIInputFrame nextState;
                nextState.fromJson(expJson["nextState"].dump());
                
                // Add experience
                addExperience(state, action, reward, nextState, done);
                imported++;
            } catch (const std::exception& e) {
                std::cerr << "Error parsing experience: " << e.what() << std::endl;
                continue;
            }
        }
        
        return imported;
    } catch (const std::exception& e) {
        std::cerr << "Error importing experiences from JSON: " << e.what() << std::endl;
        return 0;
    }
#else
    std::cerr << "JSON serialization not available. Compile with USE_NLOHMANN_JSON defined." << std::endl;
    return 0;
#endif
}

// Private Methods

std::vector<Experience> AIReinforcementLearning::sampleBatch() {
    std::vector<Experience> batch;
    
    // Ensure we have enough experiences
    if (m_experienceBuffer.size() <= m_batchSize) {
        // If we don't have enough, return all experiences
        batch.insert(batch.end(), m_experienceBuffer.begin(), m_experienceBuffer.end());
        return batch;
    }
    
    // Sample without replacement
    std::vector<size_t> indices(m_experienceBuffer.size());
    std::iota(indices.begin(), indices.end(), 0);
    std::shuffle(indices.begin(), indices.end(), m_rng);
    
    // Take the first m_batchSize elements
    batch.reserve(m_batchSize);
    for (size_t i = 0; i < m_batchSize; ++i) {
        batch.push_back(m_experienceBuffer[indices[i]]);
    }
    
    return batch;
}

float AIReinforcementLearning::calculateGAE(const std::vector<Experience>& batch, std::vector<float>& advantages) {
    // Simple advantage calculation for now
    // In a real implementation, we would calculate GAE (Generalized Advantage Estimation)
    
    advantages.clear();
    advantages.reserve(batch.size());
    
    float totalAdvantage = 0.0f;
    
    for (const auto& exp : batch) {
        // Simple advantage: reward + gamma * nextValue - currentValue
        // For simplicity, assuming value is 0.0f for now
        float advantage = exp.reward;
        if (!exp.done) {
            advantage += m_gamma * 0.0f; // Placeholder for next state value
        }
        
        advantages.push_back(advantage);
        totalAdvantage += advantage;
    }
    
    // Normalize advantages
    if (!advantages.empty()) {
        float mean = totalAdvantage / advantages.size();
        float variance = 0.0f;
        
        for (float adv : advantages) {
            variance += (adv - mean) * (adv - mean);
        }
        
        variance /= advantages.size();
        float stdDev = std::sqrt(variance) + 1e-8f; // Add small epsilon for numerical stability
        
        for (float& adv : advantages) {
            adv = (adv - mean) / stdDev;
        }
    }
    
    return totalAdvantage;
}

float AIReinforcementLearning::calculatePPOLoss(const std::vector<Experience>& batch, const std::vector<float>& advantages) {
    // Implement PPO loss calculation with clipping and value function
    
    float policyLoss = 0.0f;
    float valueLoss = 0.0f;
    float entropyLoss = 0.0f;
    
    // Process each experience
    for (size_t i = 0; i < batch.size(); ++i) {
        const auto& exp = batch[i];
        float advantage = advantages[i];
        
        // Get current policy and value predictions
        float currentLogProb = 0.0f;
        float currentValue = 0.0f;
        
        if (m_policyModel && m_policyModel->isModelLoaded()) {
            // Get log probability and value from policy model
            currentLogProb = m_policyModel->computeLogProb(exp.state, exp.action);
            currentValue = m_policyModel->computeValue(exp.state);
        } else {
            // Fallback to stored values if model not available
            currentLogProb = exp.action.getLogProb();
            currentValue = exp.value;
        }
        
        // Get old log probability from saved experience
        float oldLogProb = exp.action.getLogProb();
        
        // Calculate probability ratio (new / old)
        float ratio = std::exp(currentLogProb - oldLogProb);
        
        // Calculate surrogate objectives for PPO clipping
        float surr1 = ratio * advantage;
        float surr2 = std::clamp(ratio, 1.0f - m_clipRatio, 1.0f + m_clipRatio) * advantage;
        
        // Use minimum for pessimistic bound (PPO clipping)
        float policySurrogate = -std::min(surr1, surr2);
        
        // Calculate value function loss (MSE)
        float valueTarget = advantage + exp.value; // Advantage + old value = return
        float valueMSE = (currentValue - valueTarget) * (currentValue - valueTarget);
        
        // Calculate entropy for exploration
        float entropy = -currentLogProb * 0.01f; // Simple approximate entropy
        
        // Accumulate losses
        policyLoss += policySurrogate;
        valueLoss += valueMSE;
        entropyLoss += entropy;
    }
    
    // Calculate average losses
    float avgPolicyLoss = policyLoss / batch.size();
    float avgValueLoss = valueLoss / batch.size();
    float avgEntropyLoss = entropyLoss / batch.size();
    
    // Calculate weighted combined loss
    // Typical weights: value coefficient = 0.5-1.0, entropy coefficient = 0.001-0.01
    float totalLoss = avgPolicyLoss + 0.5f * avgValueLoss - 0.01f * avgEntropyLoss;
    
    return totalLoss;
}

// Implementation of Proximal Policy Optimization (PPO) training
float AIReinforcementLearning::TrainPPO(const std::vector<Experience>& batch, float clipEpsilon, float learningRate) {
    if (!m_policyModel || !m_policyModel->isModelLoaded()) {
        std::cerr << "Cannot train PPO: Policy model not loaded" << std::endl;
        return -1.0f;
    }
    
    if (batch.empty()) {
        std::cerr << "Cannot train PPO: Empty batch" << std::endl;
        return -1.0f;
    }
    
    // Store original parameters to restore later
    float originalLearningRate = m_learningRate;
    float originalClipRatio = m_clipRatio;
    
    // Use provided parameters if specified
    if (learningRate > 0.0f) {
        m_learningRate = learningRate;
    }
    if (clipEpsilon > 0.0f) {
        m_clipRatio = clipEpsilon;
    }
    
    // Make a copy of the batch that we can modify
    std::vector<Experience> trajectory = batch;
    
    // Compute advantages and returns using GAE
    std::vector<float> advantages;
    float totalAdvantage = calculateGAE(trajectory, advantages);
    
    // Total losses over multiple epochs
    float totalPolicyLoss = 0.0f;
    float totalValueLoss = 0.0f;
    float totalEntropyLoss = 0.0f;
    int epochCount = 4; // Standard PPO uses multiple epochs
    
    // Run multiple epochs of PPO training
    for (int epoch = 0; epoch < epochCount; ++epoch) {
        // Shuffle the data for each epoch
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(trajectory.begin(), trajectory.end(), g);
        
        // Process in mini-batches
        size_t batchSize = std::min(trajectory.size(), (size_t)64); // Typical mini-batch size
        
        for (size_t i = 0; i < trajectory.size(); i += batchSize) {
            // Get mini-batch
            size_t end = std::min(i + batchSize, trajectory.size());
            std::vector<Experience> miniBatch(trajectory.begin() + i, trajectory.begin() + end);
            
            // Extract corresponding advantages
            std::vector<float> miniBatchAdvantages;
            miniBatchAdvantages.reserve(end - i);
            for (size_t j = i; j < end; ++j) {
                miniBatchAdvantages.push_back(advantages[j]);
            }
            
            // Calculate PPO loss for this mini-batch
            float batchLoss = calculatePPOLoss(miniBatch, miniBatchAdvantages);
            
            // In a real implementation, we would apply the gradients to update the model
            // m_policyModel->applyGradients(gradients, m_learningRate);
            
            // Accumulate losses for tracking
            // In a real implementation, we would extract these from the calculated loss
            totalPolicyLoss += batchLoss;
        }
    }
    
    // Calculate average loss
    float averageLoss = totalPolicyLoss / (trajectory.size() * epochCount);
    
    // Log info
    std::cout << "PPO Training - Average Loss: " << averageLoss 
              << ", Clip Ratio: " << m_clipRatio
              << ", Learning Rate: " << m_learningRate << std::endl;
    
    // Restore original parameters
    m_learningRate = originalLearningRate;
    m_clipRatio = originalClipRatio;
    
    return averageLoss;
}

// Implement A3C training method
float AIReinforcementLearning::TrainA3C(const std::vector<Experience>& batch, int numWorkers, float learningRate) {
    if (!m_policyModel || !m_policyModel->isModelLoaded()) {
        std::cerr << "Cannot train: Policy model not loaded" << std::endl;
        return -1.0f;
    }
    
    if (batch.empty()) {
        std::cerr << "Cannot train: Empty batch" << std::endl;
        return -1.0f;
    }
    
    // Store original learning rate to restore later
    float originalLearningRate = m_learningRate;
    
    // Use provided learning rate if specified
    if (learningRate > 0.0f) {
        m_learningRate = learningRate;
    }
    
    // Set up worker threads for A3C
    std::vector<std::thread> workers;
    std::vector<float> workerLosses(numWorkers, 0.0f);
    std::mutex globalModelMutex;
    std::atomic<int> completedWorkers(0);
    
    // Shared batch for training
    const std::vector<Experience>& globalBatch = batch;
    
    // Create and start worker threads
    for (int i = 0; i < numWorkers; ++i) {
        workers.emplace_back([&, i]() {
            // Calculate which portion of the batch this worker should process
            size_t batchSize = globalBatch.size();
            size_t itemsPerWorker = batchSize / numWorkers;
            size_t start = i * itemsPerWorker;
            size_t end = (i == numWorkers - 1) ? batchSize : (i + 1) * itemsPerWorker;
            
            // Create a local batch for this worker
            std::vector<Experience> localBatch(globalBatch.begin() + start, globalBatch.begin() + end);
            
            // Clone global model for this worker
            std::shared_ptr<AITorchPolicyModel> workerModel;
            {
                std::lock_guard<std::mutex> lock(globalModelMutex);
                // In real implementation, clone the model:
                // workerModel = m_policyModel->clone();
                workerModel = m_policyModel; // Use shared model for simplicity in stub
            }
            
            // Compute advantages and returns for this worker's batch
            std::vector<float> advantages;
            std::vector<float> returns;
            advantages.reserve(localBatch.size());
            returns.reserve(localBatch.size());
            
            // Last value is 0 if done, otherwise bootstrap from the value function
            float nextValue = 0.0f;
            if (!localBatch.back().done) {
                // Get value estimate from the model
                nextValue = workerModel->computeValue(localBatch.back().nextState);
            }
            
            // Compute returns and advantages in reverse order
            for (int j = localBatch.size() - 1; j >= 0; --j) {
                // Extract information from current experience
                float reward = localBatch[j].reward;
                float value = localBatch[j].value;
                bool done = localBatch[j].done;
                
                // Compute the return (discounted sum of rewards)
                float ret = reward + m_gamma * nextValue * (1.0f - (done ? 1.0f : 0.0f));
                returns.push_back(ret);
                
                // Compute the advantage
                float advantage = ret - value;
                advantages.push_back(advantage);
                
                // Update nextValue for next iteration
                nextValue = ret;
            }
            
            // Reverse the vectors since we computed them in reverse order
            std::reverse(advantages.begin(), advantages.end());
            std::reverse(returns.begin(), returns.end());
            
            // Calculate total losses for this worker
            float totalPolicyLoss = 0.0f;
            float totalValueLoss = 0.0f;
            float totalEntropyLoss = 0.0f;
            
            // Process each experience in the local batch
            for (size_t j = 0; j < localBatch.size(); ++j) {
                const auto& exp = localBatch[j];
                float advantage = advantages[j];
                float returnValue = returns[j];
                
                // Compute policy gradient updates
                float logProb = workerModel->computeLogProb(exp.state, exp.action);
                float value = workerModel->computeValue(exp.state);
                
                // Compute losses
                float policyLoss = -logProb * advantage; // Policy gradient
                float valueLoss = 0.5f * (value - returnValue) * (value - returnValue); // MSE
                float entropy = -logProb * 0.01f; // Simple entropy approximation
                
                // Accumulate losses
                totalPolicyLoss += policyLoss;
                totalValueLoss += valueLoss;
                totalEntropyLoss += entropy;
                
                // In a real implementation, we would apply local updates to the worker model
                // workerModel->applyLocalUpdates(exp, advantage, returnValue);
            }
            
            // Calculate average losses
            float avgPolicyLoss = totalPolicyLoss / localBatch.size();
            float avgValueLoss = totalValueLoss / localBatch.size();
            float avgEntropyLoss = totalEntropyLoss / localBatch.size();
            
            // Calculate combined loss
            float totalLoss = avgPolicyLoss + 0.5f * avgValueLoss - 0.01f * avgEntropyLoss;
            
            // Store total loss for this worker
            workerLosses[i] = totalLoss;
            
            // Apply accumulated gradients to global model
            {
                std::lock_guard<std::mutex> lock(globalModelMutex);
                // In a real implementation, apply gradients from worker to global model
                // m_policyModel->applyGradients(workerModel->getGradients(), m_learningRate);
            }
            
            // Mark this worker as completed
            completedWorkers++;
        });
    }
    
    // Wait for all workers to complete
    for (auto& worker : workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    
    // Calculate average loss across all workers
    float totalLoss = 0.0f;
    for (float loss : workerLosses) {
        totalLoss += loss;
    }
    float averageLoss = totalLoss / numWorkers;
    
    // Restore original learning rate
    m_learningRate = originalLearningRate;
    
    return averageLoss;
}

// Implement ICM (Intrinsic Curiosity Module) training
float AIReinforcementLearning::TrainICM(const std::vector<Experience>& batch, float forwardScale, float inverseScale) {
    if (!m_policyModel || !m_policyModel->isModelLoaded()) {
        std::cerr << "Cannot train ICM: Policy model not loaded" << std::endl;
        return -1.0f;
    }
    
    if (batch.empty()) {
        std::cerr << "Cannot train ICM: Empty batch" << std::endl;
        return -1.0f;
    }
    
    // Set default scales if not specified
    if (forwardScale <= 0.0f) forwardScale = 0.8f;  // Default forward loss scale
    if (inverseScale <= 0.0f) inverseScale = 0.2f;  // Default inverse loss scale
    
    // Track total losses
    float totalForwardLoss = 0.0f;
    float totalInverseLoss = 0.0f;
    
    // Process each experience in the batch
    for (const auto& exp : batch) {
        // Forward dynamics model: predict next state given current state and action
        // In a real implementation, this would use a feature model to encode states to features
        
        // 1. Extract features from current state
        std::vector<float> stateFeatures;
        if (m_policyModel) {
            // Use policy model to extract features
            stateFeatures = m_policyModel->extractFeatures(exp.state);
        } else {
            // Simple fallback - use raw state
            stateFeatures = exp.state.getImageData();
        }
        
        // 2. Extract features from next state
        std::vector<float> nextStateFeatures;
        if (m_policyModel) {
            nextStateFeatures = m_policyModel->extractFeatures(exp.nextState);
        } else {
            nextStateFeatures = exp.nextState.getImageData();
        }
        
        // 3. Predict next state features from current state features and action
        std::vector<float> predictedNextStateFeatures;
        if (m_policyModel) {
            predictedNextStateFeatures = m_policyModel->predictNextFeatures(stateFeatures, exp.action);
        } else {
            // Simple fallback - copy current features
            predictedNextStateFeatures = stateFeatures;
        }
        
        // 4. Calculate forward model loss (prediction error)
        float forwardLoss = 0.0f;
        for (size_t i = 0; i < nextStateFeatures.size() && i < predictedNextStateFeatures.size(); ++i) {
            float diff = nextStateFeatures[i] - predictedNextStateFeatures[i];
            forwardLoss += diff * diff;  // MSE
        }
        forwardLoss /= nextStateFeatures.size();
        
        // 5. Use inverse model to predict action from state and next state
        AIOutputAction predictedAction;
        if (m_policyModel) {
            predictedAction = m_policyModel->predictAction(exp.state, exp.nextState);
        } else {
            // Simple fallback - random action
            predictedAction = AIOutputAction();
        }
        
        // 6. Calculate inverse model loss (action prediction error)
        float inverseLoss = exp.action.distance(predictedAction);
        
        // 7. Update total losses
        totalForwardLoss += forwardLoss;
        totalInverseLoss += inverseLoss;
        
        // 8. In a real implementation, we would use these losses to update the ICM model
        // icmModel->update(exp, forwardLoss, inverseLoss);
    }
    
    // Calculate average losses
    float avgForwardLoss = totalForwardLoss / batch.size();
    float avgInverseLoss = totalInverseLoss / batch.size();
    
    // Calculate combined loss
    float combinedLoss = forwardScale * avgForwardLoss + inverseScale * avgInverseLoss;
    
    return combinedLoss;
}

} // namespace AI 