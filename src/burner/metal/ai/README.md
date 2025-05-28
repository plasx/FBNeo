# FBNeo AI Module - CoreML Integration

This directory contains the AI module for FBNeo that enables machine learning capabilities through CoreML on macOS.

## Overview

The FBNeo AI Module allows:

1. Loading and running CoreML models for game AI assistance
2. Converting PyTorch models to CoreML format
3. Frame processing and game state analysis
4. AI-controlled game play
5. Training mode assistance

## Requirements

- macOS 10.15 or later (macOS 11+ recommended for Apple Neural Engine support)
- Xcode 12+ for development
- Python 3.6+ for model conversion
- PyTorch and CoreMLTools for custom model development

## Quick Start

### Building with AI Support

```bash
# Build FBNeo with AI support
make ai_build
```

This creates the emulator binary and also generates default AI models.

### Converting Models

```bash
# Convert models from PyTorch to CoreML
make convert_models

# Validate converted models
make validate_models
```

### Directory Structure

- `ai_definitions.h` - Core AI data structures and definitions
- `ai_input_frame.h` - Frame input format for models
- `ai_output_action.h` - Action output format from models
- `coreml_bridge.mm` - Objective-C++ bridge to CoreML
- `torch_to_coreml.mm` - PyTorch to CoreML conversion bridge
- `scripts/` - Python scripts for model conversion and validation

## Using AI Features

### Loading Models

Models are automatically loaded from the `models/` directory. Game-specific models are named after their ROM ID (e.g., `mvsc.mlmodel` for Marvel vs Capcom).

If a game-specific model isn't available, the system will fall back to `generic.mlmodel`.

### Controlling AI Behavior

The following functions are available:

```c
// Initialize AI module
int Metal_InitAI(void);

// Shutdown AI module
int Metal_ShutdownAI(void);

// Initialize AI for a specific game
int Metal_InitAIForGame(const char* gameId);

// Start AI control
int Metal_StartAI(void);

// Stop AI control
int Metal_StopAI(void);

// Control which player the AI controls
int Metal_SetAIPlayer(int player);

// Set AI difficulty level (0-10)
int Metal_SetAIDifficulty(int level);

// Enable AI training mode
int Metal_EnableAITrainingMode(int enable);

// Show AI debug overlay
int Metal_EnableAIDebugOverlay(int enable);

// Load a specific AI model
int Metal_LoadAIModel(const char* modelPath);
```

## Creating Custom Models

### Model Architecture

FBNeo AI models expect:

1. Input: Image data (game screen) with dimensions matching the game resolution
2. Output: Action predictions for game controls

### Converting Custom PyTorch Models

```bash
# Convert a PyTorch model to CoreML
python3 scripts/torch_to_coreml.py --input your_model.pt --output models/custom.mlmodel --game-type mvsc
```

### Supported Game Types

- `mvsc` - Marvel vs Capcom (or other Capcom VS games)
- `sfa3` - Street Fighter Alpha 3
- `ssf2t` - Super Street Fighter 2 Turbo
- `kof98` - King of Fighters 98
- `dino` - Cadillacs and Dinosaurs
- `1944` - 1944: The Loop Master
- `generic` - Generic model (works with any game)

## Using the CoreML API Directly

For advanced usage, you can use the CoreML bridge functions directly:

```c
// Initialize CoreML
bool CoreML_Initialize();

// Load a CoreML model
bool CoreML_LoadModel(const char* path);

// Get model information
bool CoreML_GetModelInfo(AIModelInfo* info);

// Process a frame using the model
bool CoreML_ProcessFrame(const void* frameData, int width, int height, int pitch, float* results, int resultSize);

// Render debug visualization
bool CoreML_RenderVisualization(void* overlayData, int width, int height, int pitch, int visualizationType);

// Shutdown CoreML
void CoreML_Shutdown();
```

## Performance Optimization

The CoreML integration can leverage:

- Apple Neural Engine (ANE) on M1/M2 Macs
- GPU acceleration on all supported Macs
- CPU fallback when needed

To optimize a model for a specific device:

```bash
python3 scripts/torch_to_coreml.py --input model.pt --output model.mlmodel --compute-units ALL
```

Options for `compute-units` are:
- `CPU` - CPU only
- `GPU` - GPU only
- `ANE` - Neural Engine only (M1/M2/M3 Macs)
- `ALL` - Use all available compute resources (default)

## Debugging

Enable the AI debug overlay to see:
- Model input/output visualization
- Performance metrics
- Action confidence levels
- Frame processing information

## Contributing

To contribute new models or improve the AI system:

1. Fork the repository
2. Create your feature branch
3. Add or improve models
4. Submit a pull request

## License

Same license as FBNeo 