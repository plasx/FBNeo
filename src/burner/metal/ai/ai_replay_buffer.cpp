#include "ai_replay_buffer.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <cmath>

namespace fbneo {
namespace ai {

//=========================================================================
// UniformReplayBuffer implementation
//=========================================================================

UniformReplayBuffer::UniformReplayBuffer(size_t capacity)
    : m_capacity(capacity), 
      m_rng(std::chrono::system_clock::now().time_since_epoch().count()) {
    m_buffer.clear();
}

UniformReplayBuffer::~UniformReplayBuffer() {
    clear();
}

void UniformReplayBuffer::add(const Transition& transition) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_buffer.size() >= m_capacity) {
        m_buffer.pop_front();
    }
    m_buffer.push_back(transition);
}

void UniformReplayBuffer::add(const AIInputFrame& state, const AIOutputAction& action, 
                            float reward, const AIInputFrame& nextState, bool done) {
    Transition transition(state, action, reward, nextState, done);
    add(transition);
}

TransitionBatch UniformReplayBuffer::sample(size_t batchSize) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    TransitionBatch batch;
    batch.reserve(batchSize);
    
    if (m_buffer.empty()) {
        return batch;
    }
    
    size_t bufferSize = m_buffer.size();
    batchSize = std::min(batchSize, bufferSize);
    
    std::uniform_int_distribution<size_t> dist(0, bufferSize - 1);
    
    for (size_t i = 0; i < batchSize; ++i) {
        size_t idx = dist(m_rng);
        const Transition& transition = m_buffer[idx];
        
        batch.states.push_back(transition.state);
        batch.actions.push_back(transition.action);
        batch.rewards.push_back(transition.reward);
        batch.nextStates.push_back(transition.nextState);
        batch.dones.push_back(transition.done);
        batch.weights.push_back(1.0f); // Uniform weights
        batch.indices.push_back(idx);
    }
    
    return batch;
}

void UniformReplayBuffer::update_priorities(const std::vector<size_t>& indices, 
                                         const std::vector<float>& priorities) {
    // No-op for uniform buffer
    return;
}

size_t UniformReplayBuffer::size() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_buffer.size();
}

size_t UniformReplayBuffer::capacity() const {
    return m_capacity;
}

void UniformReplayBuffer::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_buffer.clear();
}

bool UniformReplayBuffer::save(const std::string& path) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    try {
        std::ofstream file(path, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "Failed to open file for writing: " << path << std::endl;
            return false;
        }
        
        // Write buffer size
        size_t bufferSize = m_buffer.size();
        file.write(reinterpret_cast<const char*>(&bufferSize), sizeof(bufferSize));
        
        // Write each transition
        for (const auto& transition : m_buffer) {
            // Write state
            size_t stateSize = transition.state.size();
            file.write(reinterpret_cast<const char*>(&stateSize), sizeof(stateSize));
            file.write(reinterpret_cast<const char*>(transition.state.data()), stateSize * sizeof(float));
            
            // Write action
            size_t actionSize = transition.action.size();
            file.write(reinterpret_cast<const char*>(&actionSize), sizeof(actionSize));
            file.write(reinterpret_cast<const char*>(transition.action.data()), actionSize * sizeof(float));
            
            // Write reward
            file.write(reinterpret_cast<const char*>(&transition.reward), sizeof(transition.reward));
            
            // Write next state
            size_t nextStateSize = transition.nextState.size();
            file.write(reinterpret_cast<const char*>(&nextStateSize), sizeof(nextStateSize));
            file.write(reinterpret_cast<const char*>(transition.nextState.data()), nextStateSize * sizeof(float));
            
            // Write done flag and priority
            file.write(reinterpret_cast<const char*>(&transition.done), sizeof(transition.done));
            file.write(reinterpret_cast<const char*>(&transition.priority), sizeof(transition.priority));
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error saving uniform replay buffer: " << e.what() << std::endl;
        return false;
    }
}

bool UniformReplayBuffer::load(const std::string& path) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    try {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "Failed to open file for reading: " << path << std::endl;
            return false;
        }
        
        // Clear existing buffer
        m_buffer.clear();
        
        // Read buffer size
        size_t bufferSize;
        file.read(reinterpret_cast<char*>(&bufferSize), sizeof(bufferSize));
        
        // Read each transition
        for (size_t i = 0; i < bufferSize && i < m_capacity; ++i) {
            Transition transition;
            
            // Read state
            size_t stateSize;
            file.read(reinterpret_cast<char*>(&stateSize), sizeof(stateSize));
            transition.state.resize(stateSize);
            file.read(reinterpret_cast<char*>(transition.state.data()), stateSize * sizeof(float));
            
            // Read action
            size_t actionSize;
            file.read(reinterpret_cast<char*>(&actionSize), sizeof(actionSize));
            transition.action.resize(actionSize);
            file.read(reinterpret_cast<char*>(transition.action.data()), actionSize * sizeof(float));
            
            // Read reward
            file.read(reinterpret_cast<char*>(&transition.reward), sizeof(transition.reward));
            
            // Read next state
            size_t nextStateSize;
            file.read(reinterpret_cast<char*>(&nextStateSize), sizeof(nextStateSize));
            transition.nextState.resize(nextStateSize);
            file.read(reinterpret_cast<char*>(transition.nextState.data()), nextStateSize * sizeof(float));
            
            // Read done flag and priority
            file.read(reinterpret_cast<char*>(&transition.done), sizeof(transition.done));
            file.read(reinterpret_cast<char*>(&transition.priority), sizeof(transition.priority));
            
            m_buffer.push_back(transition);
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error loading uniform replay buffer: " << e.what() << std::endl;
        return false;
    }
}

