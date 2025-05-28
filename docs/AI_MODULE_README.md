# FBNeo Metal AI Module

This document provides an overview of the AI module integrated with the FBNeo Metal renderer.

## Overview

The AI module is a reinforcement learning framework that allows FBNeo to:

1. **Learn to play games** through self-play and imitation learning
2. **Enhance CPU opponents** with more intelligent behavior
3. **Provide player assistance** with suggestions and automatic combos
4. **Analyze gameplay** for insights and improvement opportunities

## Architecture

The AI system consists of the following components:

### Core RL Framework
- **RL Algorithms**: Implementations of PPO and A3C with support for prioritized experience replay
- **Intrinsic Curiosity Module (ICM)**: Enhances exploration in sparse reward environments
- **Policy Models**: Neural network models for state-to-action mapping
- **Experience Buffer**: Storage mechanism for training data

### Metal Integration
- **Metal AI Module**: Bridge between the AI system and Metal renderer
- **CoreML Integration**: Support for Apple Neural Engine acceleration
- **PyTorch to CoreML Conversion**: Tools for optimizing and deploying models

### Game Integration
- **Game-specific reward functions**: Tailored for different game genres (fighting, platformer, etc.)
- **Memory mapping**: Access to game state variables for different ROMs
- **AI Controller**: Allows toggling AI assistance during gameplay

### UI Components
- **AI Menu**: Controls for training, loading, and saving models
- **Visualizations**: Optional overlays for displaying AI state, decisions, and confidence

## Usage

### Basic Controls

The AI module can be controlled through the AI menu in FBNeo Metal:

- **Enable/Disable AI**: Toggle AI control of the game
- **Enable/Disable Training Mode**: Toggle learning mode
- **Save/Load Model**: Persist AI models for later use
- **Export to CoreML**: Convert PyTorch models to CoreML format for better performance
- **Optimize CoreML Model**: Apply additional optimizations for Apple hardware
- **Configure Distributed Training**: Set parameters for distributed learning
- **Create Game Memory Mapping**: Generate mappings for game state variables

### Training a New Model

1. Select the game you want to train on
2. Enable AI through the AI menu
3. Enable Training Mode
4. Configure training parameters (optional)
5. Play the game or let it self-play to gather experience
6. Save the model when performance is satisfactory

### Using a Pre-trained Model

1. Select the game you want to play
2. Load a previously trained model through the AI menu
3. Enable AI
4. Disable Training Mode (unless you want to continue training)
5. Play with AI assistance or watch the AI play

## Technical Details

### Reinforcement Learning Algorithms

- **PPO (Proximal Policy Optimization)**
  - Clips policy updates to prevent destructive changes
  - Uses Generalized Advantage Estimation (GAE)
  - Stable training with good sample efficiency

- **A3C (Asynchronous Advantage Actor-Critic)**
  - Parallel training across multiple workers
  - Asynchronous gradient updates to global policy
  - Good performance on multicore systems

### Neural Network Architectures

- **CNN**: For processing visual game state
- **MLP**: For processing feature vectors
- **LSTM**: For handling temporal dependencies
- **Transformer**: For complex reasoning about game state

### Apple-specific Optimizations

- **CoreML Integration**: Models can run on the Apple Neural Engine for maximum performance
- **Quantization**: Models can be compressed to 16-bit or 8-bit precision
- **Metal Performance Shaders**: Used for GPU acceleration of tensor operations

### Distributed Training

The system supports distributed training across multiple threads with:

- **Parameter synchronization**: Maintains consistency across workers
- **Experience sharing**: Allows workers to learn from each other's experiences
- **Gradient accumulation**: Improves stability of updates

## Advanced Configuration

### Memory Mapping

The AI system can access game state directly through memory mappings for more precise rewards and state observations. Memory mappings are stored in game-specific files and are created automatically or can be manually defined.

### Reward Functions

Different game genres have specialized reward functions:

- **Fighting Games**: Rewards for dealing damage, avoiding hits, performing combos
- **Platformers**: Rewards for progress, collecting items, avoiding damage
- **Puzzle Games**: Rewards for clearing lines/pieces, increasing score
- **Shooters**: Rewards for defeating enemies, avoiding damage, collecting power-ups

## Troubleshooting

If you encounter issues:

1. Check the console for error messages
2. Verify that the game ROM is supported
3. Try creating a new memory mapping for the game
4. Ensure CoreML and PyTorch dependencies are installed
5. Reduce model complexity if performance is poor

## Performance Tips

- Use CoreML models instead of PyTorch for inference
- Enable quantization for faster inference
- Target the Apple Neural Engine when available
- Reduce the input frame size when training
- Use simpler network architectures for older/slower hardware

## Contributing

To extend the AI module:

1. Add support for new game genres in `reward_functions.cpp`
2. Add memory mappings for games in `game_memory_mappings.json`
3. Implement new RL algorithms in the `ai_rl_algorithms.cpp` file
4. Add new visualizations in the `overlay_renderer.cpp` file

## License

The AI module is released under the same license as FBNeo. 