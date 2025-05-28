# FBNeo Metal Renderer - Enhanced Implementation

This is an enhanced implementation of the Metal renderer for Final Burn Neo arcade emulator, specifically designed to improve frame buffer handling and fix rendering issues with CPS2 games like Marvel vs Capcom.

## Key Improvements

1. **Robust Frame Buffer Handling**
   - Properly connects FBNeo's `pBurnDraw` to Metal's rendering pipeline
   - Creates fallback buffers when the core doesn't provide valid pointers
   - Detects and handles frame buffer size changes

2. **Enhanced Error Diagnostics**
   - Detailed logging for frame buffer operations
   - Visual indicators for invalid frame buffers (red/yellow pattern)
   - Tracks and recovers from pointer disconnections

3. **Memory Management**
   - Properly allocates and deallocates frame buffers
   - Handles different pixel formats (16/24/32-bit)
   - Manages proper pitch values for image data

## Building

Use the enhanced build script to compile the Metal renderer:

```bash
./build_metal_final.sh
```

This creates the executable `fbneo_metal` with all required Metal components.

## Running

To run Marvel vs Capcom with the enhanced Metal renderer:

```bash
./run_mvsc.sh
```

This script includes additional logging and debugging options.

## Architecture

The implementation consists of several key components:

1. **metal_renderer_bridge.cpp/.h**
   - Bridges FBNeo's `pBurnDraw` to the Metal renderer
   - Manages frame buffer lifecycle
   - Provides pre/post-frame processing

2. **metal_bridge.mm**
   - Handles the Objective-C++/Metal side of the bridge
   - Creates and updates Metal textures
   - Manages the Metal view and rendering pipeline

3. **vid_metal.mm**
   - Implements core video interface
   - Manages resolution changes and display modes

## Troubleshooting

If you encounter rendering issues:

1. Check the logs in `debug_output/mvsc_metal_debug.log`
2. Look for "METAL_BRIDGE" log entries which indicate frame buffer status
3. If you see a red/yellow checkerboard pattern, that indicates the frame buffer from FBNeo is invalid

## Known Issues

- Some CPS1/CPS2 games may require specific timing adjustments
- Audio synchronization can sometimes lag behind video
- Some shader effects may not work with all games

## Future Improvements

- Better handling of different color depths
- Additional post-processing shader effects
- Performance optimizations for high-resolution scaling
- More robust error recovery mechanisms 