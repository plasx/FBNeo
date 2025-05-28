#pragma once

#include <vector>
#include <memory>
#include <string>
#include <random>
#include <deque>
#include <unordered_map>
#include <functional>

// Forward declarations for PyTorch C++ API
namespace torch {
    class Tensor;
    
    namespace nn {
        class Module;
    }
    
    namespace optim {
        class Optimizer;
    }
}

// AI definitions
#include "ai_definitions.h"
#include "ai_input_frame.h"
#include "ai_output_action.h"

namespace fbneo {
namespace ai {

// PPO hyperparameters structure
struct PPOHyperparameters {
    float learning_rate;           // Learning rate for optimizer
    float value_coef;              // Value loss coefficient
    float entropy_coef;            // Entropy coefficient
    float clip_epsilon;            // PPO clipping parameter
    float discount_factor;         // Reward discount factor (gamma)
    float gae_lambda;              // GAE lambda parameter
    int batch_size;                // Batch size for training
    int epochs;                    // Number of epochs per training update
    int sequence_length;           // Sequence length for RNN (if used)
    bool use_gae;                  // Whether to use Generalized Advantage Estimation
    bool normalize_advantages;     // Whether to normalize advantages
    
    // Constructor with default values
    PPOHyperparameters() :
        learning_rate(0.0003f),
        value_coef(0.5f),
        entropy_coef(0.01f),
        clip_epsilon(0.2f),
        discount_factor(0.99f),
        gae_lambda(0.95f),
        batch_size(64),
        epochs(4),
        sequence_length(128),
        use_gae(true),
        normalize_advantages(true) {}
};

// PPO experience transition structure (for experience collection)
struct PPOTransition {
    std::vector<float> state;        // State features
    std::vector<float> action;       // Action taken
    float reward;                    // Reward received
    float value;                     // Value estimate
    float log_prob;                  // Log probability of action
    bool done;                       // Terminal state flag
    std::vector<float> next_state;   // Next state features
    
    PPOTransition() : reward(0.0f), value(0.0f), log_prob(0.0f), done(false) {}
};

// PPO buffer to store experience for training
class PPOBuffer {
public:
    // Construction and initialization
    PPOBuffer(int capacity);
    ~PPOBuffer();
    
    // Buffer management
    void add(const PPOTransition& transition);
    void clear();
    bool is_ready(int batch_size) const;
    int size() const;
    
    // Sample batch for training
    void prepare_batch(const PPOHyperparameters& params);
    
    // Get tensors for training
    torch::Tensor get_states() const;
    torch::Tensor get_actions() const;
    torch::Tensor get_old_log_probs() const;
    torch::Tensor get_returns() const;
    torch::Tensor get_advantages() const;
    
private:
    // Experience storage
    std::deque<PPOTransition> transitions_;
    int capacity_;
    
    // Computed values for training
    std::vector<torch::Tensor> states_;
    std::vector<torch::Tensor> actions_;
    std::vector<torch::Tensor> old_log_probs_;
    std::vector<torch::Tensor> returns_;
    std::vector<torch::Tensor> advantages_;
    
    // Compute returns and advantages for batch
    void compute_returns_and_advantages(const PPOHyperparameters& params);
};

// PPO algorithm implementation
class PPOAgent {
public:
    // Construction and initialization
    PPOAgent();
    ~PPOAgent();
    
    // Initialization
    bool initialize(const PPOHyperparameters& params);
    
    // Experience collection
    AIOutputAction select_action(const AIInputFrame& state, bool deterministic = false);
    void observe_reward(float reward, bool done);
    
    // Training methods
    bool train(int num_updates);
    bool update_policy();
    
    // Model management
    bool save(const std::string& path);
    bool load(const std::string& path);
    
    // Properties
    void set_hyperparameters(const PPOHyperparameters& params);
    const PPOHyperparameters& get_hyperparameters() const;
    
private:
    // Model components
    std::shared_ptr<torch::nn::Module> policy_network_;
    std::shared_ptr<torch::nn::Module> value_network_;
    std::shared_ptr<torch::optim::Optimizer> policy_optimizer_;
    std::shared_ptr<torch::optim::Optimizer> value_optimizer_;
    
    // Experience buffer
    std::unique_ptr<PPOBuffer> buffer_;
    
    // Hyperparameters
    PPOHyperparameters hyperparameters_;
    
    // State tracking
    std::vector<float> last_state_;
    std::vector<float> last_action_;
    float last_log_prob_;
    float last_value_;
    
    // RNG for exploration
    std::mt19937 rng_;
    
    // Convert between game state and network input format
    std::vector<float> convert_state_to_features(const AIInputFrame& state);
    AIOutputAction convert_network_output_to_action(const torch::Tensor& action_tensor);
    
    // Utility methods
    torch::Tensor evaluate_actions(const torch::Tensor& states, const torch::Tensor& actions);
    void reset_experience();
};

// PPO algorithm initialization function
bool PPO_Initialize(const PPOHyperparameters& params);

// Train the PPO algorithm with the current game state
bool PPO_Train(int num_updates);

// Update the policy based on collected experience
bool PPO_UpdatePolicy();

// Get the PPO agent instance
PPOAgent* GetPPOAgent();

} // namespace ai
} // namespace fbneo 