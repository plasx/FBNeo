# FBNeo Metal Frame Pipeline Debugging Tools

This document describes the tools and utilities for debugging the frame data pipeline in the Metal port of FBNeo.

## Overview

The Metal port of FBNeo requires several steps to properly display game frames:

1. The FBNeo core generates raw frame data in memory
2. This data must be converted to a Metal-compatible format (typically RGBA8)
3. The data is uploaded to a Metal texture
4. The texture is drawn to the screen

If any of these steps fail, you'll see a blank screen or corrupt display. These tools help identify where problems occur.

## Tools Included

### 1. Metal Debug Application (`fbneo_metal_debug`)

A standalone test application that creates test patterns, analyzes frame buffer content, and verifies texture updates using Metal.

**Building:**
```bash
make -f makefile.metal debugapp
```

**Running:**
```bash
./fbneo_metal_debug [width] [height]
```

This tool:
- Generates various test patterns (color bars, gradients, etc.)
- Analyzes frame buffer content for non-zero bytes and checksums
- Updates Metal textures with the frame data
- Saves a copy of frame data for inspection

### 2. Enhanced Frame Buffer Analysis

We've added detailed debugging to the frame pipeline in `metal_bridge.cpp` and `metal_implementation.mm`:

- Frame buffer content verification
- Non-zero byte counting and checksums
- Pixel format validation
- Texture update verification
- RGBA component analysis

### 3. Test Pattern Generator

The `Metal_GenerateTestPattern` function produces standardized test patterns for debugging:
- Color bars (pattern 0)
- Gradient patterns (pattern 1)
- Animated checkerboard (pattern 2)
- Border pattern with text (pattern 3)

### 4. Verification Script (`metal_frame_verification.sh`)

A shell script that automates testing of the frame pipeline:

**Running:**
```bash
./src/burner/metal/metal_frame_verification.sh [optional_rom_path]
```

The script:
- Runs the debug application
- Attempts to run the full emulator with a ROM if available
- Compares results and analyzes frame buffer content
- Provides a summary and recommendations

## Diagnosing Common Issues

### Blank Screen
If you see a blank screen:
1. Check if the frame buffer is being filled (look for non-zero bytes in logs)
2. Verify texture dimensions match the frame dimensions
3. Ensure the texture is being updated correctly
4. Check if the view is redrawn after updates

### Corrupt Display
If you see corrupted graphics:
1. Verify pixel format conversion (RGBA vs BGRA)
2. Check if the correct stride/pitch is used
3. Ensure the bytes per pixel calculation is correct
4. Verify texture and frame buffer dimensions match

### Flickering
If the display flickers:
1. Check if the view is redrawn too frequently
2. Verify synchronization between Metal commands
3. Ensure proper Metal command completion before presentation

## Output Files

The frame verification process creates:
- `frame_debug/debug_output.log`: Output from the debug application
- `frame_debug/frame_debug.bin`: Raw frame buffer data for analysis
- `frame_debug/full_emu_output.log`: Output from the full emulator test (if run)

## Next Steps

If issues persist:
1. Add more detailed logging to the specific problematic area
2. Implement frame-by-frame debugging with save/restore capability
3. Add visual indicators for frame updates in the rendered output
4. Consider pixel format conversion verification tools 