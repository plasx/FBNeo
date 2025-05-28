# FBNeo Metal Renderer Implementation

## Overview

This document describes the implementation of the Metal renderer for FBNeo on macOS, with a focus on:
1. Audio implementation using CoreAudio
2. Build system integration
3. Performance optimization features

## Audio Implementation

The Metal renderer includes a complete audio implementation using CoreAudio/AVAudioEngine for high-quality, low-latency audio playback. 

### Core Components

- **AVAudioEngine Integration**: Uses Apple's modern audio framework for efficient playback with minimal overhead.
- **RingBuffer Implementation**: Thread-safe ring buffer design prevents audio skipping during gameplay.
- **Audio Synchronization**: Adaptive buffer management keeps audio and video in sync.
- **Per-Game Settings**: Supports different audio settings on a per-game basis.

### Audio Bridge Architecture

The audio system is implemented with multiple layers:

1. **Core Layer**: `metal_audio_arm64.mm` - Optimized CoreAudio implementation for Apple Silicon.
2. **Integration Layer**: `metal_audio_integration.mm` - Bridges between Metal and FBNeo core.
3. **Bridge Layer**: `metal_audio_bridge.cpp` - Implements FBNeo audio interface functions.

### Audio Features

- **Sample Rate Selection**: Supports 44.1kHz and 48kHz output.
- **Volume Control**: Smooth volume control through AVAudioEngine's mixer.
- **Pause/Resume**: Clean pausing without audio artifacts.
- **Latency Measurement**: Built-in tools to monitor and optimize audio latency.
- **Performance Monitoring**: Audio CPU usage tracking to identify bottlenecks.

## Build System Integration

The build system is fully integrated with FBNeo's codebase through a customized makefile.

### Key Files

- `makefile.metal`: Main build script for the Metal renderer.
- `build_and_run.sh`: Helper script for quick building and testing.

### App Bundle Structure

The build process creates a proper macOS application bundle with:

1. Standard structure (`Contents/MacOS`, `Contents/Resources`)
2. Info.plist with necessary permissions and settings
3. Embedded shaders and resources
4. XIB files for UI components

### Building the Metal Renderer

To build the Metal renderer:

```bash
# Basic build
make -f makefile.metal

# Enhanced build with all features
make -f makefile.metal enhanced

# Quick build and run
./build_and_run.sh [optional_rom_path]
```

## Performance Optimization

The Metal renderer includes comprehensive performance monitoring and optimization capabilities.

### Performance Counters

The `metal_performance_counters` system provides:

- **Frame Time Tracking**: Monitors CPU and GPU frame times.
- **Memory Usage**: Tracks texture and buffer memory consumption.
- **Draw Call Analysis**: Counts draw calls per frame to identify bottlenecks.
- **CSV Export**: Detailed logging for offline analysis.

### Apple Silicon Optimization

The renderer is specially optimized for Apple Silicon:

1. **Shared Memory Mode**: Uses unified memory for zero-copy texture updates.
2. **Metal Shader Caching**: Precompiles shaders for faster startup.
3. **ARM64 Optimizations**: Uses ARM NEON instructions where appropriate.

### Debugging Tools

- **Performance Overlay**: Optional FPS and timing display.
- **Metal Frame Capture**: Integration with Xcode's Metal frame debugger.
- **Logging System**: Comprehensive logging of performance metrics.

## Testing Guidelines

For optimal performance and compatibility:

1. **ROM Compatibility**: Test various ROM formats and game types.
2. **Performance Profiling**: Use the built-in metrics to identify bottlenecks.
3. **Audio Sync**: Test audio/video synchronization across different games.
4. **Memory Usage**: Monitor texture memory for large ROMs.

## Known Issues and Limitations

- Some very complex shader effects may be slower on older Intel Macs.
- Audio latency can vary slightly between macOS versions.
- Performance counters have minimal overhead but can be disabled for maximum performance.

## Future Improvements

Planned enhancements for future versions:

1. **Enhanced Shaders**: Additional CRT and scaling options.
2. **Input Improvements**: Support for more controller types.
3. **Performance Optimization**: Further Apple Silicon tuning.
4. **Integration**: Deeper integration with macOS features.

## Acknowledgments

The Metal renderer builds on the work of the FBNeo team and incorporates modern macOS audio, graphics, and input technologies. 