//=========================================================================
// PrioritizedReplayBuffer implementation
//=========================================================================

// Initialize SumTree
PrioritizedReplayBuffer::SumTree::SumTree(size_t capacity) 
    : capacity(capacity), data_size(0) {
    // Allocate tree with twice capacity - 1 elements
    // (this allows for a complete binary tree)
    tree.resize(2 * capacity - 1, 0.0f);
    data_indices.resize(capacity);
}

void PrioritizedReplayBuffer::SumTree::update(size_t idx, float priority) {
    // Idx is in data indices, we need to convert to tree index
    size_t tree_idx = idx + capacity - 1;
    // Change in the tree
    float change = priority - tree[tree_idx];
    tree[tree_idx] = priority;
    
    // Propagate changes through parent nodes
    while (tree_idx != 0) {
        tree_idx = (tree_idx - 1) / 2;
        tree[tree_idx] += change;
    }
}

float PrioritizedReplayBuffer::SumTree::sum() const {
    if (tree.empty()) return 0.0f;
    return tree[0];  // Root node contains sum of all priorities
}

std::pair<size_t, float> PrioritizedReplayBuffer::SumTree::get(float value) const {
    size_t idx = 0;
    
    while (idx < capacity - 1) {  // While not a leaf node
        size_t left = 2 * idx + 1;
        size_t right = left + 1;
        
        if (left >= tree.size()) break;  // Safety check
        
        // Check if we go left or right
        if (value <= tree[left] || right >= tree.size()) {
            idx = left;
        } else {
            value -= tree[left];
            idx = right;
        }
    }
    
    size_t data_idx = idx - (capacity - 1);
    return {data_idx, tree[idx]};
}

// PrioritizedReplayBuffer implementation
PrioritizedReplayBuffer::PrioritizedReplayBuffer(size_t capacity, float alpha, float beta)
    : m_buffer(capacity), 
      m_sumTree(capacity),
      m_position(0),
      m_capacity(capacity),
      m_alpha(alpha),
      m_beta(beta),
      m_maxPriority(1.0f),
      m_rng(std::chrono::system_clock::now().time_since_epoch().count()) {
}

PrioritizedReplayBuffer::~PrioritizedReplayBuffer() {
    clear();
}

void PrioritizedReplayBuffer::add(const Transition& transition) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Use max priority for new transitions
    float priority = std::pow(m_maxPriority, m_alpha);
    
    if (m_buffer.size() < m_capacity) {
        m_buffer.push_back(transition);
    } else {
        m_buffer[m_position] = transition;
    }
    
    // Update sum tree
    m_sumTree.update(m_position, priority);
    
    // Update position
    m_position = (m_position + 1) % m_capacity;
}

void PrioritizedReplayBuffer::add(const AIInputFrame& state, const AIOutputAction& action, 
                                float reward, const AIInputFrame& nextState, bool done) {
    Transition transition(state, action, reward, nextState, done);
    add(transition);
}

TransitionBatch PrioritizedReplayBuffer::sample(size_t batchSize) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    TransitionBatch batch;
    batch.reserve(batchSize);
    
    size_t bufferSize = std::min(m_buffer.size(), m_capacity);
    if (bufferSize == 0) {
        return batch;
    }
    
    // Adjust batch size if buffer is too small
    batchSize = std::min(batchSize, bufferSize);
    
    // Calculate segment size
    float segment = m_sumTree.sum() / static_cast<float>(batchSize);
    
    // Sample from each segment
    for (size_t i = 0; i < batchSize; ++i) {
        // Calculate range for this segment
        float a = segment * i;
        float b = segment * (i + 1);
        
        // Sample uniformly from segment
        std::uniform_real_distribution<float> dist(a, b);
        float value = dist(m_rng);
        
        // Get sample from sum tree
        auto [idx, priority] = m_sumTree.get(value);
        
        // Ensure we're within bounds
        if (idx >= bufferSize) {
            idx = m_rng() % bufferSize;
        }
        
        // Calculate importance sampling weight
        float weight = 1.0f;
        if (priority > 0) {
            // P(j) = p_j^α / sum_k p_k^α
            // Importance sampling weight: w_j = (1/N * 1/P(j))^β
            // Normalize by max_weight to ensure weights <= 1
            weight = std::pow(bufferSize * priority, -m_beta);
        }
        
        const Transition& transition = m_buffer[idx];
        
        batch.states.push_back(transition.state);
        batch.actions.push_back(transition.action);
        batch.rewards.push_back(transition.reward);
        batch.nextStates.push_back(transition.nextState);
        batch.dones.push_back(transition.done);
        batch.weights.push_back(weight);
        batch.indices.push_back(idx);
    }
    
    return batch;
}

