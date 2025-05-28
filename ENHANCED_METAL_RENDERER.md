# FBNeo Enhanced Metal Renderer

This document describes the enhanced Metal renderer implementation for FBNeo on macOS/Apple Silicon.

## Overview

The enhanced Metal renderer provides the following improvements over the basic implementation:

1. **Proper Aspect Ratio Handling** - Maintains the correct aspect ratio for different games
2. **Optimized Color Conversion** - Efficiently converts between different color formats
3. **Texture Management** - Properly manages Metal textures with optimal settings for Apple Silicon
4. **Shader Options** - Support for different visual effects via configurable shaders
5. **Metal Performance Optimizations** - Uses Apple-recommended practices for best performance

## Implementation Details

### Core Components

1. **FBNeoMetalRenderer Class** - Objective-C++ class that handles all Metal rendering
2. **Metal_RenderFrame Function** - C-compatible function that bridges between FBNeo core and Metal
3. **DefaultShader.metal** - Metal shader for rendering the emulator's frame buffer

### Color Conversion

The renderer supports all common FBNeo color formats and properly converts them to Metal's BGRA8 format:

- 15-bit RGB555
- 16-bit RGB565
- 24-bit RGB888
- 32-bit ARGB8888

### Aspect Ratio Preservation

The renderer calculates the proper display rectangle to maintain the game's aspect ratio:

1. Gets the original game dimensions (width, height)
2. Calculates the aspect ratio (e.g., 4:3, 16:9, etc.)
3. Adjusts the rendering quad to fit the window while preserving aspect ratio
4. Adds letterboxing or pillarboxing as needed

### Shader System

The renderer supports multiple shader types:

- **Basic** - Simple pass-through shader (default)
- **CRT** - Simulates a CRT display with scanlines
- **Scanlines** - Only adds scanlines effect
- **HQ2X** - High-quality 2x scaling filter
- **Custom** - Supports loading custom shaders from metallib file

## Usage

### Building the Enhanced Renderer

```bash
# Use the provided build script
./build_and_run_enhanced.sh [rompath]

# Or build manually
make -f makefile.metal enhanced
./fbneo_metal_enhanced [rompath]
```

### Runtime Options

The enhanced renderer supports the following runtime options that can be configured via keyboard shortcuts or UI:

- **Toggle Aspect Ratio Preservation** (Alt+A)
- **Change Shader Type** (Alt+S cycles through shaders)
- **Toggle Bilinear Filtering** (Alt+B)
- **Toggle Fullscreen** (Alt+Enter or Cmd+F)

## Future Enhancements

1. **Additional Shaders** - More visual effects and post-processing options
2. **Metal Performance Shaders** - Use Apple's MPS for optimized scaling and effects
3. **Dynamic Resolution Scaling** - Adjust rendering resolution based on window size
4. **MetalFX Integration** - Add support for MetalFX upscaling on Apple Silicon
5. **Multiple Monitor Support** - Better handling of multiple displays

## Technical Notes

### Metal Storage Modes

The renderer uses the optimal storage mode for Apple Silicon:

- `MTLStorageModeShared` - For textures that are updated frequently
- Uses texture replacement region for efficient updates

### Vertex Buffer

The vertex buffer represents a quad for rendering with proper aspect ratio adjustments:

```
float quadVertices[] = {
    // Positions          Texture Coordinates
    left,  top,           0.0, 0.0,  // top left
    left,  bottom,        0.0, 1.0,  // bottom left
    right, bottom,        1.0, 1.0,  // bottom right
    
    right, bottom,        1.0, 1.0,  // bottom right
    right, top,           1.0, 0.0,  // top right
    left,  top,           0.0, 0.0   // top left
};
```

The positions are calculated based on the aspect ratio of the game vs. the window.

### Performance Considerations

1. **Buffer Reuse** - Static buffers are reused to avoid frequent allocations
2. **Shared Memory** - Uses shared memory mode for best performance on Apple Silicon
3. **Single-Pass Rendering** - Uses a single draw call for rendering the frame 