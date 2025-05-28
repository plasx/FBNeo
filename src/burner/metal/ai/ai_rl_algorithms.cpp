#include "ai_rl_algorithms.h"
#include "ai_torch_policy.h"
#include <algorithm>
#include <random>
#include <cmath>
#include <fstream>
#include <iostream>
#include <numeric>
#include <thread>
#include <mutex>

namespace fbneo {
namespace ai {

//------------------------------------------------------------------------------
// ExperienceBuffer Implementation
//------------------------------------------------------------------------------

ExperienceBuffer::ExperienceBuffer(size_t capacity)
    : capacity(capacity), prioritizedReplay(false), priorityAlpha(0.6f), priorityBeta(0.4f) {
    buffer.reserve(capacity);
    priorities.reserve(capacity);
    
    // Initialize random number generator
    std::random_device rd;
    rng = std::mt19937(rd());
}

ExperienceBuffer::~ExperienceBuffer() {
    clear();
}

void ExperienceBuffer::add(const Experience& exp) {
    if (buffer.size() >= capacity) {
        // Remove oldest experience
        buffer.erase(buffer.begin());
        if (!priorities.empty()) {
            priorities.erase(priorities.begin());
        }
    }
    
    // Add new experience
    buffer.push_back(exp);
    
    // Add default priority if using prioritized replay
    if (prioritizedReplay) {
        // New experiences get max priority
        float maxPriority = priorities.empty() ? 1.0f : *std::max_element(priorities.begin(), priorities.end());
        priorities.push_back(maxPriority);
    }
}

std::vector<Experience> ExperienceBuffer::sample(size_t batch_size) {
    if (buffer.empty()) {
        return std::vector<Experience>();
    }
    
    // Limit batch size to buffer size
    batch_size = std::min(batch_size, buffer.size());
    
    std::vector<Experience> batch;
    batch.reserve(batch_size);
    
    if (prioritizedReplay) {
        // Calculate sampling probabilities
        std::vector<float> probs(priorities.size());
        float sum = 0.0f;
        for (size_t i = 0; i < priorities.size(); ++i) {
            probs[i] = std::pow(priorities[i], priorityAlpha);
            sum += probs[i];
        }
        
        // Normalize probabilities
        for (auto& p : probs) {
            p /= sum;
        }
        
        // Create distribution
        std::discrete_distribution<size_t> dist(probs.begin(), probs.end());
        
        // Sample indices
        std::vector<size_t> indices;
        for (size_t i = 0; i < batch_size; ++i) {
            indices.push_back(dist(rng));
        }
        
        // Get experiences
        for (auto idx : indices) {
            batch.push_back(buffer[idx]);
        }
    } else {
        // Uniform random sampling
        std::vector<size_t> indices(buffer.size());
        std::iota(indices.begin(), indices.end(), 0);
        std::shuffle(indices.begin(), indices.end(), rng);
        
        for (size_t i = 0; i < batch_size; ++i) {
            batch.push_back(buffer[indices[i]]);
        }
    }
    
    return batch;
}

void ExperienceBuffer::clear() {
    buffer.clear();
    priorities.clear();
}

size_t ExperienceBuffer::size() const {
    return buffer.size();
}

bool ExperienceBuffer::empty() const {
    return buffer.empty();
}

void ExperienceBuffer::setPriority(size_t index, float priority) {
    if (prioritizedReplay && index < priorities.size()) {
        // Add small constant to prevent zero probability
        priorities[index] = priority + 1e-5f;
    }
}

void ExperienceBuffer::setPrioritizedReplay(bool enabled, float alpha, float beta) {
    prioritizedReplay = enabled;
    priorityAlpha = alpha;
    priorityBeta = beta;
    
    if (enabled && priorities.empty() && !buffer.empty()) {
        // Initialize priorities
        priorities.resize(buffer.size(), 1.0f);
    }
}

//------------------------------------------------------------------------------
// RLAlgorithm Base Class Implementation
//------------------------------------------------------------------------------

RLAlgorithm::RLAlgorithm(AITorchPolicy* policy)
    : policy(policy), learningRate(0.0003f), gamma(0.99f), updateFrequency(4), 
      steps(0), clipEpsilon(0.2f) {
}

RLAlgorithm::~RLAlgorithm() {
    // Base class destructor
}

void RLAlgorithm::processStep(const AIInputFrame& state, const AIOutputAction& action, 
                             float reward, const AIInputFrame& next_state, bool done) {
    // Convert state to vector
    std::vector<float> stateVec;
    // Fill with RGBA values from frame buffer
    if (state.frameBuffer && state.width > 0 && state.height > 0) {
        const uint8_t* frameData = static_cast<const uint8_t*>(state.frameBuffer);
        size_t pixelCount = state.width * state.height;
        stateVec.reserve(pixelCount); // Grayscale for simplicity
        
        for (size_t i = 0; i < pixelCount; ++i) {
            size_t offset = i * 4; // RGBA
            // Convert to grayscale and normalize to [0,1]
            float gray = (0.299f * frameData[offset] + 
                          0.587f * frameData[offset + 1] + 
                          0.114f * frameData[offset + 2]) / 255.0f;
            stateVec.push_back(gray);
        }
    }
    
    // Convert next state to vector
    std::vector<float> nextStateVec;
    if (next_state.frameBuffer && next_state.width > 0 && next_state.height > 0) {
        const uint8_t* frameData = static_cast<const uint8_t*>(next_state.frameBuffer);
        size_t pixelCount = next_state.width * next_state.height;
        nextStateVec.reserve(pixelCount);
        
        for (size_t i = 0; i < pixelCount; ++i) {
            size_t offset = i * 4; // RGBA
            float gray = (0.299f * frameData[offset] + 
                         0.587f * frameData[offset + 1] + 
                         0.114f * frameData[offset + 2]) / 255.0f;
            nextStateVec.push_back(gray);
        }
    }
    
    // Convert action to vector
    std::vector<float> actionVec;
    actionVec.push_back(action.up ? 1.0f : 0.0f);
    actionVec.push_back(action.down ? 1.0f : 0.0f);
    actionVec.push_back(action.left ? 1.0f : 0.0f);
    actionVec.push_back(action.right ? 1.0f : 0.0f);
    for (int i = 0; i < 6; ++i) {
        actionVec.push_back(action.buttons[i] ? 1.0f : 0.0f);
    }
    actionVec.push_back(action.start ? 1.0f : 0.0f);
    actionVec.push_back(action.coin ? 1.0f : 0.0f);
    
    // Create experience
    Experience exp;
    exp.state = stateVec;
    exp.action = actionVec;
    exp.reward = reward;
    exp.next_state = nextStateVec;
    exp.done = done;
    
    // Add to buffer
    buffer.add(exp);
    
    // Increment step counter
    steps++;
}

void RLAlgorithm::setHyperparameters(const std::unordered_map<std::string, float>& params) {
    // Set common hyperparameters
    if (params.count("learning_rate")) {
        learningRate = params.at("learning_rate");
    }
    if (params.count("gamma")) {
        gamma = params.at("gamma");
    }
    if (params.count("update_frequency")) {
        updateFrequency = static_cast<int>(params.at("update_frequency"));
    }
    if (params.count("clip_epsilon")) {
        clipEpsilon = params.at("clip_epsilon");
    }
}

AITorchPolicy* RLAlgorithm::getPolicy() const {
    return policy;
}

ExperienceBuffer& RLAlgorithm::getBuffer() {
    return buffer;
}

void RLAlgorithm::setLearningRate(float lr) {
    learningRate = lr;
}

float RLAlgorithm::getLearningRate() const {
    return learningRate;
}

void RLAlgorithm::setGamma(float g) {
    gamma = g;
}

float RLAlgorithm::getGamma() const {
    return gamma;
}

//------------------------------------------------------------------------------
// PPO Algorithm Implementation
//------------------------------------------------------------------------------

PPOAlgorithm::PPOAlgorithm(AITorchPolicy* policy)
    : RLAlgorithm(policy), clipEpsilon(0.2f), vfCoeff(0.5f), entropyCoeff(0.01f),
      lambda(0.95f), epochs(4) {
    
    // Create target policy (will be a copy of the policy)
    // This would be implemented with the actual policy class
    // targetPolicy = policy->clone();
}

PPOAlgorithm::~PPOAlgorithm() {
    // Cleanup
}

void PPOAlgorithm::train(const std::vector<Experience>& batch) {
    if (batch.empty()) {
        return;
    }
    
    // In a real implementation, this would:
    // 1. Convert experiences to tensors
    // 2. Compute advantages if not already computed
    // 3. Update policy network using PPO loss
    // 4. Periodically update target network
    
    // For this demonstration, we'll outline the algorithm:
    
    // Extract states, actions, rewards, etc.
    std::vector<std::vector<float>> states;
    std::vector<std::vector<float>> actions;
    std::vector<float> rewards;
    std::vector<std::vector<float>> nextStates;
    std::vector<bool> dones;
    std::vector<float> oldLogProbs;
    
    for (const auto& exp : batch) {
        states.push_back(exp.state);
        actions.push_back(exp.action);
        rewards.push_back(exp.reward);
        nextStates.push_back(exp.next_state);
        dones.push_back(exp.done);
        oldLogProbs.push_back(exp.log_prob);
    }
    
    // For multiple epochs
    for (int epoch = 0; epoch < epochs; ++epoch) {
        // Shuffle data for each epoch
        std::vector<size_t> indices(batch.size());
        std::iota(indices.begin(), indices.end(), 0);
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(indices.begin(), indices.end(), g);
        
        // Process in mini-batches
        const size_t miniBatchSize = 64;
        for (size_t i = 0; i < batch.size(); i += miniBatchSize) {
            size_t end = std::min(i + miniBatchSize, batch.size());
            size_t batchSize = end - i;
            
            // Compute actor loss (policy loss)
            // 1. Get current policy logits
            // 2. Compute ratio of new and old policies
            // 3. Compute surrogate losses
            // 4. Take minimum of surrogate losses (PPO clip)
            
            // Compute critic loss (value function loss)
            // 1. Get current value estimates
            // 2. Compute MSE between value estimates and returns
            
            // Compute entropy loss for exploration
            
            // Combine losses and perform gradient descent
            
            // In actual implementation, this would call into the policy's update method
            // policy->update(states_batch, actions_batch, old_log_probs_batch, 
            //               advantages_batch, values_batch, learningRate);
        }
    }
    
    // Periodically update target network
    if (steps % updateFrequency == 0) {
        updateTargetNetwork();
    }
}

void PPOAlgorithm::processStep(const AIInputFrame& state, const AIOutputAction& action, 
                             float reward, const AIInputFrame& next_state, bool done) {
    // Add to experience buffer with PPO-specific data
    
    // Convert to vectors as in the base class
    std::vector<float> stateVec;
    std::vector<float> nextStateVec;
    std::vector<float> actionVec;
    
    // Same conversion as base class...
    
    // Additional PPO-specific processing:
    // 1. Get action probabilities from current policy
    // 2. Calculate log probability of chosen action
    // 3. Get value estimate from critic
    
    float logProb = 0.0f; // Would come from policy
    float value = 0.0f;   // Would come from critic
    
    // Create experience
    Experience exp;
    exp.state = stateVec;
    exp.action = actionVec;
    exp.reward = reward;
    exp.next_state = nextStateVec;
    exp.done = done;
    exp.log_prob = logProb;
    exp.value = value;
    
    // Add to buffer
    buffer.add(exp);
    
    // Increment step counter
    steps++;
    
    // If episode ended, train on this trajectory
    if (done && buffer.size() > 20) { // Minimum batch size
        auto batch = buffer.sample(buffer.size()); // Get all experiences
        std::vector<Experience> trajectory(batch.begin(), batch.end());
        
        // Compute GAE for the trajectory
        computeGAE(trajectory);
        
        // Train for multiple epochs
        trainEpochs(trajectory);
        
        // Clear buffer after training
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
                float val = std::stof(value);
                
                if (key == "learning_rate") {
                    learningRate = val;
                } else if (key == "gamma") {
                    gamma = val;
                } else if (key == "clip_epsilon") {
                    clipEpsilon = val;
                } else if (key == "vf_coeff") {
                    vfCoeff = val;
                } else if (key == "entropy_coeff") {
                    entropyCoeff = val;
                } else if (key == "lambda") {
                    lambda = val;
                } else if (key == "epochs") {
                    epochs = static_cast<int>(val);
                }
            }
        }
        file.close();
    }
    
