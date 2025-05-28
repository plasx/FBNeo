# Building FBNeo with AI Features

This guide explains how to build FBNeo with the 2025 AI features enabled on macOS.

## Prerequisites

Before building, ensure you have the following installed:

1. **Xcode 15.0+** (required for Metal 3.0+ support)
2. **macOS 14.0+** (Sonoma or newer)
3. **Command Line Tools** (`xcode-select --install`)
4. **Homebrew** (for additional dependencies)

Required packages:
```bash
brew install cmake ninja python@3.11
```

For AI development, you'll also need:
```bash
pip3 install torch torchvision coremltools onnx tensorflow-macos
```

## Basic Build

To build the basic version with AI features:

```bash
# Clone the repository if you haven't already
git clone https://github.com/finalburnneo/FBNeo.git
cd FBNeo

# Clean any previous build
make -f makefile.metal clean

# Build with AI features enabled
./build_metal_ai.sh
```

The compiled binary will be available as `fbneo_metal` in the root directory.

## Build Options

### Optimized Build

For maximum performance on Apple Silicon:

```bash
METAL_OPTIMIZATION=high ENABLE_NEURAL_ENGINE=1 ./build_metal_ai.sh
```

This enables aggressive optimization and Neural Engine acceleration.

### Debug Build

For development and debugging:

```bash
DEBUG=1 METAL_SHADER_DEBUG=1 ./build_metal_ai.sh
```

This includes debug symbols and enables Metal shader debugging.

### Custom Model Path

To specify a custom model directory:

```bash
CUSTOM_MODEL_PATH="/path/to/models" ./build_metal_ai.sh
```

## Build Configuration Options

| Option | Values | Default | Description |
|--------|--------|---------|-------------|
| `DEBUG` | 0, 1 | 0 | Enable debug symbols and logging |
| `ENABLE_AI` | 0, 1 | 1 | Toggle AI features |
| `ENABLE_NEURAL_ENGINE` | 0, 1 | 1 | Enable Neural Engine acceleration |
| `METAL_OPTIMIZATION` | none, low, medium, high | medium | Metal optimization level |
| `METAL_SHADER_DEBUG` | 0, 1 | 0 | Enable Metal shader debugging |
| `CUSTOM_MODEL_PATH` | path | "" | Custom model directory |
| `PYTORCH_SUPPORT` | 0, 1 | 1 | Enable PyTorch model support |
| `ONNX_SUPPORT` | 0, 1 | 1 | Enable ONNX model support |
| `TFLITE_SUPPORT` | 0, 1 | 0 | Enable TensorFlow Lite support |

## Common Build Issues

### Missing CoreML Headers

If you encounter errors about missing CoreML headers:

```
error: 'CoreML/CoreML.h' file not found
```

Make sure you have the latest Xcode and are targeting a supported macOS version:

```bash
MACOSX_DEPLOYMENT_TARGET=14.0 ./build_metal_ai.sh
```

### Metal Shader Compilation Failures

If Metal shader compilation fails:

```
error: failed to compile Metal shader
```

Try disabling optimization for debugging:

```bash
METAL_OPTIMIZATION=none ./build_metal_ai.sh
```

### Model Loading Issues

If you see errors related to model loading:

```
Error: Failed to load CoreML model
```

Check that your models are in the correct format and location:

```bash
# Default search paths
~/Library/Application Support/FBNeo/Models/
./Resources/Models/
```

## Advanced Configuration

### Custom Build Script

For advanced builds, you can create a configuration file:

```bash
# Create a build config
cat > .metal_ai_config << EOF
DEBUG=1
ENABLE_NEURAL_ENGINE=1
METAL_OPTIMIZATION=high
CUSTOM_MODEL_PATH="$HOME/AI/Models"
EOF

# Build using the config
./build_metal_ai.sh --config .metal_ai_config
```

### Building Specific Components

To build only specific components:

```bash
# Build only the AI components
./build_metal_ai.sh --components ai

# Build only the Metal renderer
./build_metal_ai.sh --components metal

# Build both
./build_metal_ai.sh --components metal,ai
```

## Verifying AI Features

After building, verify AI features are working:

```bash
# Run with AI features enabled and verbose logging
./fbneo_metal --ai-debug --verbose
```

You should see information about available AI models and hardware acceleration in the console output.

## Next Steps

After building, you can:

1. Install custom models in `~/Library/Application Support/FBNeo/Models/`
2. Configure AI settings in the emulator's settings menu
3. Check the performance metrics in the AI debug overlay (enable with F10)

For more information on using the AI features, see the [AI Features User Guide](./AI_Features_User_Guide.md). 