void PrioritizedReplayBuffer::update_priorities(const std::vector<size_t>& indices, 
                                             const std::vector<float>& priorities) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (indices.size() != priorities.size()) {
        std::cerr << "Error: indices and priorities size mismatch in update_priorities" << std::endl;
        return;
    }
    
    for (size_t i = 0; i < indices.size(); ++i) {
        size_t idx = indices[i];
        float priority = priorities[i];
        
        // Clip priority to small positive value to ensure non-zero sampling probability
        priority = std::max(priority, 1e-5f);
        
        // Update max priority
        m_maxPriority = std::max(m_maxPriority, priority);
        
        // Update transition priority
        if (idx < m_buffer.size()) {
            m_buffer[idx].priority = priority;
            
            // Update sum tree with alpha-adjusted priority
            float alpha_priority = std::pow(priority, m_alpha);
            m_sumTree.update(idx, alpha_priority);
        }
    }
}

size_t PrioritizedReplayBuffer::size() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return std::min(m_buffer.size(), m_capacity);
}

size_t PrioritizedReplayBuffer::capacity() const {
    return m_capacity;
}

void PrioritizedReplayBuffer::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_buffer.clear();
    m_position = 0;
    
    // Reset sum tree
    for (size_t i = 0; i < 2 * m_capacity - 1; ++i) {
        m_sumTree.tree[i] = 0.0f;
    }
}

void PrioritizedReplayBuffer::set_beta(float beta) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_beta = beta;
}

float PrioritizedReplayBuffer::get_max_priority() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_maxPriority;
}

bool PrioritizedReplayBuffer::save(const std::string& path) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    try {
        std::ofstream file(path, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "Failed to open file for writing: " << path << std::endl;
            return false;
        }
        
        // Write buffer metadata
        size_t bufferSize = std::min(m_buffer.size(), m_capacity);
        file.write(reinterpret_cast<const char*>(&bufferSize), sizeof(bufferSize));
        file.write(reinterpret_cast<const char*>(&m_position), sizeof(m_position));
        file.write(reinterpret_cast<const char*>(&m_alpha), sizeof(m_alpha));
        file.write(reinterpret_cast<const char*>(&m_beta), sizeof(m_beta));
        file.write(reinterpret_cast<const char*>(&m_maxPriority), sizeof(m_maxPriority));
        
        // Write each transition
        for (size_t i = 0; i < bufferSize; ++i) {
            const auto& transition = m_buffer[i];
            
            // Write state
            size_t stateSize = transition.state.size();
            file.write(reinterpret_cast<const char*>(&stateSize), sizeof(stateSize));
            file.write(reinterpret_cast<const char*>(transition.state.data()), stateSize * sizeof(float));
            
            // Write action
            size_t actionSize = transition.action.size();
            file.write(reinterpret_cast<const char*>(&actionSize), sizeof(actionSize));
            file.write(reinterpret_cast<const char*>(transition.action.data()), actionSize * sizeof(float));
            
            // Write reward
            file.write(reinterpret_cast<const char*>(&transition.reward), sizeof(transition.reward));
            
            // Write next state
            size_t nextStateSize = transition.nextState.size();
            file.write(reinterpret_cast<const char*>(&nextStateSize), sizeof(nextStateSize));
            file.write(reinterpret_cast<const char*>(transition.nextState.data()), nextStateSize * sizeof(float));
            
            // Write done flag and priority
            file.write(reinterpret_cast<const char*>(&transition.done), sizeof(transition.done));
            file.write(reinterpret_cast<const char*>(&transition.priority), sizeof(transition.priority));
        }
        
        // Write sum tree
        file.write(reinterpret_cast<const char*>(m_sumTree.tree.data()), m_sumTree.tree.size() * sizeof(float));
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error saving prioritized replay buffer: " << e.what() << std::endl;
        return false;
    }
}

bool PrioritizedReplayBuffer::load(const std::string& path) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    try {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "Failed to open file for reading: " << path << std::endl;
            return false;
        }
        
        // Clear existing buffer
        clear();
        
        // Read buffer metadata
        size_t bufferSize;
        file.read(reinterpret_cast<char*>(&bufferSize), sizeof(bufferSize));
        file.read(reinterpret_cast<char*>(&m_position), sizeof(m_position));
        file.read(reinterpret_cast<char*>(&m_alpha), sizeof(m_alpha));
        file.read(reinterpret_cast<char*>(&m_beta), sizeof(m_beta));
        file.read(reinterpret_cast<char*>(&m_maxPriority), sizeof(m_maxPriority));
        
        // Ensure we don't read more than capacity
        bufferSize = std::min(bufferSize, m_capacity);
        
        // Read each transition
        for (size_t i = 0; i < bufferSize; ++i) {
            Transition transition;
            
            // Read state
            size_t stateSize;
            file.read(reinterpret_cast<char*>(&stateSize), sizeof(stateSize));
            transition.state.resize(stateSize);
            file.read(reinterpret_cast<char*>(transition.state.data()), stateSize * sizeof(float));
            
            // Read action
            size_t actionSize;
            file.read(reinterpret_cast<char*>(&actionSize), sizeof(actionSize));
            transition.action.resize(actionSize);
            file.read(reinterpret_cast<char*>(transition.action.data()), actionSize * sizeof(float));
            
            // Read reward
            file.read(reinterpret_cast<char*>(&transition.reward), sizeof(transition.reward));
            
            // Read next state
            size_t nextStateSize;
            file.read(reinterpret_cast<char*>(&nextStateSize), sizeof(nextStateSize));
            transition.nextState.resize(nextStateSize);
            file.read(reinterpret_cast<char*>(transition.nextState.data()), nextStateSize * sizeof(float));
            
            // Read done flag and priority
            file.read(reinterpret_cast<char*>(&transition.done), sizeof(transition.done));
            file.read(reinterpret_cast<char*>(&transition.priority), sizeof(transition.priority));
            
            m_buffer.push_back(transition);
            
            // Update sum tree with alpha-adjusted priority
            float alpha_priority = std::pow(transition.priority, m_alpha);
            m_sumTree.update(i, alpha_priority);
        }
        
        // Read sum tree (optional, since we've already reconstructed it)
        if (file.peek() != EOF) {
            file.read(reinterpret_cast<char*>(m_sumTree.tree.data()), m_sumTree.tree.size() * sizeof(float));
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error loading prioritized replay buffer: " << e.what() << std::endl;
        return false;
    }
}

