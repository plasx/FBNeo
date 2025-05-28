# Reinforcement Learning Training Pipeline

This document outlines the reinforcement learning (RL) training pipeline for the FBNeo AI implementation.

## Configuration

```json
{
  "algorithm": "ppo",
  "model": {
    "hidden_layers": [128, 128],
    "activation": "relu",
    "learning_rate": 0.0003
  },
  "training": {
    "gamma": 0.99,
    "lambda": 0.95,
    "clip_ratio": 0.2,
    "epochs": 10,
    "batch_size": 64,
    "max_episodes": 10000,
    "target_kl": 0.01
  },
  "environment": {
    "game": "sfiii3n",
    "opponent_difficulty": 3,
    "episodes_per_update": 20,
    "reward_shaping": {
      "win": 1.0,
      "damage_dealt": 0.01,
      "damage_received": -0.01,
      "time_penalty": -0.001
    }
  }
}
```

## Model Architecture

```
InputLayer (size=AIInputFrame features) 
    |
    v
DenseLayer (128 units, ReLU)
    |
    v
DenseLayer (128 units, ReLU)
    |
    v
[Policy Head]                [Value Head]
DenseLayer (128 units)       DenseLayer (128 units)
    |                            |
    v                            v
SoftmaxLayer (action_size)   LinearLayer (1)
```

## Training Loop

```python
def train_ppo(config):
    # Initialize model and environment
    model = PolicyValueNetwork(config)
    simulator = FBNeoSimulator(config["environment"]["game"])
    
    # PPO buffers
    observations = []
    actions = []
    rewards = []
    values = []
    dones = []
    log_probs = []
    
    # Training loop
    for episode in range(config["training"]["max_episodes"]):
        # Collect experiences using current policy
        collect_episodes(
            model, 
            simulator, 
            config["environment"]["episodes_per_update"],
            observations, actions, rewards, values, dones, log_probs
        )
        
        # Compute advantages and returns
        advantages, returns = compute_gae(
            rewards, values, dones,
            config["training"]["gamma"],
            config["training"]["lambda"]
        )
        
        # Update policy and value function
        for epoch in range(config["training"]["epochs"]):
            # Generate mini-batches
            batch_indices = generate_mini_batches(
                len(observations),
                config["training"]["batch_size"]
            )
            
            # Update on each mini-batch
            for indices in batch_indices:
                batch_obs = [observations[i] for i in indices]
                batch_acts = [actions[i] for i in indices]
                batch_advs = [advantages[i] for i in indices]
                batch_rets = [returns[i] for i in indices]
                batch_logps = [log_probs[i] for i in indices]
                
                # Policy loss, value loss, entropy loss
                policy_loss, value_loss, entropy = update_model(
                    model,
                    batch_obs, batch_acts, batch_advs, 
                    batch_rets, batch_logps,
                    config["training"]["clip_ratio"]
                )
                
                # Check KL divergence for early stopping
                if kl > config["training"]["target_kl"]:
                    break
        
        # Evaluate policy
        eval_score = evaluate_policy(model, simulator)
        
        # Save model checkpoint
        if episode % 100 == 0:
            save_checkpoint(model, f"checkpoint_{episode}.pt")
    
    # Export final model
    export_model(model, "final_model.pt")
    export_to_coreml(model, "final_model.mlmodel")
```

## Implementation Plan

1. **Environment Interface**
   - Create a Python wrapper for FBNeo headless mode
   - Implement OpenAI Gym-compatible API
   - Support parallel environment instances

2. **Neural Network Implementation**
   - Implement policy-value network in PyTorch
   - Add support for different activation functions
   - Create proper input/output interfaces

3. **Algorithm Implementation**
   - Implement PPO with clipped objective
   - Add support for GAE advantage estimation
   - Implement entropy bonus for exploration

4. **Reward Shaping**
   - Design reward functions for fighting games
   - Support customizable reward components
   - Implement curriculum learning

5. **Distributed Training**
   - Support multi-GPU data parallelism
   - Implement asynchronous rollouts
   - Add checkpointing and recovery

## Reward Function

```cpp
float calculateReward(const AIInputFrame& prev, const AIInputFrame& current, const Config& config) {
    float reward = 0.0f;
    
    // Damage dealt reward
    float damage_dealt = prev.p2_health - current.p2_health;
    if (damage_dealt > 0) {
        reward += damage_dealt * config.reward_shaping.damage_dealt;
    }
    
    // Damage received penalty
    float damage_received = prev.p1_health - current.p1_health;
    if (damage_received > 0) {
        reward += damage_received * config.reward_shaping.damage_received;
    }
    
    // Win reward
    if (current.p2_health <= 0 && current.p1_health > 0) {
        reward += config.reward_shaping.win;
    }
    
    // Loss penalty
    if (current.p1_health <= 0 && current.p2_health > 0) {
        reward -= config.reward_shaping.win;
    }
    
    // Time penalty
    reward += config.reward_shaping.time_penalty;
    
    return reward;
}
```

## Model Compression

For deployment on mobile devices, the following compression techniques will be applied:

1. **Quantization**
   - Convert FP32 weights to INT8
   - Calibrate using representative game data
   - Measure accuracy impact

2. **Pruning**
   - Remove neurons with small weights
   - Fine-tune after pruning
   - Target 50-70% model size reduction

3. **Knowledge Distillation**
   - Train smaller student model from larger teacher
   - Match logits rather than just actions
   - Balance accuracy vs. inference speed

## Testing and Evaluation

The AI system will be evaluated using:

1. **Win Rate Metrics**
   - Win percentage against game AI at various difficulties
   - Win percentage against previous model versions
   - Win percentage against human players (optional)

2. **Performance Metrics**
   - Average frames per second during inference
   - Memory usage
   - CPU/GPU utilization
   - Model load time

3. **Behavior Analysis**
   - Action diversity analysis
   - Combo frequency and effectiveness
   - Defensive capability measurement
   - Adaptation to opponent patterns 