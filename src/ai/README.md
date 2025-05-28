# FBNeo AI Integration Implementation

This directory contains the implementation of the AI integration for FinalBurn Neo. The system enables AI-driven gameplay, data collection, training, and deployment of models to various platforms.

## Core Components

### Data Structures

- **AIInputFrame** (`ai_input_frame.h/cpp`): Normalized representation of game state (player positions, health, etc.) extracted from memory. Used as input to AI models.
- **AIOutputAction** (`ai_output_action.h/cpp`): Represents possible AI actions and decisions with confidence levels. Maps to emulator input bitmasks.
- **AIMemoryMapping** (`ai_memory_mapping.h/cpp`): Game-specific memory address mapping for extracting state information. Configurable via JSON files.

### AI Controller

- **NeuralAIController** (`neural_ai_controller.h/cpp`): Core controller that orchestrates game state extraction, model inference, action selection, and data logging. Supports multiple AI backends.

### Model Infrastructure

- **AITorchPolicyModel** (`ai_torch_policy.h/cpp`): PyTorch-based neural network model for action inference. Uses LibTorch for C++ integration.
- **CoreMLInterface** (`coreml_interface.h/cpp`): Apple CoreML integration for running models on iOS and macOS using the Neural Engine.

### Data Collection & Training

- **AIDatasetLogger** (`ai_dataset_logger.h/cpp`): Thread-safe logger for recording game state and actions to JSONL files for training. Supports file rotation and asynchronous writes.

## Directory Structure

```
src/ai/
├── ai_input_frame.h/cpp       # Game state representation
├── ai_output_action.h/cpp     # AI action representation
├── ai_memory_mapping.h/cpp    # Memory mapping configuration
├── ai_dataset_logger.h/cpp    # Data collection logger
├── neural_ai_controller.h/cpp # Main AI controller
├── ai_torch_policy.h/cpp      # PyTorch model integration
├── coreml_interface.h/cpp     # CoreML integration (Apple platforms)
├── RLTrainingPipeline.md      # RL training pipeline specification
└── README.md                  # This file
```

## Integration Flow

1. **Initialization**: `NeuralAIController` loads the appropriate `AIMemoryMapping` for the current game.
2. **Per-Frame Processing**:
   - Game state is extracted into an `AIInputFrame` using `AIMemoryMapping`
   - `NeuralAIController` passes the frame to the active model (PyTorch or CoreML)
   - Model returns an `AIOutputAction` representing the chosen action
   - Action is converted to input bitmasks and injected into the emulator
   - If data collection is enabled, the frame and action are logged via `AIDatasetLogger`
3. **Multi-Frame Actions**: Special moves requiring sequences of inputs are managed by the `NeuralAIController` by queueing actions.

## Model Backend Options

- **PyTorch (LibTorch)**: Primary backend for desktop platforms, supports both inference and limited training.
- **CoreML**: High-performance backend for Apple platforms, optimized for Apple Silicon and the Neural Engine.
- **Random**: Simple random action selection for testing.
- **Rule-Based**: Simple heuristic AI for testing and development.

## Building and Dependencies

This code depends on:
- LibTorch (PyTorch C++ API)
- nlohmann/json for JSON parsing
- xxHash for fast hashing
- CoreML framework (on Apple platforms)

## Next Steps

- Implement a Python training pipeline using collected data
- Create model conversion utilities for CoreML export
- Add replay visualization tools for debugging and analysis
- Expand game support with additional memory mappings 