//=========================================================================
// EpisodicReplayBuffer implementation
//=========================================================================

EpisodicReplayBuffer::EpisodicReplayBuffer(size_t capacity)
    : m_capacity(capacity),
      m_totalTransitions(0),
      m_rng(std::chrono::system_clock::now().time_since_epoch().count()) {
    m_episodes.clear();
    m_currentEpisode.clear();
}

EpisodicReplayBuffer::~EpisodicReplayBuffer() {
    clear();
}

void EpisodicReplayBuffer::add(const Transition& transition) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Add to current episode
    m_currentEpisode.push_back(transition);
    m_totalTransitions++;
    
    // If terminal state, end the episode
    if (transition.done) {
        end_episode();
    }
}

void EpisodicReplayBuffer::add(const AIInputFrame& state, const AIOutputAction& action, 
                            float reward, const AIInputFrame& nextState, bool done) {
    Transition transition(state, action, reward, nextState, done);
    add(transition);
}

void EpisodicReplayBuffer::end_episode() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Only add non-empty episodes
    if (!m_currentEpisode.empty()) {
        m_episodes.push_back(m_currentEpisode);
        
        // If we exceed capacity, remove oldest episode
        while (m_episodes.size() > m_capacity) {
            m_totalTransitions -= m_episodes.front().size();
            m_episodes.erase(m_episodes.begin());
        }
        
        // Clear current episode
        m_currentEpisode.clear();
    }
}

TransitionBatch EpisodicReplayBuffer::sample(size_t batchSize) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    TransitionBatch batch;
    batch.reserve(batchSize);
    
    if (m_episodes.empty() && m_currentEpisode.empty()) {
        return batch;
    }
    
    // Sample transitions from episodes
    size_t numSampled = 0;
    
    // First try to sample from completed episodes
    if (!m_episodes.empty()) {
        // Randomly select episodes and transitions within them
        std::uniform_int_distribution<size_t> episodeDist(0, m_episodes.size() - 1);
        
        while (numSampled < batchSize) {
            // Select a random episode
            size_t episodeIdx = episodeDist(m_rng);
            const auto& episode = m_episodes[episodeIdx];
            
            if (episode.empty()) continue;
            
            // Select a random transition from the episode
            std::uniform_int_distribution<size_t> transitionDist(0, episode.size() - 1);
            size_t transitionIdx = transitionDist(m_rng);
            const auto& transition = episode[transitionIdx];
            
            batch.states.push_back(transition.state);
            batch.actions.push_back(transition.action);
            batch.rewards.push_back(transition.reward);
            batch.nextStates.push_back(transition.nextState);
            batch.dones.push_back(transition.done);
            batch.weights.push_back(1.0f); // Uniform weights for episodic
            
            // Store the global index (episode, transition)
            // We encode it as (episodeIdx << 32) | transitionIdx
            size_t globalIdx = (episodeIdx << 32) | transitionIdx;
            batch.indices.push_back(globalIdx);
            
            numSampled++;
            
            // Stop if we've sampled enough
            if (numSampled >= batchSize) break;
        }
    }
    
    // If we need more samples and have a current episode, sample from it
    if (numSampled < batchSize && !m_currentEpisode.empty()) {
        std::uniform_int_distribution<size_t> transitionDist(0, m_currentEpisode.size() - 1);
        
        while (numSampled < batchSize) {
            size_t transitionIdx = transitionDist(m_rng);
            const auto& transition = m_currentEpisode[transitionIdx];
            
            batch.states.push_back(transition.state);
            batch.actions.push_back(transition.action);
            batch.rewards.push_back(transition.reward);
            batch.nextStates.push_back(transition.nextState);
            batch.dones.push_back(transition.done);
            batch.weights.push_back(1.0f); // Uniform weights for episodic
            
            // Store index in current episode with special marker
            // We set the highest bit to 1 to indicate current episode
            size_t globalIdx = (1ULL << 63) | transitionIdx;
            batch.indices.push_back(globalIdx);
            
            numSampled++;
        }
    }
    
    return batch;
}