    // Load networks (would call policy->load())
    std::string policyPath = path + ".policy";
    // policy->load(policyPath);
    
    std::string targetPath = path + ".target";
    // targetPolicy->load(targetPath);
    
    return true;
}

void PPOAlgorithm::computeGAE(std::vector<Experience>& trajectory, float lambda) {
    if (trajectory.empty()) {
        return;
    }
    
    // Compute returns and advantages using Generalized Advantage Estimation
    
    // Initialize
    int n = trajectory.size();
    std::vector<float> returns(n);
    std::vector<float> advantages(n);
    
    // Bootstrap value if not done
    float nextValue = 0.0f;
    if (!trajectory.back().done) {
        // Would get estimate from target network
        // nextValue = targetPolicy->getValue(trajectory.back().next_state);
    }
    
    // Compute GAE
    float gae = 0.0f;
    for (int t = n - 1; t >= 0; --t) {
        // If terminal state, next value is 0
        float nextVal = (t == n - 1) ? nextValue : trajectory[t + 1].value;
        
        // Delta = reward + gamma * nextValue * (1 - done) - value
        float delta = trajectory[t].reward + gamma * nextVal * (1.0f - static_cast<float>(trajectory[t].done)) - trajectory[t].value;
        
        // GAE = delta + gamma * lambda * GAE * (1 - done)
        gae = delta + gamma * lambda * gae * (1.0f - static_cast<float>(trajectory[t].done));
        
        // Advantage = GAE
        advantages[t] = gae;
        
        // Returns = advantage + value
        returns[t] = advantages[t] + trajectory[t].value;
        
        // Store in trajectory
        trajectory[t].advantage = advantages[t];
        trajectory[t].value = returns[t]; // Overwrite with target value
    }
    
    // Normalize advantages
    if (n > 1) {
        float mean = 0.0f;
        float stddev = 0.0f;
        
        // Compute mean
        for (int t = 0; t < n; ++t) {
            mean += advantages[t];
        }
        mean /= n;
        
        // Compute stddev
        for (int t = 0; t < n; ++t) {
            float diff = advantages[t] - mean;
            stddev += diff * diff;
        }
        stddev = std::sqrt(stddev / n);
        
        // Normalize
        if (stddev > 1e-6f) {
            for (int t = 0; t < n; ++t) {
                trajectory[t].advantage = (advantages[t] - mean) / stddev;
            }
        }
    }
}

