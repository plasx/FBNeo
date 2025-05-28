#!/usr/bin/env python3
"""
Training Pipeline for FBNeo AI

This module implements a training pipeline for reinforcement learning with
Proximal Policy Optimization (PPO) and Intrinsic Curiosity Module (ICM).
"""

import os
import sys
import json
import time
import logging
import threading
import torch
import torch.nn as nn
import torch.nn.functional as F
import torch.optim as optim
import numpy as np
from collections import deque
from typing import Dict, List, Tuple, Any, Optional, Union

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger("TrainingPipeline")

class PPONetwork(nn.Module):
    """
    PPO Network with Actor and Critic heads
    """
    def __init__(self, input_dim: int, hidden_dim: int, num_actions: int):
        super(PPONetwork, self).__init__()
        
        # Shared feature extractor
        self.feature_extractor = nn.Sequential(
            nn.Linear(input_dim, hidden_dim),
            nn.ReLU(),
            nn.Linear(hidden_dim, hidden_dim),
            nn.ReLU()
        )
        
        # Policy head (actor)
        self.policy_head = nn.Sequential(
            nn.Linear(hidden_dim, hidden_dim // 2),
            nn.ReLU(),
            nn.Linear(hidden_dim // 2, num_actions)
        )
        
        # Value head (critic)
        self.value_head = nn.Sequential(
            nn.Linear(hidden_dim, hidden_dim // 2),
            nn.ReLU(),
            nn.Linear(hidden_dim // 2, 1)
        )
    
    def forward(self, x: torch.Tensor) -> Tuple[torch.Tensor, torch.Tensor]:
        """
        Forward pass through the network
        
        Args:
            x: Input state tensor
            
        Returns:
            Tuple of (action_logits, value)
        """
        features = self.feature_extractor(x)
        action_logits = self.policy_head(features)
        value = self.value_head(features)
        
        return action_logits, value
    
    def get_action_and_value(self, state: torch.Tensor, 
                             deterministic: bool = False) -> Tuple[torch.Tensor, torch.Tensor, torch.Tensor, torch.Tensor]:
        """
        Get action and value for a given state
        
        Args:
            state: Input state tensor
            deterministic: If True, use argmax action selection
            
        Returns:
            Tuple of (action, log_prob, entropy, value)
        """
        action_logits, value = self(state)
        
        # Calculate action distribution
        action_probs = F.softmax(action_logits, dim=-1)
        action_dist = torch.distributions.Categorical(action_probs)
        
        # Sample action
        if deterministic:
            action = torch.argmax(action_probs, dim=-1)
        else:
            action = action_dist.sample()
        
        # Calculate log probability and entropy
        action_log_prob = action_dist.log_prob(action)
        entropy = action_dist.entropy()
        
        return action, action_log_prob, entropy, value

class ICMNetwork(nn.Module):
    """
    Intrinsic Curiosity Module (ICM)
    
    Predicts next state features and actions for calculating intrinsic rewards.
    """
    def __init__(self, input_dim: int, hidden_dim: int, num_actions: int, feature_dim: int = 256):
        super(ICMNetwork, self).__init__()
        
        # Feature extractor
        self.feature_extractor = nn.Sequential(
            nn.Linear(input_dim, hidden_dim),
            nn.ReLU(),
            nn.Linear(hidden_dim, feature_dim)
        )
        
        # Inverse model: predicts action from states
        self.inverse_model = nn.Sequential(
            nn.Linear(feature_dim * 2, hidden_dim),
            nn.ReLU(),
            nn.Linear(hidden_dim, num_actions)
        )
        
        # Forward model: predicts next state features
        self.forward_model = nn.Sequential(
            nn.Linear(feature_dim + num_actions, hidden_dim),
            nn.ReLU(),
            nn.Linear(hidden_dim, feature_dim)
        )
        
        self.feature_dim = feature_dim
        self.num_actions = num_actions
    
    def forward(self, state: torch.Tensor, next_state: torch.Tensor, 
                action: torch.Tensor) -> Tuple[torch.Tensor, torch.Tensor, torch.Tensor]:
        """
        Forward pass through ICM
        
        Args:
            state: Current state tensor
            next_state: Next state tensor
            action: Action tensor
            
        Returns:
            Tuple of (action_logits, predicted_features, next_state_features)
        """
        # Extract features from current and next states
        state_features = self.feature_extractor(state)
        next_state_features = self.feature_extractor(next_state)
        
        # Inverse model: predict action
        inverse_input = torch.cat([state_features, next_state_features], dim=1)
        action_logits = self.inverse_model(inverse_input)
        
        # One-hot encode action for forward model
        action_one_hot = F.one_hot(action.long(), self.num_actions).float()
        
        # Forward model: predict next state features
        forward_input = torch.cat([state_features, action_one_hot], dim=1)
        predicted_features = self.forward_model(forward_input)
        
        return action_logits, predicted_features, next_state_features

class PPOTrainer:
    """
    PPO Trainer with ICM
    """
    def __init__(self, config: Dict[str, Any]):
        """
        Initialize the PPO trainer
        
        Args:
            config: Configuration dictionary
        """
        self.config = config
        
        # PPO hyperparameters
        self.gamma = config.get("gamma", 0.99)
        self.gae_lambda = config.get("gae_lambda", 0.95)
        self.clip_ratio = config.get("clip_ratio", 0.2)
        self.value_coef = config.get("value_coef", 0.5)
        self.entropy_coef = config.get("entropy_coef", 0.01)
        self.max_grad_norm = config.get("max_grad_norm", 0.5)
        self.ppo_epochs = config.get("ppo_epochs", 4)
        self.batch_size = config.get("batch_size", 64)
        
        # ICM hyperparameters
        self.use_icm = config.get("use_icm", True)
        self.icm_reward_scale = config.get("icm_reward_scale", 0.01)
        self.forward_loss_weight = config.get("forward_loss_weight", 0.2)
        
        # Experience buffer
        self.buffer = []
        
        # Initialize the headless runner in the distributed worker
        self._initialize_headless_runner()
    
    def _initialize_headless_runner(self):
        """Initialize the headless runner for experience collection"""
        try:
            # This will be implemented in the distributed worker
            from src.burner.metal.ai.headless_manager import HeadlessManager
            from src.ai.ai_memory_mapping import AIMemoryMapping
            
            # Initialize the headless manager
            self.headless_manager = HeadlessManager.getInstance()
            self.headless_manager.initialize(self.config.get("num_parallel_envs", 1))
            
            # Initialize memory mapping
            self.memory_mapping = AIMemoryMapping()
            self.memory_mapping.initialize(self.config.get("game_id", "sf3"))
            
            logger.info("Headless runner initialized")
            
        except Exception as e:
            logger.warning(f"Could not initialize headless runner: {e}")
            self.headless_manager = None
            self.memory_mapping = None
    
    def collect_experiences(self, model: nn.Module, workload: float = 1.0) -> List[Dict[str, Any]]:
        """
        Collect experiences using the headless runner
        
        Args:
            model: Policy model for making decisions
            workload: Fraction of maximum workload to process (0.0-1.0)
            
        Returns:
            List of experience dictionaries
        """
        if self.headless_manager is None:
            # If no headless runner, return empty experiences
            return []
        
        # Calculate number of episodes to run based on workload
        max_episodes = self.config.get("episodes_per_batch", 10)
        num_episodes = max(1, int(max_episodes * workload))
        
        experiences = []
        
        # Set up callbacks for the headless runner
        episodes_complete = 0
        current_state = None
        current_log_prob = None
        current_value = None
        current_action = None
        
        # Set frame callback to extract state
        def frame_callback(buffer, width, height, stride):
            nonlocal current_state
            
            # Extract state from memory mapping
            self.memory_mapping.refreshValues()
            
            # Convert to input features
            # This would be implemented to extract features from the memory mapping
            # For now, just create a random state
            state = torch.rand(1, self.config.get("input_dim", 128))
            current_state = state
            
            # Get action from model
            with torch.no_grad():
                action, log_prob, _, value = model.get_action_and_value(state)
                
            current_log_prob = log_prob.item()
            current_value = value.item()
            current_action = action.item()
            
            # Return action to game
            return current_action
        
        # Set episode complete callback to gather experiences
        def episode_complete_callback(episode_id, total_reward):
            nonlocal episodes_complete
            
            # Add experience to buffer
            experiences.append({
                "state": current_state,
                "action": current_action,
                "reward": total_reward,
                "log_prob": current_log_prob,
                "value": current_value
            })
            
            episodes_complete += 1
        
        # Configure headless runner
        runner = self.headless_manager.getInstance(0)  # Get first instance
        runner.setFrameCallback(frame_callback)
        runner.setEpisodeCompleteCallback(episode_complete_callback)
        
        # Run episodes
        runner.runEpisodes(num_episodes)
        
        return experiences
    
    def compute_gradients(self, model: nn.Module, experiences: List[Dict[str, Any]]) -> Dict[str, torch.Tensor]:
        """
        Compute gradients from collected experiences
        
        Args:
            model: Policy model
            experiences: List of experience dictionaries
            
        Returns:
            Dictionary of parameter gradients
        """
        if not experiences:
            return {}
        
        # Prepare tensors from experiences
        states = torch.cat([exp["state"] for exp in experiences])
        actions = torch.tensor([exp["action"] for exp in experiences])
        rewards = torch.tensor([exp["reward"] for exp in experiences])
        old_log_probs = torch.tensor([exp["log_prob"] for exp in experiences])
        old_values = torch.tensor([exp["value"] for exp in experiences])
        
        # Calculate advantages
        advantages = rewards - old_values
        
        # Normalize advantages
        advantages = (advantages - advantages.mean()) / (advantages.std() + 1e-8)
        
        # Calculate returns
        returns = rewards
        
        # Create optimizer
        optimizer = optim.Adam(model.parameters(), lr=self.config.get("lr", 3e-4))
        
        # Zero gradients
        optimizer.zero_grad()
        
        # PPO update
        for _ in range(self.ppo_epochs):
            # Get indices for mini-batches
            indices = torch.randperm(states.size(0))
            
            # Process mini-batches
            for start_idx in range(0, states.size(0), self.batch_size):
                end_idx = min(start_idx + self.batch_size, states.size(0))
                batch_indices = indices[start_idx:end_idx]
                
                # Get current predictions
                batch_states = states[batch_indices]
                batch_actions = actions[batch_indices]
                batch_old_log_probs = old_log_probs[batch_indices]
                batch_advantages = advantages[batch_indices]
                batch_returns = returns[batch_indices]
                
                # Forward pass
                _, new_log_probs, entropy, new_values = model.get_action_and_value(
                    batch_states, batch_actions)
                
                # Policy loss
                ratio = torch.exp(new_log_probs - batch_old_log_probs)
                surr1 = ratio * batch_advantages
                surr2 = torch.clamp(ratio, 1.0 - self.clip_ratio, 1.0 + self.clip_ratio) * batch_advantages
                policy_loss = -torch.min(surr1, surr2).mean()
                
                # Value loss
                value_loss = F.mse_loss(new_values, batch_returns)
                
                # Entropy loss
                entropy_loss = -entropy.mean()
                
                # Total loss
                loss = policy_loss + self.value_coef * value_loss + self.entropy_coef * entropy_loss
                
                # Backward pass
                loss.backward()
        
        # Clip gradients
        torch.nn.utils.clip_grad_norm_(model.parameters(), self.max_grad_norm)
        
        # Get gradients
        gradients = {}
        for name, param in model.named_parameters():
            if param.grad is not None:
                gradients[name] = param.grad.clone()
        
        return gradients
    
    def save_model(self, model: nn.Module, path: str):
        """
        Save model to disk
        
        Args:
            model: Model to save
            path: Path to save to
        """
        # Ensure directory exists
        os.makedirs(os.path.dirname(path), exist_ok=True)
        
        # Save model
        torch.save(model.state_dict(), path)
        logger.info(f"Model saved to {path}")
    
    def export_to_coreml(self, model: nn.Module, path: str):
        """
        Export model to CoreML format
        
        Args:
            model: Model to export
            path: Path to save to
        """
        try:
            import coremltools as ct
            
            # Create a sample input for tracing
            input_dim = self.config.get("input_dim", 128)
            sample_input = torch.rand(1, input_dim)
            
            # Trace the model
            traced_model = torch.jit.trace(
                lambda x: model.get_action_and_value(x, deterministic=True)[0], 
                sample_input
            )
            
            # Convert to CoreML
            coreml_model = ct.convert(
                traced_model,
                inputs=[ct.TensorType(name="input", shape=sample_input.shape)]
            )
            
            # Save model
            coreml_model.save(path)
            logger.info(f"Model exported to CoreML format at {path}")
            
        except Exception as e:
            logger.error(f"Error exporting to CoreML: {e}")

def create_model(config: Dict[str, Any]) -> nn.Module:
    """
    Create a new PPO model based on configuration
    
    Args:
        config: Configuration dictionary
        
    Returns:
        PPO network model
    """
    # Get dimensions from config
    input_dim = config.get("input_dim", 128)
    hidden_dim = config.get("hidden_dim", 256)
    num_actions = config.get("num_actions", 16)
    
    # Create model
    model = PPONetwork(input_dim, hidden_dim, num_actions)
    
    # Use CUDA if available
    if torch.cuda.is_available() and config.get("use_cuda", True):
        model = model.cuda()
    
    return model

def load_model(path: Optional[str]) -> nn.Module:
    """
    Load a model from disk or create a new one if path is None
    
    Args:
        path: Path to model file, or None to create a new model
        
    Returns:
        Loaded or new model
    """
    if path and os.path.exists(path):
        # Load config
        config_path = os.path.join(os.path.dirname(path), "config.json")
        if os.path.exists(config_path):
            with open(config_path, 'r') as f:
                config = json.load(f)
        else:
            # Default config
            config = {
                "input_dim": 128,
                "hidden_dim": 256,
                "num_actions": 16
            }
        
        # Create model
        model = create_model(config)
        
        # Load state
        model.load_state_dict(torch.load(path))
        logger.info(f"Loaded model from {path}")
        
        return model
    else:
        # Create new model with default config
        config = {
            "input_dim": 128,
            "hidden_dim": 256,
            "num_actions": 16
        }
        
        model = create_model(config)
        logger.info("Created new model with default config")
        
        return model

if __name__ == "__main__":
    # Example standalone training (not distributed)
    import argparse
    
    parser = argparse.ArgumentParser(description="PPO Training for FBNeo AI")
    parser.add_argument("--config", required=True, help="Path to configuration file")
    parser.add_argument("--output", required=True, help="Path to save model")
    
    args = parser.parse_args()
    
    # Load configuration
    with open(args.config, 'r') as f:
        config = json.load(f)
    
    # Create model and trainer
    model = create_model(config)
    trainer = PPOTrainer(config)
    
    # Training loop
    num_iterations = config.get("num_iterations", 100)
    for i in range(num_iterations):
        logger.info(f"Iteration {i+1}/{num_iterations}")
        
        # Collect experiences
        experiences = trainer.collect_experiences(model)
        
        # Compute gradients and update model
        gradients = trainer.compute_gradients(model, experiences)
        
        # Apply gradients
        optimizer = optim.Adam(model.parameters(), lr=config.get("lr", 3e-4))
        optimizer.zero_grad()
        
        for name, param in model.named_parameters():
            if name in gradients:
                param.grad = gradients[name]
        
        optimizer.step()
        
        # Save model periodically
        if (i+1) % config.get("save_interval", 10) == 0:
            trainer.save_model(model, f"{args.output}_iter_{i+1}.pt")
    
    # Save final model
    trainer.save_model(model, args.output)
    
    # Export to CoreML if enabled
    if config.get("export_coreml", False):
        trainer.export_to_coreml(model, f"{args.output}.mlmodel") 