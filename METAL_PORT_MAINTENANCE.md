# FBNeo Metal Port Maintenance Guide

This document outlines how to maintain and future-proof the Metal port of FBNeo for macOS.

## Overview

The Metal port provides a native macOS implementation of FBNeo that uses Apple's Metal graphics API. This implementation:
- Uses modern Apple APIs (Metal, CoreAudio, GameController)
- Works well on Apple Silicon and modern Intel Macs
- Provides full emulation of CPS2 games like Marvel vs. Capcom
- Avoids deprecated code paths and legacy fallbacks

## Build Instructions

To build the Metal port:

```bash
# Clean build
make -f makefile.metal clean

# Build using all available cores
make -f makefile.metal -j$(sysctl -n hw.ncpu)

# Run with a ROM file
./fbneo_metal /path/to/rom.zip
```

## Recent Cleanups and Improvements

The following improvements have been made to the Metal port:

1. **Legacy Code Elimination**:
   - Removed SDL fallback rendering code
   - Removed X11 window handling dependencies
   - Removed CPS stubs (`METAL_CPS_STUB`)
   - Removed test pattern generation code
   - Cleaned up deprecated macOS API usage

2. **Shader Simplification**:
   - Simplified Metal shader compilation in the makefile
   - Fixed Metal shader standard version references

3. **Code Documentation**:
   - Added clear commenting throughout the codebase
   - Created this maintenance guide

## Preventing Regressions

### Legacy Code Elimination

The following legacy code and workarounds have been removed:

- **SDL Fallbacks**: All SDL fallback rendering has been removed
- **X11 Dependencies**: No X11 window handling code is present in the Metal port
- **CPS Stubs**: The `METAL_CPS_STUB` implementation has been removed
- **Test Patterns**: Test pattern generation has been replaced with real ROM rendering
- **Deprecated APIs**: Outdated macOS APIs have been removed or updated

### Current macOS API Usage

The port uses modern frameworks available in macOS 14+:

- **Metal/MetalKit**: For graphics rendering
- **AVFoundation/AudioToolbox/CoreAudio**: For audio processing
- **GameController**: For controller input
- **Cocoa**: For native UI elements

### Automatic Reference Counting (ARC)

The implementation uses Objective-C++ with ARC enabled:
- Simplifies memory management
- Avoids leaks in Objective-C/Cocoa code
- Uses `-fobjc-arc` flag in the makefile

## Regression Testing

When updating FBNeo core (e.g., v1.0.0.4, etc.), test the Metal build with these games:

1. **CPS2 Games**:
   - Marvel vs. Capcom (`mvsc.zip`)
   - Street Fighter Alpha 3 (`sfa3.zip`)
   - Darkstalkers 3 (`dstlk.zip`)

2. **Neo Geo Games**:
   - King of Fighters '98 (`kof98.zip`)
   - Metal Slug X (`mslugx.zip`)

3. **Other Systems**:
   - Test at least one game from another system (CPS1, etc.)

### Test Areas

For each game, verify:

1. **Graphics**: No visual glitches, correct colors/scaling
2. **Audio**: Clear sound without crackling, proper volume
3. **Input**: Keyboard and controller input works as expected
4. **Save States**: Save/load states work properly (F5/F8 keys)
5. **Performance**: Game runs at full speed with stable framerate

## Performance Monitoring

The Metal port should run at full speed on Apple Silicon. If performance issues arise:

1. **GPU Frame Capture**: Use Xcode's GPU Frame Capture to identify bottlenecks
2. **Debug Overlay**: Enable the debug overlay to monitor FPS and frame time
3. **Buffer Management**: Consider adjusting buffer sizes/counts if needed
4. **Texture Usage**: Ensure textures are uploaded efficiently

## Maintenance Tasks

### When Updating FBNeo Core

1. Rebuild with `-f makefile.metal` and check for compiler warnings/errors
2. Ensure any new core interfaces are properly connected to Metal implementation
3. Test save states across different games to ensure compatibility
4. Check that audio still works correctly with the updated sound core

### For macOS API Changes

1. Check for deprecated API warnings during compilation
2. Update to newer APIs when Apple deprecates current ones:
   - If AudioQueue is deprecated, migrate to AVAudioEngine
   - If GameController APIs change, update to the newest version
   - Stay current with Metal API changes and best practices

### Metal Shader Updates

If you need to update the Metal shaders:

1. The shaders are located in `src/burner/metal/DefaultShader.metal` and `src/burner/metal/enhanced_metal_shaders.metal`
2. When modifying shaders, make sure to:
   - Use macOS-compatible Metal code
   - Test on both Apple Silicon and Intel Macs
   - Verify performance impact

## Known Issues and Future Improvements

1. **Metal Shader Compilation**: The current build temporarily skips shader compilation. To fix this:
   - Review the Metal shader code for compatibility issues
   - Update the shader compilation in the makefile to use correct Metal standards
   - Consider using a simpler shader approach for better compatibility

2. **Audio Latency**: Could be improved with more advanced buffer management

3. **Save State Support**: More comprehensive state serialization

4. **Enhanced Shaders**: Additional Metal shaders for CRT effects, etc.

5. **UI Improvements**: Native macOS preferences UI

## Reporting Issues

If you encounter issues with the Metal port, please report them with:

1. macOS version
2. Hardware details (Apple Silicon/Intel)
3. Steps to reproduce
4. Expected vs. actual behavior
5. Any console output or crash logs 