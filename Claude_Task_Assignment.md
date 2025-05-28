# FBNeo AI Integration - Model-Specific Task Assignment

This document divides implementation tasks between different AI models based on their capabilities, ensuring the most powerful models tackle the most complex tasks.

## Claude 3.7 Sonnet MAX Tasks

Claude 3.7 Sonnet MAX is tasked with implementing core AI components and complex integrations requiring deep technical understanding.

### Current Focus

1. **Apple Silicon Advanced Optimizations**
   - Implement specialized Metal 3 ray tracing for advanced effects
   - Optimize CoreML models with quantization for Apple Neural Engine
   - Implement specialized ML compute units for faster inference
   - Design dynamic memory management for AI tensors

2. **Neural Engine Optimizations**
   - Create custom CoreML converters for specialized model architectures
   - Implement mixed-precision training pipeline
   - Develop dynamic network architecture adaptation
   - Create hardware-specific model optimization tools

3. **Advanced AI Components**
   - Implement PPO algorithm with multi-agent capabilities
   - Design self-play training architecture
   - Create model compression pipeline for embedded deployment
   - Implement curriculum learning for progressive AI training

### Completion Status

Claude 3.7 Sonnet MAX has already completed these critical components:

- ✅ `mps_graph_engine.mm` - Metal Performance Shaders Graph integration
- ✅ `coreml_engine.mm` - CoreML engine implementation
- ✅ `torch_to_coreml.mm` - PyTorch model conversion
- ✅ `enhanced_metal_shaders.metal` - Advanced shader effects
- ✅ `ai_torch_policy.cpp/h` - AI policy implementation
- ✅ `ai_input_frame.h` - Input data structure
- ✅ `ai_output_action.h` - Output action structure
- ✅ `frame_data_display.cpp/h` - Frame data visualization
- ✅ `game_state_display.cpp/h` - Game state visualization
- ✅ Headless mode implementation

## Regular Claude 3.7 Sonnet Tasks

Regular Claude 3.7 Sonnet is tasked with build system fixes, implementation of more standard components, and documentation.

### Current Focus

1. **Build System Fixes**
   - Fix missing header files via symlinks
   - Resolve macro redefinition issues
   - Update makefile include paths
   - Create missing directories for object files

2. **Core Integration**
   - Connect Metal renderer to FBNeo core
   - Fix ROM loading and game initialization
   - Implement proper frame buffer handling
   - Connect audio generation from emulation to CoreAudio

3. **UI Development**
   - Implement AI control panel interface
   - Add model selection screen
   - Create difficulty adjustment controls
   - Develop visualization tools for AI activity

### Next Steps for Claude 3.7 Sonnet

1. Create build script to fix header issues:
   ```bash
   mkdir -p src/dep/generated
   touch src/dep/generated/tiles_generic.h
   touch src/dep/generated/burnint.h
   ln -sf ../../../../burn/tiles_generic.h src/dep/generated/tiles_generic.h
   ln -sf ../../../../burn/burnint.h src/dep/generated/burnint.h
   ```

2. Fix `wrapper_burn.cpp` by removing test pattern code and connecting to real FBNeo core

3. Fix `metal_renderer.mm` language linkage issues for proper C++/Objective-C++ interaction

4. Fix ROM loading implementation in `metal_bridge.cpp`:
   ```cpp
   // Replace the stub implementation with actual ROM path detection
   int DetectRomPaths() {
       // Code to scan standard ROM directories
       // Check user Documents folder
       // Check user Application Support folder
       // Return count of ROMs found
   }
   ```

## GPT-4 Tasks (Optional)

GPT-4 can assist with these specific tasks when needed:

1. **Training Pipeline Development**
   - Design Python training scripts in PyTorch
   - Create model export utilities for CoreML
   - Implement training visualization and monitoring
   - Develop hyperparameter optimization tools

2. **Documentation and Tutorials**
   - Create comprehensive API documentation
   - Develop tutorial series for AI model creation
   - Design architecture diagrams
   - Write user guides for different features

## Stubs That Need Implementation

These stub implementations need to be replaced with functional code:

1. **Metal Integration Stubs**
   - `Metal_Init()` - Replace test pattern with actual FBNeo integration
   - `Metal_RenderFrame()` - Implement proper frame buffer conversion
   - `BurnLibInit_Metal()` - Connect to actual FBNeo initialization
   - `DetectRomPaths()` - Implement proper ROM discovery

2. **Audio Integration Stubs**
   - `Metal_InitAudioSystem()` - Connect to FBNeo audio generation
   - `Metal_GetAudioBuffer()` - Implement proper audio buffer access
   - `Metal_ProcessAudio()` - Handle audio processing and mixing

## Current Implementation Status

| Component | Status | Assigned To |
|-----------|--------|-------------|
| AI Engine Core | ✅ Complete (100%) | Claude 3.7 Sonnet MAX |
| Metal Renderer | 🔄 In Progress (95%) | Regular Claude 3.7 Sonnet |
| Build System Fixes | 🔄 In Progress | Regular Claude 3.7 Sonnet |
| Memory Mapping | ✅ Complete (95%) | Claude 3.7 Sonnet MAX |
| Headless Mode | ✅ Complete (100%) | Claude 3.7 Sonnet MAX |
| Training Pipeline | 🔄 In Progress (70%) | GPT-4 (Optional) |
| AI Visualization | ✅ Complete (100%) | Claude 3.7 Sonnet MAX |
| Automated Testing | 🟡 Not Started (0%) | Regular Claude 3.7 Sonnet |
| Audio Integration | 🔄 In Progress (60%) | Regular Claude 3.7 Sonnet |
| Input Integration | 🔄 In Progress (80%) | Regular Claude 3.7 Sonnet | 