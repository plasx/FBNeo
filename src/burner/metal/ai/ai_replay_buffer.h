#pragma once

#include "ai_input_frame.h"
#include "ai_output_action.h"
#include <vector>
#include <deque>
#include <string>
#include <random>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace fbneo {
namespace ai {

/**
 * @brief Structure to represent a single transition in the replay buffer
 */
struct Transition {
    AIInputFrame state;            // Current state
    AIOutputAction action;         // Action taken
    float reward;                  // Reward received
    AIInputFrame nextState;        // Next state
    bool done;                     // Whether this is a terminal state
    float priority;                // Priority for prioritized experience replay
    
    Transition() : reward(0.0f), done(false), priority(1.0f) {}
    
    Transition(const AIInputFrame& s, const AIOutputAction& a, float r, 
              const AIInputFrame& ns, bool d) 
        : state(s), action(a), reward(r), nextState(ns), done(d), priority(1.0f) {}
};

/**
 * @brief Structure for a batch of transitions
 */
struct TransitionBatch {
    std::vector<AIInputFrame> states;
    std::vector<AIOutputAction> actions;
    std::vector<float> rewards;
    std::vector<AIInputFrame> nextStates;
    std::vector<bool> dones;
    std::vector<float> weights;  // Importance sampling weights
    std::vector<size_t> indices; // Indices in the buffer
    
    size_t size() const { return states.size(); }
    void clear() {
        states.clear();
        actions.clear();
        rewards.clear();
        nextStates.clear();
        dones.clear();
        weights.clear();
        indices.clear();
    }
    
    void reserve(size_t capacity) {
        states.reserve(capacity);
        actions.reserve(capacity);
        rewards.reserve(capacity);
        nextStates.reserve(capacity);
        dones.reserve(capacity);
        weights.reserve(capacity);
        indices.reserve(capacity);
    }
};

/**
 * @brief Types of replay buffer algorithms
 */
enum class ReplayBufferType {
    UNIFORM,        // Standard uniform sampling
    PRIORITIZED,    // Prioritized experience replay
    HINDSIGHT,      // Hindsight experience replay
    EPISODIC        // Full episode storage
};

/**
 * @brief Configuration for replay buffer
 */
struct ReplayBufferConfig {
    size_t capacity;                // Maximum capacity of the buffer
    ReplayBufferType type;          // Type of replay buffer
    float alpha;                    // Priority exponent (for prioritized replay)
    float beta;                     // Importance sampling exponent (for prioritized replay)
    bool useCuda;                   // Whether to store data on GPU
    std::string persistencePath;    // Path to save/load the buffer from disk
    
    ReplayBufferConfig()
        : capacity(100000),
          type(ReplayBufferType::UNIFORM),
          alpha(0.6f),
          beta(0.4f),
          useCuda(false),
          persistencePath("") {}
};

/**
 * @brief Interface for replay buffer implementations
 */
class IReplayBuffer {
public:
    virtual ~IReplayBuffer() = default;
    
    virtual void add(const Transition& transition) = 0;
    virtual void add(const AIInputFrame& state, const AIOutputAction& action, 
                    float reward, const AIInputFrame& nextState, bool done) = 0;
    virtual TransitionBatch sample(size_t batchSize) = 0;
    virtual void update_priorities(const std::vector<size_t>& indices, 
                                 const std::vector<float>& priorities) = 0;
    virtual size_t size() const = 0;
    virtual size_t capacity() const = 0;
    virtual void clear() = 0;
    virtual bool save(const std::string& path) = 0;
    virtual bool load(const std::string& path) = 0;
};

/**
 * @brief Uniform sampling replay buffer implementation
 */
class UniformReplayBuffer : public IReplayBuffer {
public:
    explicit UniformReplayBuffer(size_t capacity);
    virtual ~UniformReplayBuffer();
    
    void add(const Transition& transition) override;
    void add(const AIInputFrame& state, const AIOutputAction& action, 
            float reward, const AIInputFrame& nextState, bool done) override;
    TransitionBatch sample(size_t batchSize) override;
    void update_priorities(const std::vector<size_t>& indices, 
                         const std::vector<float>& priorities) override;
    size_t size() const override;
    size_t capacity() const override;
    void clear() override;
    bool save(const std::string& path) override;
    bool load(const std::string& path) override;
    
private:
    std::deque<Transition> m_buffer;
    size_t m_capacity;
    std::mt19937 m_rng;
    std::mutex m_mutex;
};

/**
 * @brief Prioritized experience replay buffer implementation
 */
class PrioritizedReplayBuffer : public IReplayBuffer {
public:
    PrioritizedReplayBuffer(size_t capacity, float alpha = 0.6f, float beta = 0.4f);
    virtual ~PrioritizedReplayBuffer();
    
    void add(const Transition& transition) override;
    void add(const AIInputFrame& state, const AIOutputAction& action, 
            float reward, const AIInputFrame& nextState, bool done) override;
    TransitionBatch sample(size_t batchSize) override;
    void update_priorities(const std::vector<size_t>& indices, 
                         const std::vector<float>& priorities) override;
    size_t size() const override;
    size_t capacity() const override;
    void clear() override;
    bool save(const std::string& path) override;
    bool load(const std::string& path) override;
    