std::vector<Transition> EpisodicReplayBuffer::sample_episode() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_episodes.empty()) {
        return std::vector<Transition>();
    }
    
    // Select a random episode
    std::uniform_int_distribution<size_t> episodeDist(0, m_episodes.size() - 1);
    size_t episodeIdx = episodeDist(m_rng);
    
    return m_episodes[episodeIdx];
}

void EpisodicReplayBuffer::update_priorities(const std::vector<size_t>& indices, 
                                          const std::vector<float>& priorities) {
    // No priority updates for episodic buffer
    return;
}

size_t EpisodicReplayBuffer::size() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_totalTransitions;
}

size_t EpisodicReplayBuffer::capacity() const {
    return m_capacity;
}

void EpisodicReplayBuffer::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_episodes.clear();
    m_currentEpisode.clear();
    m_totalTransitions = 0;
}

bool EpisodicReplayBuffer::save(const std::string& path) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    try {
        std::ofstream file(path, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "Failed to open file for writing: " << path << std::endl;
            return false;
        }
        
        // Write number of episodes
        size_t numEpisodes = m_episodes.size();
        file.write(reinterpret_cast<const char*>(&numEpisodes), sizeof(numEpisodes));
        
        // Write whether we have a current episode
        bool hasCurrentEpisode = !m_currentEpisode.empty();
        file.write(reinterpret_cast<const char*>(&hasCurrentEpisode), sizeof(hasCurrentEpisode));
        
        // Write each episode
        for (const auto& episode : m_episodes) {
            // Write episode size
            size_t episodeSize = episode.size();
            file.write(reinterpret_cast<const char*>(&episodeSize), sizeof(episodeSize));
            
            // Write each transition in the episode
            for (const auto& transition : episode) {
                // Write state
                size_t stateSize = transition.state.size();
                file.write(reinterpret_cast<const char*>(&stateSize), sizeof(stateSize));
                file.write(reinterpret_cast<const char*>(transition.state.data()), stateSize * sizeof(float));
                
                // Write action
                size_t actionSize = transition.action.size();
                file.write(reinterpret_cast<const char*>(&actionSize), sizeof(actionSize));
                file.write(reinterpret_cast<const char*>(transition.action.data()), actionSize * sizeof(float));
                
                // Write reward
                file.write(reinterpret_cast<const char*>(&transition.reward), sizeof(transition.reward));
                
                // Write next state
                size_t nextStateSize = transition.nextState.size();
                file.write(reinterpret_cast<const char*>(&nextStateSize), sizeof(nextStateSize));
                file.write(reinterpret_cast<const char*>(transition.nextState.data()), nextStateSize * sizeof(float));
                
                // Write done flag and priority
                file.write(reinterpret_cast<const char*>(&transition.done), sizeof(transition.done));
                file.write(reinterpret_cast<const char*>(&transition.priority), sizeof(transition.priority));
            }
        }
        
        // Write current episode if it exists
        if (hasCurrentEpisode) {
            // Write episode size
            size_t episodeSize = m_currentEpisode.size();
            file.write(reinterpret_cast<const char*>(&episodeSize), sizeof(episodeSize));
            
            // Write each transition in the current episode
            for (const auto& transition : m_currentEpisode) {
                // Write state
                size_t stateSize = transition.state.size();
                file.write(reinterpret_cast<const char*>(&stateSize), sizeof(stateSize));
                file.write(reinterpret_cast<const char*>(transition.state.data()), stateSize * sizeof(float));
                
                // Write action
                size_t actionSize = transition.action.size();
                file.write(reinterpret_cast<const char*>(&actionSize), sizeof(actionSize));
                file.write(reinterpret_cast<const char*>(transition.action.data()), actionSize * sizeof(float));
                
                // Write reward
                file.write(reinterpret_cast<const char*>(&transition.reward), sizeof(transition.reward));
                
                // Write next state
                size_t nextStateSize = transition.nextState.size();
                file.write(reinterpret_cast<const char*>(&nextStateSize), sizeof(nextStateSize));
                file.write(reinterpret_cast<const char*>(transition.nextState.data()), nextStateSize * sizeof(float));
                
                // Write done flag and priority
                file.write(reinterpret_cast<const char*>(&transition.done), sizeof(transition.done));
                file.write(reinterpret_cast<const char*>(&transition.priority), sizeof(transition.priority));
            }
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error saving episodic replay buffer: " << e.what() << std::endl;
        return false;
    }
}

