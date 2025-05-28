# AI Models for FBNeo

This document provides information about using and converting AI models with the FBNeo emulator.

## Supported Model Formats

FBNeo supports two types of AI models:

1. **PyTorch Models** (.pt, .pth files)
   - Loaded via LibTorch
   - Cross-platform support (macOS, Linux, Windows)
   - Higher CPU usage but more flexible

2. **CoreML Models** (.mlmodel, .mlmodelc files)
   - macOS/iOS only
   - Leverages Apple Neural Engine for acceleration on Apple Silicon
   - Lower power consumption
   - Potentially faster inference on Apple hardware

## Model Directory Structure

The default location for AI models is the `models/` directory in the FBNeo root folder:

```
models/
├── default_fighter.pt     # Default PyTorch model for fighting games
├── sf3_specialist.pt      # Specialized model for Street Fighter III
├── sf3_specialist.mlmodel # CoreML version of the same model
└── game_specific/
    ├── sfiii3n/           # Game-specific models
    └── kof98/
```

## Converting PyTorch Models to CoreML

To convert a PyTorch model to CoreML format, use the provided `model_converter.py` script:

```bash
# Install dependencies
pip install torch coremltools

# Convert a single model
python src/ai/model_converter.py --input models/my_model.pt --output models/my_model.mlmodel --verbose

# Batch convert all models in a directory
python src/ai/model_converter.py --input models/ --output models/ --batch --verbose

# Add quantization for smaller file size (but potentially lower accuracy)
python src/ai/model_converter.py --input models/my_model.pt --output models/my_model.mlmodel --quantize
```

### Conversion Parameters

- `--input-shape`: Specify input tensor shape if it can't be inferred (e.g., `--input-shape 1 32` for batch size 1 with 32 features)
- `--quantize`: Use 16-bit floating point for smaller file size
- `--verbose`: Print detailed information during conversion

## Model Requirements

### Input Format

Models should expect an input tensor with:
- PyTorch: Typically a FloatTensor with shape [1, N] where N is the number of features.
- CoreML: MLMultiArray with similar dimensions

The `AIInputFrame` class converts game state to the appropriate tensor format. Features include normalized player positions, health values, and other game state information.

### Output Format

Models should produce an output tensor with:
- Shape [1, M] where M is the number of possible actions
- Each element represents the probability/score for a specific action

The action with the highest score will be selected by the `NeuralAIController`.

## Using Models in FBNeo

1. Place your model file in the `models/` directory
2. Launch FBNeo with Metal renderer
3. Load a supported game
4. In the "AI" menu:
   - Select "Player 1" or "Player 2"
   - Enable "AI Control"
   - Select your model from the "Select Model" submenu
   - Choose a difficulty level
5. Play against the AI!

## Model Performance

On Apple Silicon Macs, CoreML models typically offer better performance due to Neural Engine acceleration. Some benchmarks:

| Model Type | Size | Inference Time | Power Usage |
|------------|------|----------------|-------------|
| PyTorch    | 1 MB | ~2-5 ms        | Medium      |
| CoreML     | 1 MB | ~0.5-2 ms      | Low         |

Actual performance will vary based on model complexity and hardware.

## Creating Custom Models

To create custom AI models, use the `DatasetLogger` to collect gameplay data, then train a model using the following general approach:

1. Collect gameplay data using `AIDatasetLogger`
2. Preprocess data with `dataset_loader.py`
3. Train a model using PyTorch
4. Export the model as a TorchScript module
5. Optionally convert to CoreML using `model_converter.py`

For detailed instructions on training models, see `TRAINING.md` (forthcoming).

## Troubleshooting

- **"Model file not found"**: Ensure the model is in the correct directory and has the correct permissions
- **"Failed to load model"**: Check that the model format is compatible (TorchScript for PyTorch, .mlmodel/.mlmodelc for CoreML)
- **"Incompatible input shape"**: The model expects a different input shape than what's provided by `AIInputFrame`
- **"Error compiling CoreML model"**: Try rebuilding the model with a different version of coremltools

For CoreML-specific issues, check Console.app for logs related to CoreML and Neural Engine. 