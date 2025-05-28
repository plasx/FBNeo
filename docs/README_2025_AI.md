# FBNeo 2025 AI Features - README

## Overview

The FBNeo (Final Burn Neo) emulator has been enhanced with cutting-edge artificial intelligence capabilities utilizing Apple's latest 2025 technologies. This implementation leverages CoreML 5.0, Metal 3.5 Performance Shaders, and PyTorch 2.5 to provide unprecedented AI-powered features for both gameplay enhancement and research applications.

## Key Features

- **AI-Assisted Gameplay**: Intelligent assistance with configurable difficulty levels
- **Self-Learning AI**: On-device training using reinforcement learning
- **Neural Engine Optimization**: Hardware-accelerated inference using Apple Neural Engine
- **Low-Latency Processing**: 60+ FPS performance with active AI
- **Memory-Efficient Design**: Quantized models with int4/int8 precision
- **Visualization Tools**: Real-time display of AI decision-making
- **Research Framework**: Tools for reinforcement learning experimentation

## System Requirements

- **Hardware**: Apple Silicon M3/M4 or newer
- **OS**: macOS 16+ (Sequoia)
- **Memory**: 16GB RAM minimum (32GB+ recommended for training)
- **Storage**: 2GB for base installation, 10GB+ recommended for model storage

## Installation

### Standard Installation

1. Download the latest release from the [releases page](https://github.com/finalburnneo/FBNeo/releases)
2. Mount the DMG file and drag FBNeo to your Applications folder
3. Launch the application

### Building from Source

1. Clone the repository:
   ```bash
   git clone https://github.com/finalburnneo/FBNeo.git
   cd FBNeo
   ```

2. Install dependencies:
   ```bash
   brew install cmake python@3.11
   pip3 install torch torchvision
   ```

3. Build the Metal AI implementation:
   ```bash
   ./build_metal_core.sh
   ```

4. Download pre-trained models (optional):
   ```bash
   ./download_models.sh
   ```

## Basic Usage

### AI Assistant Mode

1. Launch FBNeo and load a ROM
2. Open the AI menu (`⌘+A` or **Options → AI**)
3. Select **Enable AI Assistant**
4. Configure the difficulty level (1-10)
5. Begin gameplay with AI assistance

### Self-Play Training Mode

1. Launch FBNeo and load a ROM
2. Open the AI menu (`⌘+A` or **Options → AI**)
3. Select **Enable Training Mode**
4. Configure training parameters
5. Begin self-play training session
6. Monitor progress in the training visualization window

### Model Management

1. Open the AI menu (`⌘+A` or **Options → AI**)
2. Select **Manage Models**
3. Options for:
   - Importing models
   - Selecting active models
   - Viewing model performance metrics
   - Exporting trained models

## Advanced Features

### Visualization Tools

1. Enable AI visualization (`⌘+V` or **Options → AI → Visualization**)
2. Available visualizations:
   - Action probability distribution
   - State value estimates
   - Attention maps
   - Feature activations

### Custom Model Training

1. Open the AI menu (`⌘+A` or **Options → AI**)
2. Select **Custom Training**
3. Configure:
   - Network architecture
   - Learning parameters
   - Training scenarios
   - Reward functions

### Distributed Training

1. Enable multi-instance training in the settings
2. Configure the primary coordinator instance
3. Launch additional FBNeo instances in worker mode
4. Monitor distributed training progress

## Documentation

For more detailed information, refer to the following documentation:

- [AI Features Summary](2025_AI_Features_Summary.md) - Overview of all AI capabilities
- [Implementation Status](Implementation_Status_Update.md) - Current development status
- [Next Steps](2025_AI_Next_Steps.md) - Upcoming development priorities
- [Technical Details](Advanced_AI_Implementation_2025.md) - Detailed technical implementation
- [API Reference](./ai_api/README.md) - API documentation for developers

## Troubleshooting

### Common Issues

1. **Performance Problems**
   - Ensure you're running on supported hardware
   - Check that other resource-intensive applications are closed
   - Reduce model complexity in AI settings

2. **Model Loading Failures**
   - Verify model compatibility with your OS version
   - Check file permissions for the models directory
   - Try reinstalling the pre-trained models

3. **Training Instability**
   - Increase memory allocation for training
   - Reduce batch size in training settings
   - Use checkpointing for long training sessions

### Logging and Diagnostics

1. Enable verbose logging:
   ```bash
   defaults write com.fbneo.FBNeo AILoggingLevel -int 3
   ```

2. Generate diagnostic report:
   ```bash
   ./fbneo_metal --ai-diagnostics > diagnostic_report.txt
   ```

## Contributing

Contributions to the FBNeo AI implementation are welcome! Please see [CONTRIBUTING.md](../CONTRIBUTING.md) for guidelines.

Key areas for contribution:
- Model optimization techniques
- New game-specific AI models
- Training curriculum improvements
- Documentation and tutorials

## License

This project is licensed under the BSD 3-Clause License - see the [LICENSE](../LICENSE) file for details.

## Acknowledgments

- The FBNeo development team
- Apple's Core ML and Metal engineering teams
- PyTorch development team
- Contributors to the reinforcement learning research community

## Contact

For questions, feedback, or issues related to the AI implementation:
- GitHub Issues: [https://github.com/finalburnneo/FBNeo/issues](https://github.com/finalburnneo/FBNeo/issues)
- Discord: [FBNeo Discord Channel](https://discord.gg/xAg5Ve) 