bool EpisodicReplayBuffer::load(const std::string& path) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    try {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "Failed to open file for reading: " << path << std::endl;
            return false;
        }
        
        // Clear existing buffer
        clear();
        
        // Read number of episodes
        size_t numEpisodes;
        file.read(reinterpret_cast<char*>(&numEpisodes), sizeof(numEpisodes));
        
        // Read whether we have a current episode
        bool hasCurrentEpisode;
        file.read(reinterpret_cast<char*>(&hasCurrentEpisode), sizeof(hasCurrentEpisode));
        
        // Read each episode
        for (size_t i = 0; i < numEpisodes && i < m_capacity; ++i) {
            // Read episode size
            size_t episodeSize;
            file.read(reinterpret_cast<char*>(&episodeSize), sizeof(episodeSize));
            
            std::vector<Transition> episode;
            episode.reserve(episodeSize);
            
            // Read each transition in the episode
            for (size_t j = 0; j < episodeSize; ++j) {
                Transition transition;
                
                // Read state
                size_t stateSize;
                file.read(reinterpret_cast<char*>(&stateSize), sizeof(stateSize));
                transition.state.resize(stateSize);
                file.read(reinterpret_cast<char*>(transition.state.data()), stateSize * sizeof(float));
                
                // Read action
                size_t actionSize;
                file.read(reinterpret_cast<char*>(&actionSize), sizeof(actionSize));
                transition.action.resize(actionSize);
                file.read(reinterpret_cast<char*>(transition.action.data()), actionSize * sizeof(float));
                
                // Read reward
                file.read(reinterpret_cast<char*>(&transition.reward), sizeof(transition.reward));
                
                // Read next state
                size_t nextStateSize;
                file.read(reinterpret_cast<char*>(&nextStateSize), sizeof(nextStateSize));
                transition.nextState.resize(nextStateSize);
                file.read(reinterpret_cast<char*>(transition.nextState.data()), nextStateSize * sizeof(float));
                
                // Read done flag and priority
                file.read(reinterpret_cast<char*>(&transition.done), sizeof(transition.done));
                file.read(reinterpret_cast<char*>(&transition.priority), sizeof(transition.priority));
                
                episode.push_back(transition);
                m_totalTransitions++;
            }
            
            m_episodes.push_back(episode);
        }
        
        // Read current episode if it exists
        if (hasCurrentEpisode) {
            // Read episode size
            size_t episodeSize;
            file.read(reinterpret_cast<char*>(&episodeSize), sizeof(episodeSize));
            
            m_currentEpisode.reserve(episodeSize);
            
            // Read each transition in the current episode
            for (size_t j = 0; j < episodeSize; ++j) {
                Transition transition;
                
                // Read state
                size_t stateSize;
                file.read(reinterpret_cast<char*>(&stateSize), sizeof(stateSize));
                transition.state.resize(stateSize);
                file.read(reinterpret_cast<char*>(transition.state.data()), stateSize * sizeof(float));
                
                // Read action
                size_t actionSize;
                file.read(reinterpret_cast<char*>(&actionSize), sizeof(actionSize));
                transition.action.resize(actionSize);
                file.read(reinterpret_cast<char*>(transition.action.data()), actionSize * sizeof(float));
                
                // Read reward
                file.read(reinterpret_cast<char*>(&transition.reward), sizeof(transition.reward));
                
                // Read next state
                size_t nextStateSize;
                file.read(reinterpret_cast<char*>(&nextStateSize), sizeof(nextStateSize));
                transition.nextState.resize(nextStateSize);
                file.read(reinterpret_cast<char*>(transition.nextState.data()), nextStateSize * sizeof(float));
                
                // Read done flag and priority
                file.read(reinterpret_cast<char*>(&transition.done), sizeof(transition.done));
                file.read(reinterpret_cast<char*>(&transition.priority), sizeof(transition.priority));
                
                m_currentEpisode.push_back(transition);
                m_totalTransitions++;
            }
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error loading episodic replay buffer: " << e.what() << std::endl;
        return false;
    }
}

//=========================================================================
// ReplayBuffer implementation
//=========================================================================

ReplayBuffer::ReplayBuffer(const ReplayBufferConfig& config) {
    // Create the appropriate buffer implementation based on config
    switch (config.type) {
        case ReplayBufferType::PRIORITIZED:
            m_buffer = std::make_unique<PrioritizedReplayBuffer>(config.capacity, config.alpha, config.beta);
            break;
        case ReplayBufferType::EPISODIC:
            m_buffer = std::make_unique<EpisodicReplayBuffer>(config.capacity);
            break;
        case ReplayBufferType::UNIFORM:
        default:
            m_buffer = std::make_unique<UniformReplayBuffer>(config.capacity);
            break;
    }
    
    // Load from persistence path if provided
    if (!config.persistencePath.empty()) {
        load(config.persistencePath);
    }
}

ReplayBuffer::~ReplayBuffer() {
    // Unique_ptr will handle cleanup
}

void ReplayBuffer::add(const Transition& transition) {
    if (m_buffer) {
        m_buffer->add(transition);
    }
}

void ReplayBuffer::add(const AIInputFrame& state, const AIOutputAction& action, 
                    float reward, const AIInputFrame& nextState, bool done) {
    if (m_buffer) {
        m_buffer->add(state, action, reward, nextState, done);
    }
}

TransitionBatch ReplayBuffer::sample(size_t batchSize) {
    if (!m_buffer) {
        return TransitionBatch();
    }
    return m_buffer->sample(batchSize);
}

void ReplayBuffer::update_priorities(const std::vector<size_t>& indices, 
                                   const std::vector<float>& priorities) {
    if (m_buffer) {
        m_buffer->update_priorities(indices, priorities);
    }
}

size_t ReplayBuffer::size() const {
    if (!m_buffer) {
        return 0;
    }
    return m_buffer->size();
}

