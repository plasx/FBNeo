# FBNeo Metal AI Implementation - Next Tasks

This document outlines the specific tasks that need to be completed for the AI implementation in FBNeo Metal after the build system has been fixed.

## 1. CoreML Integration

### 1.1 CoreML Manager Implementation
- [ ] Create `coreml_manager.mm` with the following functionality:
  - [ ] Model loading and verification
  - [ ] Neural Engine acceleration support
  - [ ] Batch prediction capability
  - [ ] Error handling and reporting
  - [ ] Model metadata extraction

### 1.2 Model Loading System
- [ ] Implement model discovery in standard locations:
  - [ ] Application bundle (`Resources/Models/`)
  - [ ] User library (`~/Library/Application Support/FBNeo/Models/`)
  - [ ] Game-specific directories
- [ ] Create model verification system that checks:
  - [ ] Input/output tensor dimensions
  - [ ] Required operations support
  - [ ] Model version compatibility

### 1.3 Model Format Support
- [ ] Add support for multiple model formats:
  - [ ] Native CoreML (`.mlmodel`, `.mlpackage`)
  - [ ] PyTorch conversion using `coremltools`
  - [ ] ONNX conversion support
  - [ ] TensorFlow Lite support (optional)

## 2. Frame Processing Pipeline

### 2.1 Frame Capture
- [ ] Create hooks into the emulation render loop:
  - [ ] Capture frame buffer after rendering
  - [ ] Implement frame skipping for performance
  - [ ] Add buffer pooling to minimize allocations

### 2.2 Frame Preprocessing
- [ ] Implement image processing operations:
  - [ ] RGB/RGBA format conversion
  - [ ] Resizing to model input dimensions
  - [ ] Normalization (0-1 or -1 to 1)
  - [ ] Data layout transformation (NCHW/NHWC)

### 2.3 Metal Compute Shaders
- [ ] Create metal compute shaders for preprocessing:
  - [ ] `preprocess.metal` for input transformation
  - [ ] `postprocess.metal` for output processing
  - [ ] Performance-optimized implementations

## 3. Action System

### 3.1 Action Mapping
- [ ] Create action mapping system:
  - [ ] Define map between AI outputs and emulator inputs
  - [ ] Support different control schemes
  - [ ] Add game-specific mapping profiles

### 3.2 Input Injection
- [ ] Implement input injection into the emulation:
  - [ ] Button press/release events
  - [ ] Joystick movement simulation
  - [ ] Special input sequences

### 3.3 Confidence Handling
- [ ] Add confidence threshold system:
  - [ ] User-configurable thresholds
  - [ ] Per-action type thresholds
  - [ ] Hysteresis to prevent rapid switching

## 4. UI and Configuration

### 4.1 Settings Interface
- [ ] Create settings UI for AI features:
  - [ ] Model selection dropdown
  - [ ] Enable/disable toggle
  - [ ] Performance settings (frame skip, batch size)
  - [ ] Confidence threshold sliders

### 4.2 Visualization
- [ ] Implement debug visualization:
  - [ ] Action confidence display
  - [ ] Performance metrics (inference time, FPS)
  - [ ] Heat map overlay for spatial attention

### 4.3 Configuration Persistence
- [ ] Add configuration saving/loading:
  - [ ] Global AI settings
  - [ ] Per-game settings profiles
  - [ ] Model preferences

## 5. Testing and Optimization

### 5.1 Unit Tests
- [ ] Create tests for AI components:
  - [ ] Model loading and verification
  - [ ] Frame preprocessing pipeline
  - [ ] Action mapping and injection

### 5.2 Performance Optimization
- [ ] Optimize inference performance:
  - [ ] Batch processing for multiple frames
  - [ ] Asynchronous processing
  - [ ] Weight quantization options

### 5.3 Compatibility Testing
- [ ] Test on different hardware configurations:
  - [ ] Apple M1/M2/M3 series chips
  - [ ] Intel Macs with Metal support
  - [ ] Different GPU capabilities

## 6. Documentation

### 6.1 User Documentation
- [ ] Create user guides:
  - [ ] AI feature usage instructions
  - [ ] Model installation guide
  - [ ] Game compatibility list

### 6.2 Developer Documentation
- [ ] Document the AI integration:
  - [ ] Architecture overview
  - [ ] API reference
  - [ ] Custom model creation guide

### 6.3 Example Models
- [ ] Provide example models:
  - [ ] General gameplay assistant
  - [ ] Game-specific models for popular games
  - [ ] Tutorial models with gradual assistance

## Implementation Timeline

### Phase 1: Core Integration (1-2 weeks)
- Complete CoreML Manager implementation
- Basic frame capture and preprocessing
- Simple action mapping system

### Phase 2: Features and UI (2-3 weeks)
- Full preprocessing pipeline with Metal shaders
- Complete action system with game-specific mappings
- Settings UI and visualization

### Phase 3: Optimization and Testing (1-2 weeks)
- Performance optimization across different hardware
- Comprehensive testing
- User and developer documentation 