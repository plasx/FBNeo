# FBNeo Metal Port for macOS/ARM64

This document outlines the current state of the Metal port and how to build and run it.

## Current Status

The Metal port is currently in development and requires several fixes due to:

1. Language linkage conflicts in declarations between C and Objective-C++
2. Inconsistent function signatures in bridge files
3. Build system issues with the main makefile not correctly building the Metal port

## Working Demo Applications

To verify that Metal is working correctly on your system, we've created several demo applications:

### Basic Metal Window Demo
```
make -f makefile.metal testapp
./fbneo_metal_test
```

This creates a simple window using Metal with a blue background, verifying that Metal is functioning correctly.

### Animated Render Demo
```
make -f makefile.metal rendertest
./fbneo_metal_render
```

This creates a more advanced demo with an animated, colorful square that rotates and changes colors, demonstrating Metal's rendering capabilities.

## Required Fixes for the Full Metal Port

To make the full Metal port work correctly, the following issues need to be addressed:

### 1. Standardize Declarations

The declarations in metal_declarations.h have been standardized to work with both C and Objective-C++, with:

- All declarations within a consistent `extern "C"` block for C++ compatibility
- Removed duplicate declarations of the same variables and functions
- Added proper include guards

### 2. Fix Function Signatures

Function signature mismatches in bridge files have been addressed by:

- Ensuring AI_ProcessFrame has a consistent return type
- Fixing Metal_Init parameter list to be consistent in all files
- Standardizing Metal_RenderFrame return type

### 3. Updated Build System

A standalone build system for the Metal port has been created that doesn't rely on the main makefile:

- Added standalone build targets for demo applications
- Created simplified implementations that avoid dependency conflicts
- Fixed compiler and linker flags for macOS/ARM64

## Next Steps for Full Integration

For a more robust Metal port, consider the following architectural improvements:

1. **Better Separation of Concerns**
   - Create a clear separation between Metal rendering code and FBNeo core code
   - Use a well-defined bridge interface between C and Objective-C++

2. **Modular Build System**
   - Create separate components for core, renderer, input, and audio
   - Build each component separately and link them together

3. **Metal Shader Management**
   - Use the shader compilation and loading approach demonstrated in metal_minimal.mm
   - Add fallbacks for different Metal feature sets

4. **Apple Silicon Optimization**
   - Use unified memory where possible, as shown in the storage mode selection in the demos
   - Optimize texture updates for Apple GPUs using proper buffer management

## Usage Guide

### Building Demo Applications
```
# Build all demo applications
make -f makefile.metal

# Build specific demos
make -f makefile.metal testapp      # Basic window
make -f makefile.metal rendertest   # Animated graphics
make -f makefile.metal demo         # Animated gradient

# Clean build artifacts
make -f makefile.metal clean
```

### Structure of the Fixes

1. `metal_declarations.h` - Fixed header with proper extern "C" linkage
2. `metal_minimal.mm` - Simplified Metal renderer implementation
3. `metal_standalone_test.mm` - Basic Metal window demonstration
4. `metal_render_test.mm` - Advanced Metal rendering with animation

The demo apps prove that Metal is working correctly on your macOS/ARM64 system, providing a foundation for completing the full Metal renderer for FBNeo. 