size_t ReplayBuffer::capacity() const {
    if (!m_buffer) {
        return 0;
    }
    return m_buffer->capacity();
}

void ReplayBuffer::clear() {
    if (m_buffer) {
        m_buffer->clear();
    }
}

bool ReplayBuffer::save(const std::string& path) {
    if (!m_buffer) {
        return false;
    }
    
    // If path is empty, don't save
    if (path.empty()) {
        return false;
    }
    
    return m_buffer->save(path);
}

bool ReplayBuffer::load(const std::string& path) {
    if (!m_buffer) {
        return false;
    }
    
    // If path is empty, don't load
    if (path.empty()) {
        return false;
    }
    
    return m_buffer->load(path);
}

void ReplayBuffer::end_episode() {
    // Only relevant for episodic buffer
    EpisodicReplayBuffer* episodicBuffer = 
        dynamic_cast<EpisodicReplayBuffer*>(m_buffer.get());
    
    if (episodicBuffer) {
        episodicBuffer->end_episode();
    }
}

std::vector<Transition> ReplayBuffer::sample_episode() {
    // Only relevant for episodic buffer
    EpisodicReplayBuffer* episodicBuffer = 
        dynamic_cast<EpisodicReplayBuffer*>(m_buffer.get());
    
    if (episodicBuffer) {
        return episodicBuffer->sample_episode();
    }
    
    return std::vector<Transition>();
}

ReplayBufferType ReplayBuffer::get_type() const {
    if (dynamic_cast<PrioritizedReplayBuffer*>(m_buffer.get())) {
        return ReplayBufferType::PRIORITIZED;
    } else if (dynamic_cast<EpisodicReplayBuffer*>(m_buffer.get())) {
        return ReplayBufferType::EPISODIC;
    } else {
        return ReplayBufferType::UNIFORM;
    }
}

void ReplayBuffer::set_beta(float beta) {
    // Only relevant for prioritized buffer
    PrioritizedReplayBuffer* prioritizedBuffer = 
        dynamic_cast<PrioritizedReplayBuffer*>(m_buffer.get());
    
    if (prioritizedBuffer) {
        prioritizedBuffer->set_beta(beta);
    }
}

//=========================================================================
// C API Implementation
//=========================================================================