void PPOAlgorithm::trainEpochs(std::vector<Experience>& trajectory, int epochs) {
    // Train for multiple epochs on the trajectory
    for (int epoch = 0; epoch < epochs; ++epoch) {
        train(trajectory);
    }
}

void PPOAlgorithm::updateTargetNetwork() {
    // In a real implementation, this would copy weights from policy to target policy
    // targetPolicy->copyFrom(policy);
}

//------------------------------------------------------------------------------
// RLAlgorithmFactory Implementation
//------------------------------------------------------------------------------

std::unique_ptr<RLAlgorithm> RLAlgorithmFactory::create(const std::string& type, AITorchPolicy* policy) {
    if (type == "ppo") {
        return std::make_unique<PPOAlgorithm>(policy);
    } else if (type == "a3c") {
        return std::make_unique<A3CAlgorithm>(policy, 4); // 4 workers by default
    } else {
        std::cerr << "Unknown algorithm type: " << type << ". Using PPO as default." << std::endl;
        return std::make_unique<PPOAlgorithm>(policy);
    }
}

std::vector<std::string> RLAlgorithmFactory::getAvailableAlgorithms() {
    return {"ppo", "a3c"}; // Add others as implemented
}

std::unordered_map<std::string, float> RLAlgorithmFactory::getDefaultHyperparameters(const std::string& type) {
    if (type == "ppo") {
        return {
            {"learning_rate", 3e-4f},
            {"gamma", 0.99f},
            {"clip_epsilon", 0.2f},
            {"vf_coeff", 0.5f},
            {"entropy_coeff", 0.01f},
            {"lambda", 0.95f},
            {"epochs", 4.0f},
            {"update_frequency", 4.0f}
        };
    } else if (type == "a3c") {
        return {
            {"learning_rate", 1e-4f},
            {"gamma", 0.99f},
            {"entropy_coeff", 0.01f},
            {"update_frequency", 1.0f}
        };
    } else {
        return {}; // Empty map for unknown types
    }
}