    void set_beta(float beta);
    float get_max_priority() const;
    
private:
    struct SumTree {
        std::vector<float> tree;
        std::vector<size_t> data_indices;
        size_t capacity;
        size_t data_size;
        
        SumTree(size_t capacity);
        void update(size_t idx, float priority);
        float sum() const;
        std::pair<size_t, float> get(float value) const;
    };
    
    std::vector<Transition> m_buffer;
    SumTree m_sumTree;
    size_t m_position;
    size_t m_capacity;
    float m_alpha;
    float m_beta;
    float m_maxPriority;
    std::mt19937 m_rng;
    std::mutex m_mutex;
};

/**
 * @brief Episodic replay buffer for storing complete game episodes
 */
class EpisodicReplayBuffer : public IReplayBuffer {
public:
    explicit EpisodicReplayBuffer(size_t capacity);
    virtual ~EpisodicReplayBuffer();
    
    void add(const Transition& transition) override;
    void add(const AIInputFrame& state, const AIOutputAction& action, 
            float reward, const AIInputFrame& nextState, bool done) override;
    TransitionBatch sample(size_t batchSize) override;
    void update_priorities(const std::vector<size_t>& indices, 
                         const std::vector<float>& priorities) override;
    size_t size() const override;
    size_t capacity() const override;
    void clear() override;
    bool save(const std::string& path) override;
    bool load(const std::string& path) override;
    
    void end_episode();
    std::vector<Transition> sample_episode();
    
private:
    std::vector<std::vector<Transition>> m_episodes;
    std::vector<Transition> m_currentEpisode;
    size_t m_capacity; // Maximum number of episodes
    size_t m_totalTransitions;
    std::mt19937 m_rng;
    std::mutex m_mutex;
};

/**
 * @brief Main replay buffer class that manages multiple buffer implementations
 */
class ReplayBuffer {
public:
    explicit ReplayBuffer(const ReplayBufferConfig& config = ReplayBufferConfig());
    ~ReplayBuffer();
    
    void add(const Transition& transition);
    void add(const AIInputFrame& state, const AIOutputAction& action, 
            float reward, const AIInputFrame& nextState, bool done);
    TransitionBatch sample(size_t batchSize);
    void update_priorities(const std::vector<size_t>& indices, 
                         const std::vector<float>& priorities);
    size_t size() const;
    size_t capacity() const;
    void clear();
    bool save(const std::string& path = "");
    bool load(const std::string& path = "");
    
    void end_episode();
    std::vector<Transition> sample_episode();
    ReplayBufferType get_type() const;
    void set_beta(float beta);
    
private:
    std::unique_ptr<IReplayBuffer> m_buffer;
    ReplayBufferConfig m_config;
};

// C API for integration with non-C++ code
extern "C" {
    // Opaque handle for C API
    typedef void* FBNEO_ReplayBuffer;
    
    // Create a replay buffer
    FBNEO_ReplayBuffer FBNEO_ReplayBuffer_Create(int capacity, int bufferType, 
                                               float alpha, float beta, 
                                               const char* persistencePath);
    
    // Destroy a replay buffer
    void FBNEO_ReplayBuffer_Destroy(FBNEO_ReplayBuffer handle);
    
    // Add a transition to the buffer
    int FBNEO_ReplayBuffer_Add(FBNEO_ReplayBuffer handle, 
                              void* state, int stateSize, 
                              void* action, int actionSize, 
                              float reward, 
                              void* nextState, int nextStateSize, 
                              int done);
    
    // Sample a batch of transitions
    int FBNEO_ReplayBuffer_Sample(FBNEO_ReplayBuffer handle, int batchSize,
                                 void* statesOut, void* actionsOut, 
                                 float* rewardsOut, void* nextStatesOut, 
                                 int* donesOut, float* weightsOut, 
                                 int* indicesOut);
    
    // Update priorities
    void FBNEO_ReplayBuffer_UpdatePriorities(FBNEO_ReplayBuffer handle,
                                           const int* indices, int indicesCount,
                                           const float* priorities);
    
    // Get buffer size
    int FBNEO_ReplayBuffer_Size(FBNEO_ReplayBuffer handle);
    
    // Get buffer capacity
    int FBNEO_ReplayBuffer_Capacity(FBNEO_ReplayBuffer handle);
    
    // Clear the buffer
    void FBNEO_ReplayBuffer_Clear(FBNEO_ReplayBuffer handle);
    
    // Save buffer to disk
    int FBNEO_ReplayBuffer_Save(FBNEO_ReplayBuffer handle, const char* path);
    
    // Load buffer from disk
    int FBNEO_ReplayBuffer_Load(FBNEO_ReplayBuffer handle, const char* path);
    
    // End episode (for episodic buffer)
    void FBNEO_ReplayBuffer_EndEpisode(FBNEO_ReplayBuffer handle);
}

} // namespace ai
} // namespace fbneo 