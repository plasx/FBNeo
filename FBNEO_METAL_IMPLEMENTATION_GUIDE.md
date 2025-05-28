# FBNeo Metal Implementation Guide

This guide documents the approach for implementing a Metal frontend for FBNeo on macOS, specifically for running CPS2 games like Marvel vs Capcom.

## Implementation Strategy

The implementation follows a modular approach with clear separation of concerns to avoid duplicate symbols and manage complexity:

### 1. Core Components

- **Unified Error Handling**: Centralized error reporting and logging
- **Frame Buffer Management**: Handling conversion between emulator data and Metal texture
- **Metal Rendering**: Shader-based rendering of game graphics
- **Input Handling**: Keyboard and gamepad input mapping
- **Audio Processing**: Sound buffer management

### 2. Build Organization

- Separate compilation of C, C++, and Objective-C++ files
- Unified header files to ensure consistent declarations
- Careful management of linkage between languages
- Minimal dependencies between components

## Implementation Details

### Frame Buffer Management

The frame buffer system serves as the bridge between the emulator's internal rendering and Metal:

1. FBNeo renders to `pBurnDraw`, a raw pixel buffer
2. Our bridge code converts this to a Metal-compatible texture format
3. Test patterns can be generated to validate the rendering pipeline

### Error Handling

A unified error handling system provides:

- Error codes and messages for different subsystems
- Debug logging with configurable verbosity levels
- Runtime diagnostics for troubleshooting

### Metal Rendering

The Metal rendering pipeline consists of:

- Vertex and fragment shaders for basic texture display
- A bridge layer to manage texture updates
- Optional post-processing effects (scanlines, CRT curvature)

### Simplified Implementation

For initial development and testing, a simplified implementation has been created:

```
fbneo_minimal          - A basic Cocoa app with Metal view
build_minimal.sh       - Minimal build script for testing
build_simplified.sh    - More complete but still simplified build
```

### Full Implementation

The full implementation includes:

```
fbneo_metal            - Complete emulator with Metal frontend
build_final_resolved.sh - Build script with duplicate symbol resolution
run_mvsc_resolved.sh   - Script to run Marvel vs Capcom
```

## Troubleshooting

Common issues and solutions:

1. **Duplicate Symbols**: Carefully manage which files implement which functions
2. **Shader Compilation**: Ensure Metal shaders are compiled and available at runtime
3. **ROM Loading**: Verify ROM paths and use diagnostic logging

## Next Steps

Future development should focus on:

1. Enhancing the renderer with more advanced effects
2. Improving gamepad support
3. Adding a configuration UI
4. Implementing save states
5. Optimizing performance

## References

- [Metal Programming Guide](https://developer.apple.com/documentation/metal)
- [FBNeo Documentation](https://github.com/finalburnneo/FBNeo)
- [CPS2 Technical Information](https://wiki.arcadeotaku.com/w/CPS-2) 