//------------------------------------------------------------------------------
// A3C Algorithm Implementation
//------------------------------------------------------------------------------

A3CAlgorithm::A3CAlgorithm(AITorchPolicy* globalPolicy, int numWorkers)
    : RLAlgorithm(globalPolicy), globalPolicy(globalPolicy), numWorkers(numWorkers), shouldStop(false) {
    
    // Initialize workers
    workers.reserve(numWorkers);
    for (int i = 0; i < numWorkers; ++i) {
        auto worker = std::make_unique<WorkerState>();
        worker->id = i;
        worker->running = false;
        
        // Create a copy of the global policy for this worker
        // In a real implementation, this would be:
        // worker->policy = globalPolicy->clone();
        
        workers.push_back(std::move(worker));
    }
}

A3CAlgorithm::~A3CAlgorithm() {
    // Stop all workers
    stopWorkers();
}

void A3CAlgorithm::train(const std::vector<Experience>& batch) {
    // A3C trains in a distributed manner, so this function
    // mainly serves to pass the experiences to the workers
    if (batch.empty()) {
        return;
    }
    
    // Gradient accumulation lock
    std::lock_guard<std::mutex> lock(globalMutex);
    
    // In a real implementation, this would:
    // 1. Compute advantages and returns
    // 2. Update global policy with experiences
    // 3. Broadcast updated policy to workers
    
    // For demonstration, this is a simplified outline
    
    // Compute advantages (similar to PPO)
    std::vector<Experience> trajectory = batch;
    
    // Compute returns (discount rewards)
    float discounted_reward = 0.0f;
    for (int t = trajectory.size() - 1; t >= 0; --t) {
        discounted_reward = trajectory[t].reward + gamma * discounted_reward * (1.0f - static_cast<float>(trajectory[t].done));
        trajectory[t].value = discounted_reward; // Use as return value
    }
    
    // Compute advantages
    for (auto& exp : trajectory) {
        // Simple advantage: return - value estimate
        exp.advantage = exp.value - exp.value; // In real impl, would use value from network
    }
    
    // In real implementation, update global policy
    // globalPolicy->update(trajectory);
    
    // Synchronize worker policies with global policy
    for (auto& worker : workers) {
        if (worker->running) {
            // In real implementation, copy weights from global to worker
            // worker->policy->copyFrom(globalPolicy);
        }
    }
}

