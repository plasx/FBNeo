# FBNeo Metal Rendering Pipeline

This document outlines the Metal rendering pipeline implementation for the FinalBurn Neo emulator on macOS.

## Architecture Overview

The Metal rendering pipeline consists of several key components:

1. **Resource Manager** - Handles lifecycle of all Metal resources
2. **Frame Manager** - Manages frame buffers and texture updates
3. **View Delegate** - Controls rendering cycle and display synchronization
4. **Shader Compilation** - Manages Metal shader compilation and loading
5. **Rendering Pipeline** - Defines the rendering pipeline state and draw commands

## Initialization Flow

The initialization sequence is as follows:

1. `MetalRenderer_Init` is called with a Metal device and MTKView
2. The Resource Manager is initialized with the device
3. Shader libraries are loaded from compiled .metallib files
4. Basic rendering resources (buffers, samplers) are created
5. Render pipeline state is created with appropriate vertex/fragment shaders
6. Frame textures are initialized with default dimensions
7. View delegate is set up to handle rendering cycle

## Frame Update and Presentation

The frame rendering process follows these steps:

1. `MetalRenderer_UpdateFrame` is called with new frame data
2. Frame Manager updates the texture with the new data using optimal transfer methods
3. For on-demand rendering, `setNeedsDisplay` is called on the Metal view
4. `MetalRenderer_Draw` is called by the view delegate when a frame should be presented
5. A command buffer and render pass are created
6. The frame texture is rendered to the view using a quad with bilinear or nearest filtering
7. The drawable is presented with VSync (or without for maximum performance)

## Synchronization and Triple Buffering

The pipeline implements triple buffering to ensure smooth frame presentation:

1. A semaphore controls access to in-flight command buffers (max 3)
2. Proper synchronization ensures GPU isn't overloaded
3. VSync can be enabled/disabled with proper frame pacing
4. Frame timing is monitored for consistent performance

## Resource Management

Resources are managed efficiently:

1. Textures use optimal storage modes based on device capabilities
2. Unified memory is detected and leveraged when available
3. Resources are properly released during shutdown
4. Memory allocation failures are handled gracefully

## Shader Management

Metal shaders are managed as follows:

1. Shaders are compiled during build into .metallib files
2. The makefile integrates Metal shader compilation
3. Compiled shaders are loaded at runtime for best performance
4. Fallback mechanisms handle missing shader libraries

## Platform-Specific Optimizations

The pipeline includes optimizations for various Mac hardware:

1. Apple Silicon optimizations with unified memory
2. Discrete GPU optimizations with private storage mode
3. Texture updates use the most efficient path for each platform
4. Resource usage is logged for debugging and optimization

## Error Handling

Robust error handling is implemented throughout:

1. Null pointer checks for all critical resources
2. NSError propagation for Metal API errors
3. Fallback mechanisms for critical failures
4. Detailed error logging

## Usage

To use the Metal renderer:

1. Initialize with `MetalRenderer_Init(device, view)`
2. Call `MetalRenderer_UpdateFrame(frameData, width, height)` with new frame data
3. For manual control, call `MetalRenderer_Draw()` to present the current frame
4. Configure with `MetalRenderer_SetOptions(vsync, bilinear_filter, fullscreen)`
5. Shutdown with `MetalRenderer_Shutdown()`

## Advanced Features

The pipeline supports several advanced features:

1. Scanline rendering for CRT effect
2. Scaling and filtering options
3. Triple buffering for smooth animation
4. Adaptive frame pacing for optimal performance

## Debugging

For debugging:

1. Resource usage reports via `[resourceManager logResourceUsage]`
2. Frame timing logs every 60 frames
3. GPU time measurements
4. Clear error messages for common issues

## Performance Considerations

Key performance optimizations:

1. Use of private storage mode on discrete GPUs
2. Shared storage mode on unified memory devices
3. Minimized CPU-GPU synchronization points
4. Command buffer reuse
5. Efficient texture update paths

## Known Issues and Workarounds

1. **Pink Screen Issue**: If you see a pink screen, it usually indicates shader compilation failure. Check that .metallib files are being generated properly.
2. **Black Screen**: Could indicate a rendering pipeline initialization failure or incorrect view configuration.
3. **Performance Issues**: Disable continuous rendering and enable VSync for more stable performance on older hardware. 