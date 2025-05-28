# FBNeo 2025 AI Models Compatibility Guide

## Supported Model Formats

The FBNeo 2025 AI implementation supports multiple model formats through a flexible loader system:

1. **CoreML (.mlmodel, .mlpackage)**
   - Primary format for Apple Silicon optimization
   - Supports Neural Engine hardware acceleration
   - Best performance on Apple devices
   - Can be converted from other formats

2. **PyTorch (.pt, .pth)**
   - Support via conversion to CoreML
   - Requires additional conversion step
   - Good for research and rapid iteration

3. **ONNX (.onnx)**
   - Industry-standard interchange format
   - Supported via conversion to CoreML
   - Useful for cross-platform models

4. **TensorFlow Lite (.tflite)**
   - Lightweight models for mobile/embedded devices
   - Lower overhead for simpler models
   - Supported via direct inference or conversion

## Model Specifications

### Input Requirements

All models must accept standard input formats:

- **Game Frame Images**
  - Dimensions: Variable, typically 224x256 to 384x224
  - Channels: 3 (RGB) or 4 (RGBA)
  - Data type: float32 (normalized 0-1) or uint8 (0-255)

- **Game State Vectors** (optional)
  - Dimensions: Variable based on game
  - Typical elements: score, lives, level, power-ups, etc.
  - Data type: float32

### Output Requirements

Models should produce one of the following output formats:

- **Action Vectors**
  - Dimensions: Variable based on control scheme
  - Elements: Button presses, joystick positions
  - Data type: float32 (for analog values) or boolean (for buttons)

- **Action Probabilities**
  - Dimensions: Number of possible actions
  - Elements: Probability for each action
  - Data type: float32 (0-1 range)

## Hardware Acceleration

The implementation leverages multiple hardware acceleration paths:

1. **Apple Neural Engine**
   - Available on M1/M2/M3 chips and newer
   - Highest performance for CoreML models
   - Automatic fallback to GPU/CPU

2. **Metal Performance Shaders**
   - GPU-accelerated inference
   - Supported on all Apple GPUs
   - Used when Neural Engine is unavailable

3. **CPU Fallback**
   - Multi-threaded CPU execution
   - Available on all systems
   - Used when GPU acceleration is unavailable

## Model Loading System

The model loading system follows a defined search path:

1. **Application Bundle**
   - `Resources/Models/` in the application bundle
   - Pre-packaged default models

2. **User Library**
   - `~/Library/Application Support/FBNeo/Models/`
   - User-installed models

3. **Game-Specific Directory**
   - `~/Library/Application Support/FBNeo/Models/<game_id>/`
   - Game-specific custom models

## Model Conversion

The implementation includes built-in model conversion capabilities:

1. **PyTorch to CoreML**
   - Uses `torch.jit` and `coremltools`
   - Preserves original architecture
   - Optimizes for Metal execution

2. **ONNX to CoreML**
   - Direct conversion via `coremltools`
   - Maintains computational graph
   - Maps to Metal Performance Shaders

3. **TensorFlow Lite to CoreML**
   - Conversion via intermediate ONNX format
   - May require operator mapping
   - Performance varies by model complexity

## Performance Considerations

When creating or selecting models for FBNeo, consider:

1. **Size and Complexity**
   - Smaller models offer lower latency
   - <5MB models ideal for real-time gameplay
   - <20MB models work well for analysis

2. **Inference Speed**
   - Target <10ms for reactive gameplay
   - <30ms acceptable for slower games
   - <100ms for analysis-only models

3. **Memory Usage**
   - <100MB working memory for best performance
   - <500MB for more complex models
   - Memory efficiency critical on low-end devices

## Game Compatibility

Models can be created for specific games or generalized across genres:

1. **General Models**
   - Work across multiple games in a genre
   - Lower accuracy but broader applicability
   - Good for exploration and casual use

2. **Game-Specific Models**
   - Optimized for a single game
   - Higher accuracy and response
   - Can incorporate game-specific knowledge

3. **Fine-Tuned Models**
   - Base model with game-specific fine-tuning
   - Balance between specificity and generalization
   - Good compromise for most users

## Future Compatibility

The 2025 AI implementation is designed for forward compatibility:

1. **Apple MLX Support**
   - Framework for advanced ML on Apple Silicon
   - Will be added in future updates

2. **New Model Architectures**
   - Support for transformer-based models
   - Vision transformer integration planned

3. **Custom Operator Support**
   - Framework for game-specific operators
   - Performance optimizations for specialized tasks 