void A3CAlgorithm::processStep(const AIInputFrame& state, const AIOutputAction& action, 
                             float reward, const AIInputFrame& next_state, bool done) {
    // For A3C, the base class implementation is sufficient
    // Each worker will handle its own experience collection
    RLAlgorithm::processStep(state, action, reward, next_state, done);
}

void A3CAlgorithm::setHyperparameters(const std::unordered_map<std::string, float>& params) {
    // First set base class parameters
    RLAlgorithm::setHyperparameters(params);
    
    // Set A3C-specific parameters
    // No specific parameters yet
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
    
    // Load global policy (would call policy->load())
    std::string policyPath = path + ".policy";
    // globalPolicy->load(policyPath);
    
    // Update worker policies
    for (auto& worker : workers) {
        // worker->policy->copyFrom(globalPolicy);
    }
    
    return true;
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
}

void A3CAlgorithm::stopWorkers() {
    // Stop worker threads
    shouldStop = true;
    
    for (auto& worker : workers) {
        if (worker->running) {
            worker->running = false;
            if (worker->thread.joinable()) {
                worker->thread.join();
            }
        }
    }
}

void A3CAlgorithm::workerFunction(WorkerState* state) {
    std::cout << "Worker " << state->id << " started" << std::endl;
    
    // Initialize worker state
    state->buffer.clear();
    
    // In a real implementation, this would:
    // 1. Run a copy of the environment
    // 2. Collect experiences
    // 3. Compute gradients
    // 4. Update global policy
    // 5. Copy global policy back to worker
    
    while (state->running && !shouldStop) {
        // Collect experience
        // This would interface with the game environment
        // For demonstration, we'll just simulate some random experiences
        
        // Initialize episode
        float totalReward = 0.0f;
        bool done = false;
        int stepCount = 0;
        const int maxSteps = 1000;
        
        std::vector<Experience> trajectory;
        
        while (!done && stepCount < maxSteps && state->running && !shouldStop) {
            // Get state
            // In real implementation, this would get the game state
            std::vector<float> stateVec(100, 0.0f); // Dummy state
            
            // Get action from policy
            // In real implementation, this would use the worker's policy
            std::vector<float> actionVec(12, 0.0f); // Dummy action
            
            // Take action in environment
            // In real implementation, this would interface with the game
            std::vector<float> nextStateVec(100, 0.0f); // Dummy next state
            float reward = 0.1f; // Dummy reward
            done = (stepCount >= maxSteps - 1); // End after max steps
            
            // Create experience
            Experience exp;
            exp.state = stateVec;
            exp.action = actionVec;
            exp.reward = reward;
            exp.next_state = nextStateVec;
            exp.done = done;
            
            // Store in trajectory
            trajectory.push_back(exp);
            
            // Update total reward
            totalReward += reward;
            stepCount++;
            
            // Sleep to avoid busy-waiting
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
        // If we collected experiences, update global network
        if (!trajectory.empty()) {
            updateGlobalNetwork(state, trajectory);
            
            // Clear trajectory
            trajectory.clear();
            
            // Log episode completion
            std::cout << "Worker " << state->id << " completed episode with reward " 
                     << totalReward << " in " << stepCount << " steps" << std::endl;
        }
    }
    
    std::cout << "Worker " << state->id << " stopped" << std::endl;
}

void A3CAlgorithm::updateGlobalNetwork(WorkerState* state, const std::vector<Experience>& trajectory) {
    // Compute gradients and update global network
    
    // Lock to prevent concurrent updates to global policy
    {
        std::lock_guard<std::mutex> lock(globalMutex);
        
        // In a real implementation, this would:
        // 1. Compute advantages and returns
        // 2. Compute gradients on worker policy
        // 3. Apply gradients to global policy
        
        // For demonstration, we'll just call the train method
        train(trajectory);
    }
    
    // Update worker policy
    // In real implementation, this would copy weights from global to worker
    // state->policy->copyFrom(globalPolicy);
}

//------------------------------------------------------------------------------
// ICM Module Implementation
//------------------------------------------------------------------------------

ICMModule::ICMModule(AITorchPolicy* policy)
    : policy(policy), rewardScale(0.01f), forwardLossWeight(0.2f), inverseLossWeight(0.8f) {
}

ICMModule::~ICMModule() {
    // Cleanup
}

bool ICMModule::initialize() {
    // Create feature network
    // In real implementation, this would:
    // 1. Create a network to encode states to features
    // 2. Create a forward model to predict next state features
    // 3. Create an inverse model to predict actions from states
    
    // featureNetwork = std::make_unique<AITorchPolicy>();
    // forwardModel = std::make_unique<AITorchPolicy>();
    // inverseModel = std::make_unique<AITorchPolicy>();
    
    return true;
}

float ICMModule::calculateIntrinsicReward(const AIInputFrame& state, const AIOutputAction& action, 
                                        const AIInputFrame& next_state) {
    // Encode states to features
    std::vector<float> stateFeatures = encodeState(state);
    std::vector<float> nextStateFeatures = encodeState(next_state);
    
    // Predict next state features
    std::vector<float> predictedNextStateFeatures = predictNextState(stateFeatures, action);
    
    // Calculate prediction error (forward model error)
    float error = 0.0f;
    if (!predictedNextStateFeatures.empty() && !nextStateFeatures.empty()) {
        for (size_t i = 0; i < predictedNextStateFeatures.size(); ++i) {
            float diff = predictedNextStateFeatures[i] - nextStateFeatures[i];
            error += diff * diff;
        }
        error = std::sqrt(error / predictedNextStateFeatures.size());
    }
    
    // Scale error to get intrinsic reward
    return error * rewardScale;
}

void ICMModule::update(const std::vector<Experience>& batch) {
    if (batch.empty()) {
        return;
    }
    
    // In a real implementation, this would:
    // 1. Convert experiences to tensors
    // 2. Encode states to features
    // 3. Predict next state features
    // 4. Predict actions from state pairs
    // 5. Compute forward and inverse losses
    // 6. Update networks
    
    // For demonstration, we'll simulate the process
    
    // Extract data
    std::vector<std::vector<float>> states;
    std::vector<std::vector<float>> actions;
    std::vector<std::vector<float>> nextStates;
    
    for (const auto& exp : batch) {
        states.push_back(exp.state);
        actions.push_back(exp.action);
        nextStates.push_back(exp.next_state);
    }
    
    // In actual implementation, this would update the networks
    // featureNetwork->update(...);
    // forwardModel->update(...);
    // inverseModel->update(...);
}

void ICMModule::setRewardScale(float scale) {
    rewardScale = scale;
}

float ICMModule::getRewardScale() const {
    return rewardScale;
}

bool ICMModule::save(const std::string& path) {
    // Save networks
    std::cout << "Saving ICM to " << path << std::endl;
    
    // Save hyperparameters
    std::string hyperparamsPath = path + ".icm_params";
    std::ofstream file(hyperparamsPath);
    if (file.is_open()) {
        file << "reward_scale=" << rewardScale << std::endl;
        file << "forward_loss_weight=" << forwardLossWeight << std::endl;
        file << "inverse_loss_weight=" << inverseLossWeight << std::endl;
        file.close();
    }
    
    // Save networks (would call save() on each)
    std::string featurePath = path + ".icm_feature";
    // featureNetwork->save(featurePath);
    
    std::string forwardPath = path + ".icm_forward";
    // forwardModel->save(forwardPath);
    
    std::string inversePath = path + ".icm_inverse";
    // inverseModel->save(inversePath);
    
    return true;
}

bool ICMModule::load(const std::string& path) {
    // Load networks
    std::cout << "Loading ICM from " << path << std::endl;
    
    // Load hyperparameters
    std::string hyperparamsPath = path + ".icm_params";
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
    
    // Load networks (would call load() on each)
    std::string featurePath = path + ".icm_feature";
    // featureNetwork->load(featurePath);
    
    std::string forwardPath = path + ".icm_forward";
    // forwardModel->load(forwardPath);
    
    std::string inversePath = path + ".icm_inverse";
    // inverseModel->load(inversePath);
    
    return true;
}

std::vector<float> ICMModule::encodeState(const AIInputFrame& state) {
    // Encode state to features
    std::vector<float> features;
    
    // In a real implementation, this would use the feature network
    // to encode the state into a feature vector
    
    // For demonstration, we'll create a simple feature vector
    if (state.frameBuffer && state.width > 0 && state.height > 0) {
        const uint8_t* frameData = static_cast<const uint8_t*>(state.frameBuffer);
        
        // Downsample to a small feature vector
        const int featureSize = 16;
        features.resize(featureSize);
        
        // Compute average pixel values in grid cells
        int cellWidth = state.width / 4;
        int cellHeight = state.height / 4;
        
        for (int y = 0; y < 4; ++y) {
            for (int x = 0; x < 4; ++x) {
                float sum = 0.0f;
                int count = 0;
                
                for (int cy = 0; cy < cellHeight; ++cy) {
                    for (int cx = 0; cx < cellWidth; ++cx) {
                        int px = x * cellWidth + cx;
                        int py = y * cellHeight + cy;
                        
                        if (px < state.width && py < state.height) {
                            int offset = (py * state.width + px) * 4; // RGBA
                            
                            // Convert to grayscale
                            float gray = (0.299f * frameData[offset] + 
                                         0.587f * frameData[offset + 1] + 
                                         0.114f * frameData[offset + 2]) / 255.0f;
                            
                            sum += gray;
                            count++;
                        }
                    }
                }
                
                // Store average
                features[y * 4 + x] = count > 0 ? (sum / count) : 0.0f;
            }
        }
    }
    
    return features;
}

std::vector<float> ICMModule::predictNextState(const std::vector<float>& stateFeatures, 
                                             const AIOutputAction& action) {
    // Predict next state features from current state features and action
    std::vector<float> predictedFeatures;
    
    // In a real implementation, this would use the forward model
    // to predict the next state features
    
    // For demonstration, we'll just return the input features
    // with some noise to simulate prediction
    if (!stateFeatures.empty()) {
        predictedFeatures = stateFeatures;
        
        // Add some noise to features to simulate prediction
        std::random_device rd;
        std::mt19937 gen(rd());
        std::normal_distribution<float> noise(0.0f, 0.1f);
        
        for (auto& f : predictedFeatures) {
            f += noise(gen);
        }
    }
    
    return predictedFeatures;
}

std::vector<float> ICMModule::predictAction(const std::vector<float>& stateFeatures, 
                                          const std::vector<float>& nextStateFeatures) {
    // Predict action from state and next state features
    std::vector<float> predictedAction;
    
    // In a real implementation, this would use the inverse model
    // to predict the action that led from state to next state
    
    // For demonstration, we'll create a simple prediction
    if (!stateFeatures.empty() && !nextStateFeatures.empty()) {
        // Create a 12-dimensional action vector (same as AIOutputAction)
        predictedAction.resize(12, 0.0f);
        
        // Compute differences between features
        std::vector<float> diffs;
        for (size_t i = 0; i < stateFeatures.size(); ++i) {
            diffs.push_back(nextStateFeatures[i] - stateFeatures[i]);
        }
        
        // Use differences to predict action
        // This is very simplistic and just for demonstration
        if (!diffs.empty()) {
            // Map largest positive and negative diffs to directions
            float maxDiff = -1.0f;
            float minDiff = 1.0f;
            size_t maxIdx = 0;
            size_t minIdx = 0;
            
            for (size_t i = 0; i < diffs.size(); ++i) {
                if (diffs[i] > maxDiff) {
                    maxDiff = diffs[i];
                    maxIdx = i;
                }
                if (diffs[i] < minDiff) {
                    minDiff = diffs[i];
                    minIdx = i;
                }
            }
            
            // Map to directional actions
            if (maxIdx % 4 == 0) predictedAction[0] = 1.0f; // Up
            if (maxIdx % 4 == 1) predictedAction[1] = 1.0f; // Down
            if (maxIdx % 4 == 2) predictedAction[2] = 1.0f; // Left
            if (maxIdx % 4 == 3) predictedAction[3] = 1.0f; // Right
            
            // Random button press
            if (maxDiff > 0.2f) {
                predictedAction[4 + (maxIdx % 6)] = 1.0f; // Button press
            }
        }
    }
    
    return predictedAction;
}

} // namespace ai
} // namespace fbneo 