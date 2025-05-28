# FBNeo Metal AI Integration

This document provides information about the AI integration in the Metal backend for Final Burn Neo emulator.

## Overview

The FBNeo Metal AI integration leverages Apple's latest technologies to enhance the emulation experience through intelligent gameplay assistance, improved CPU opponents, self-learning capabilities, and advanced visual features. The implementation is built on top of the following key technologies:

- **CoreML 5.0**: For secure model loading and hardware-accelerated inference
- **Metal 3.5**: For tensor operations and neural network computation
- **Neural Engine**: For optimized AI performance on Apple Silicon
- **Vision**: For frame analysis and pattern recognition

## Installation

### Prerequisites

- macOS 16+ (Sequoia)
- Apple Silicon Mac (M1/M2/M3 or later)
- Xcode 16.0 or later (for development)
- Homebrew (optional, for dependencies)

### Building from Source

1. Clone the FBNeo repository:
   ```
   git clone https://github.com/finalburnneo/FBNeo.git
   cd FBNeo
   ```

2. Run the AI-enabled build script:
   ```
   ./build_metal_ai.sh
   ```

3. Complete the build process:
   ```
   make -f makefile.metal.ai
   ```

4. The built binary will be created as `fbneo_metal_ai` in the project root.

### Installing Models

AI models are required for optimal performance. You can use:

1. **Automatic installation**:
   ```
   ./download_models.sh
   ```

2. **Manual installation**: Place model files in one of the following directories:
   - `~/Documents/FBNeo/models`
   - `/Users/Shared/FBNeo/models`
   - `models/` directory in the FBNeo application folder

## Usage

### Basic Usage

1. Launch FBNeo Metal with AI features:
   ```
   ./fbneo_metal_ai
   ```

2. Access AI settings from the in-game menu (Input → AI Settings).

3. Choose an appropriate AI model based on your game and desired features.

### AI Modes

The AI system can operate in several modes:

- **Analysis**: Observe gameplay to learn patterns without interfering
- **Assist**: Provide gameplay assistance at various levels
- **Opponent**: Enhance CPU opponents with improved AI
- **Training**: Train the AI through self-play
- **Benchmark**: Test AI performance

### Configuration Options

From the AI settings menu, you can configure:

- **Assistance Level**: Controls how much help the AI provides
- **Difficulty**: Adjusts the challenge level for AI opponents
- **Model Selection**: Choose from available AI models
- **Performance Settings**: Balance quality vs. speed
- **Visualization**: Enable AI visualization overlays

## Features

### Player Assistance

The AI can help players in several ways:

- **Reactive Assistance**: Help with blocking, dodging, and timing
- **Combo Assistance**: Detect combo opportunities and assist execution
- **Strategy Hints**: Provide strategic advice during gameplay
- **Adaptive Difficulty**: Adjust to the player's skill level

### CPU Enhancement

For single-player games, the AI can:

- **Improve CPU Opponents**: Make them more challenging and realistic
- **Add Variety**: Create varied and unpredictable play styles
- **Scale Difficulty**: Provide appropriate challenge for any skill level

### Training and Learning

The AI system can:

- **Learn from Players**: Adapt to player styles through observation
- **Self-Improvement**: Use reinforcement learning to improve over time
- **Export Models**: Save trained models for future use
- **Game-Specific Learning**: Optimize strategies for specific games

### Visualization

AI visualizations can show:

- **Attention Maps**: See what the AI is focusing on
- **Decision Confidence**: Visualize AI certainty about decisions
- **Predicted Actions**: Preview AI's planned moves
- **Game State Analysis**: See the AI's understanding of the game state

## Technical Details

### Supported Model Formats

- **CoreML** (.mlmodel, .mlpackage)
- **PyTorch** (.pt, .pth)
- **ONNX** (.onnx)
- **TensorFlow Lite** (.tflite)

### Privacy and Security

- **Differential Privacy**: Protects user data during training
- **Secure Model Loading**: Verifies model integrity and authenticity
- **Local Processing**: All AI processing happens on-device

### Performance Optimization

- **Mixed Precision**: Supports FP32, FP16, and INT8/INT4 quantization
- **Hardware Acceleration**: Utilizes CPU, GPU, and Neural Engine
- **Batch Processing**: Efficient multi-frame processing
- **Low Latency Mode**: Optimized for real-time gameplay

## Troubleshooting

### Common Issues

- **Models Not Loading**: Ensure models are placed in the correct directory and have the right format
- **Performance Issues**: Try lowering the precision or disabling certain AI features
- **Compatibility Problems**: Some older games may not be fully supported by all AI features

### Debug Mode

Enable debug logging with:
```
./fbneo_metal_ai --ai-debug
```

This will create detailed logs in `~/Library/Logs/FBNeo/ai_debug.log`.

## Extending

Developers can extend the AI system with:

- **Custom Models**: Create specialized models for specific games
- **Game-Specific Analyzers**: Implement game-specific state extraction
- **New Features**: Add new AI capabilities via the API

## Credits

The FBNeo Metal AI implementation builds upon:

- **Apple's ML Frameworks**: CoreML, Metal Performance Shaders
- **Open Source Models**: Various pretrained networks and architectures
- **FBNeo Core**: The outstanding emulation framework
- **Community Contributions**: Testing, models, and feedback

## License

This AI implementation is released under the same license as FBNeo. See the LICENSE file for more details. 