extern "C" {

using ReplayBufferHandle = fbneo::ai::ReplayBuffer*;

FBNEO_ReplayBuffer FBNEO_ReplayBuffer_Create(int capacity, int bufferType, 
                                       float alpha, float beta, 
                                       const char* persistencePath) {
    try {
        // Create config
        fbneo::ai::ReplayBufferConfig config;
        config.capacity = static_cast<size_t>(capacity > 0 ? capacity : 100000);
        
        // Set buffer type
        switch (bufferType) {
            case 1:
                config.type = fbneo::ai::ReplayBufferType::PRIORITIZED;
                break;
            case 2:
                config.type = fbneo::ai::ReplayBufferType::EPISODIC;
                break;
            case 0:
            default:
                config.type = fbneo::ai::ReplayBufferType::UNIFORM;
                break;
        }
        
        // Set alpha and beta
        config.alpha = alpha;
        config.beta = beta;
        
        // Set persistence path
        if (persistencePath) {
            config.persistencePath = persistencePath;
        }
        
        // Create buffer
        return reinterpret_cast<FBNEO_ReplayBuffer>(new fbneo::ai::ReplayBuffer(config));
    } catch (const std::exception& e) {
        std::cerr << "Error creating replay buffer: " << e.what() << std::endl;
        return nullptr;
    }
}

void FBNEO_ReplayBuffer_Destroy(FBNEO_ReplayBuffer handle) {
    if (handle) {
        delete reinterpret_cast<ReplayBufferHandle>(handle);
    }
}

int FBNEO_ReplayBuffer_Add(FBNEO_ReplayBuffer handle, 
                      void* state, int stateSize, 
                      void* action, int actionSize, 
                      float reward, 
                      void* nextState, int nextStateSize, 
                      int done) {
    if (!handle || !state || !action || !nextState) {
        return 0;
    }
    
    try {
        ReplayBufferHandle buffer = reinterpret_cast<ReplayBufferHandle>(handle);
        
        // Create input frame from state
        AIInputFrame stateFrame;
        stateFrame.setFrameData(state, stateSize);
        
        // Create output action from action
        AIOutputAction outputAction;
        outputAction.setActionData(static_cast<float*>(action), actionSize);
        
        // Create input frame from next state
        AIInputFrame nextStateFrame;
        nextStateFrame.setFrameData(nextState, nextStateSize);
        
        // Add transition to buffer
        buffer->add(stateFrame, outputAction, reward, nextStateFrame, done != 0);
        
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error adding to replay buffer: " << e.what() << std::endl;
        return 0;
    }
}

int FBNEO_ReplayBuffer_Sample(FBNEO_ReplayBuffer handle, int batchSize,
                         void* statesOut, void* actionsOut, 
                         float* rewardsOut, void* nextStatesOut, 
                         int* donesOut, float* weightsOut, 
                         int* indicesOut) {
    if (!handle || batchSize <= 0) {
        return 0;
    }
    
    try {
        ReplayBufferHandle buffer = reinterpret_cast<ReplayBufferHandle>(handle);
        
        // Sample batch from buffer
        TransitionBatch batch = buffer->sample(static_cast<size_t>(batchSize));
        
        // Copy data to output buffers if provided
        size_t sampleSize = batch.size();
        if (sampleSize == 0) {
            return 0;
        }
        
        // Copy states
        if (statesOut && !batch.states.empty()) {
            float* statesPtr = static_cast<float*>(statesOut);
            for (size_t i = 0; i < sampleSize; ++i) {
                const auto& state = batch.states[i];
                size_t stateSize = state.size();
                if (stateSize > 0) {
                    std::memcpy(statesPtr, state.data(), stateSize * sizeof(float));
                    statesPtr += stateSize;
                }
            }
        }
        
        // Copy actions
        if (actionsOut && !batch.actions.empty()) {
            float* actionsPtr = static_cast<float*>(actionsOut);
            for (size_t i = 0; i < sampleSize; ++i) {
                const auto& action = batch.actions[i];
                size_t actionSize = action.size();
                if (actionSize > 0) {
                    std::memcpy(actionsPtr, action.data(), actionSize * sizeof(float));
                    actionsPtr += actionSize;
                }
            }
        }
        
        // Copy rewards
        if (rewardsOut && !batch.rewards.empty()) {
            std::memcpy(rewardsOut, batch.rewards.data(), sampleSize * sizeof(float));
        }
        
        // Copy next states
        if (nextStatesOut && !batch.nextStates.empty()) {
            float* nextStatesPtr = static_cast<float*>(nextStatesOut);
            for (size_t i = 0; i < sampleSize; ++i) {
                const auto& nextState = batch.nextStates[i];
                size_t nextStateSize = nextState.size();
                if (nextStateSize > 0) {
                    std::memcpy(nextStatesPtr, nextState.data(), nextStateSize * sizeof(float));
                    nextStatesPtr += nextStateSize;
                }
            }
        }
        
        // Copy dones
        if (donesOut && !batch.dones.empty()) {
            for (size_t i = 0; i < sampleSize; ++i) {
                donesOut[i] = batch.dones[i] ? 1 : 0;
            }
        }
        
        // Copy weights
        if (weightsOut && !batch.weights.empty()) {
            std::memcpy(weightsOut, batch.weights.data(), sampleSize * sizeof(float));
        }
        
        // Copy indices
        if (indicesOut && !batch.indices.empty()) {
            for (size_t i = 0; i < sampleSize; ++i) {
                indicesOut[i] = static_cast<int>(batch.indices[i]);
            }
        }
        
        return static_cast<int>(sampleSize);
    } catch (const std::exception& e) {
        std::cerr << "Error sampling from replay buffer: " << e.what() << std::endl;
        return 0;
    }
}

void FBNEO_ReplayBuffer_UpdatePriorities(FBNEO_ReplayBuffer handle,
                                   const int* indices, int indicesCount,
                                   const float* priorities) {
    if (!handle || !indices || !priorities || indicesCount <= 0) {
        return;
    }
    
    try {
        ReplayBufferHandle buffer = reinterpret_cast<ReplayBufferHandle>(handle);
        
        // Convert indices to size_t
        std::vector<size_t> indicesVec(indicesCount);
        for (int i = 0; i < indicesCount; ++i) {
            indicesVec[i] = static_cast<size_t>(indices[i]);
        }
        
        // Copy priorities
        std::vector<float> prioritiesVec(priorities, priorities + indicesCount);
        
        // Update priorities
        buffer->update_priorities(indicesVec, prioritiesVec);
    } catch (const std::exception& e) {
        std::cerr << "Error updating priorities: " << e.what() << std::endl;
    }
}

int FBNEO_ReplayBuffer_Size(FBNEO_ReplayBuffer handle) {
    if (!handle) {
        return 0;
    }
    return static_cast<int>(reinterpret_cast<ReplayBufferHandle>(handle)->size());
}

int FBNEO_ReplayBuffer_Capacity(FBNEO_ReplayBuffer handle) {
    if (!handle) {
        return 0;
    }
    return static_cast<int>(reinterpret_cast<ReplayBufferHandle>(handle)->capacity());
}

void FBNEO_ReplayBuffer_Clear(FBNEO_ReplayBuffer handle) {
    if (handle) {
        reinterpret_cast<ReplayBufferHandle>(handle)->clear();
    }
}

int FBNEO_ReplayBuffer_Save(FBNEO_ReplayBuffer handle, const char* path) {
    if (!handle || !path) {
        return 0;
    }
    return reinterpret_cast<ReplayBufferHandle>(handle)->save(path) ? 1 : 0;
}

int FBNEO_ReplayBuffer_Load(FBNEO_ReplayBuffer handle, const char* path) {
    if (!handle || !path) {
        return 0;
    }
    return reinterpret_cast<ReplayBufferHandle>(handle)->load(path) ? 1 : 0;
}

void FBNEO_ReplayBuffer_EndEpisode(FBNEO_ReplayBuffer handle) {
    if (handle) {
        reinterpret_cast<ReplayBufferHandle>(handle)->end_episode();
    }
}

} // extern "C"

} // namespace ai
} // namespace fbneo 