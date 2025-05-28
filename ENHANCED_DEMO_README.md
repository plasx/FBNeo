# FBNeo Metal Enhanced Demo

This is an enhanced demonstration of the FBNeo Metal backend with realistic rendering and animation capabilities.

## Overview

Unlike the original stub implementation, this enhanced demo:

1. Shows 60 frames of animation with smooth transitions
2. Renders a CPS2-style grid pattern
3. Displays animated sprites using sine/cosine functions for motion
4. Shows game title and frame counter
5. Simulates an actual CPS2 game visually
6. Implements proper memory management

## Building

To build the enhanced demo:

```bash
./build_demo.sh
```

This creates a binary in `bin/metal/fbneo_metal` and a symlink in the root directory.

## Running

To run the demo:

```bash
./fbneo_metal /path/to/rom.zip
```

Example:

```bash
./fbneo_metal /Users/username/ROMs/mvsc.zip
```

Note: The ROM file doesn't actually need to be a valid CPS2 ROM as the demo uses stub implementations.

## Implementation Details

The enhanced demo consists of:

- `metal_demo_main.c` - Main entry point that runs the 60-frame animation
- `metal_demo_stubs.c` - Implementation of the core functions with animation
- `metal_renderer_stubs.c` - Stubs for the Metal rendering API

The demo simulates the structure of the CPS2 rendering system by:

1. Generating a grid-based background
2. Drawing an animated circular sprite
3. Displaying game information text

The animation works by:
- Incrementing a frame counter
- Using sine/cosine functions to move sprites
- Updating the display each frame
- Adding a small delay between frames for visibility

## Next Steps

This demo can be extended by:
- Adding more complex sprite animations
- Implementing actual input handling
- Adding sound effects
- Creating more realistic game elements
- Connecting to the actual Metal rendering API instead of stubs 