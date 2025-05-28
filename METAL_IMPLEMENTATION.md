# FBNeo Metal Renderer Implementation

This document summarizes the implementation of the Metal renderer for FBNeo on macOS.

## 1. Metal Renderer Integration

### Rendering Pipeline

The Metal renderer implementation provides:

- **Efficient Rendering**: High-performance Metal API usage for hardware-accelerated graphics
- **Multiple Color Format Support**: Handles RGB565, RGB555, RGB888, and RGBA8888 frame buffers
- **Automatic Format Conversion**: Converts from various input formats to Metal-compatible RGBA8888
- **Double-Buffering**: To prevent tearing and ensure smooth animation

### Shader Options

The renderer supports multiple shader options:

- **Standard**: Basic bilinear filtering for smooth scaling
- **Pixel Perfect**: Nearest-neighbor filtering for crisp pixel art
- **Scanlines**: Classic scanline effect for retro feel
- **CRT Emulation**: Simulates CRT display with curvature, scanlines, and aperture grille
- **Enhanced**: AI-assisted upscaling with detail preservation (if CoreML is available)

## 2. Aspect Ratio Handling

Multiple aspect ratio modes are supported:

- **Original**: Maintains the game's original aspect ratio with letterboxing/pillarboxing
- **Stretched**: Fills the entire window by stretching the image
- **Pixel Perfect**: Integer scaling to maintain crisp pixels

The implementation dynamically adjusts the vertex buffer to handle various display dimensions and aspect ratios.

## 3. Input Handling

### Keyboard Support

- **Native macOS Keyboard Events**: Captures and processes key events through custom NSView
- **Key Mapping**: Maps macOS key codes to FBNeo input codes
- **Modifier Key Support**: Handles shift, control, option, and command keys
- **Multi-Key Support**: Processes multiple simultaneous key presses

### Game Controller Support

- **GameController Framework**: Uses Apple's GameController framework for controller support
- **Controller Mapping**: Maps game controller buttons to FBNeo inputs
- **Multiple Controller Support**: Supports up to 4 controllers for multiplayer
- **Per-Game Mappings**: Allows saving and loading controller mappings per game

## 4. Window Management

The Metal renderer integrates with macOS window management:

- **Proper Window Creation**: Creates standard macOS windows with title bar and controls
- **Full-Screen Support**: Toggles between windowed and full-screen modes
- **Resolution Handling**: Adapts to different display resolutions
- **Resizable Windows**: Properly handles window resizing with aspect ratio preservation

## 5. Performance Features

Performance optimization features include:

- **Performance Counters**: Tracks CPU and GPU frame times
- **Memory Usage Monitoring**: Monitors texture and buffer memory usage
- **Apple Silicon Optimization**: Uses shared memory for efficient transfers on Apple Silicon
- **Render State Caching**: Minimizes state changes for better performance

## 6. Integration with FBNeo Core

The Metal renderer seamlessly integrates with the FBNeo core:

- **Frame Buffer Synchronization**: Efficiently transfers frame data from emulator to GPU
- **Input System Compatibility**: Properly maps inputs to FBNeo's input system
- **Configuration Integration**: Respects FBNeo's configuration settings

## Implementing the Metal Renderer

The Metal renderer is implemented through several key components:

1. **FBNeoMetalRenderer**: Main class that handles rendering, shader management, and texture updates
2. **Metal Shader Library**: Contains various shaders for different visual effects
3. **Input Handlers**: Process keyboard and controller input
4. **C-Compatible Interface**: Bridges between Objective-C++ and C code for FBNeo integration

## Future Enhancements

Potential future enhancements could include:

- **More Advanced Shaders**: Additional post-processing effects
- **CoreML-Enhanced Upscaling**: More sophisticated AI-assisted rendering
- **Additional Controller Types**: Support for specialized arcade controllers
- **Performance Optimizations**: Further tuning for specific Apple hardware 