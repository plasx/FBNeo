# Frame Loop and Video Output Implementation for FBNeo Metal

This document summarizes the implementation of the frame loop and video output for the FBNeo Metal emulator.

## Overview

The frame loop implementation connects the emulation core with the Metal renderer to display the game output. The key components are:

1. The emulation timer (`emulation_timer_callback`) that drives the frame loop
2. The `Metal_RunFrame` function that runs a single frame of emulation
3. The Metal renderer that displays the frame buffer on screen

## Key Implementation Details

### 1. Frame Buffer Setup

- Created a proper frame buffer initialization system that:
  - Allocates memory for the frame buffer based on game dimensions
  - Sets up a 32-bit BGRA8888 color format for compatibility with Metal
  - Configures `pBurnDraw`, `nBurnPitch`, and `nBurnBpp` for the FBNeo core

### 2. Emulation Frame Loop

- Modified `Metal_RunFrame` to:
  - Initialize frame buffer settings before each frame
  - Process player input before running the frame
  - Run the emulation frame using `BurnDrvFrame()`
  - Convert the output to the format expected by Metal (BGRA8888)
  - Set a flag to indicate the frame buffer has been updated

### 3. Metal Rendering

- Updated the Metal renderer to:
  - Check for updated frame data in each render cycle
  - Copy the emulation frame buffer to the Metal texture
  - Render the texture to the screen as a full-screen quad
  - Avoid any test pattern fallbacks and always use real emulation output

### 4. Frame Loop Connection

- Connected the frame loop with the Metal View:
  - Set the MTKView to render at 60 FPS (matching the game's frame rate)
  - Ensured `drawInMTKView` updates the texture with the latest emulation output
  - Used the emulation timer to drive the frame execution

## Debug Features

- Added debug checks to verify that:
  - The frame buffer contains valid data (using checksums)
  - The dimensions are correct for the game
  - The renderer is receiving and displaying the proper frame data

## Testing and Verification

To test the implementation:

1. Run the emulator with Marvel vs Capcom ROM
2. Verify that the game graphics are displayed correctly
3. Ensure stable frame rate and proper synchronization
4. Check for any graphical glitches or artifacts

## Remaining Improvements

- Add frame rate throttling to ensure consistent 60 FPS
- Implement additional video filters and effects
- Add CRT simulation and scanline options
- Support for different aspect ratios and display modes

## Summary of Changes

1. **Removed Test Patterns**
   - Deprecated `Metal_GenerateTestPattern` function
   - Removed all fallbacks to test patterns in rendering code
   - Updated frame processing to always use the real emulation frame buffer

2. **Connected Frame Loop to Emulation**
   - Modified `drawInMTKView` in `MetalViewDelegate` to call `Metal_RunFrame` on each frame
   - Set `preferredFramesPerSecond` to 60 FPS for CPS2 games
   - Ensured continuous rendering by setting `_metalView.paused = NO`

3. **Enhanced Frame Buffer Handling**
   - Created `InitFrameBufferAndEmulationSettings` function to properly set up frame buffer pointers
   - Updated `Metal_RunFrame` to use the new initialization function
   - Ensured `pBurnDraw`, `nBurnPitch`, and `nBurnBpp` are correctly set before each frame

4. **Optimized Metal Rendering**
   - Updated `MetalRenderer_UpdateFrame` to efficiently copy frame data to Metal textures
   - Optimized texture updates for Apple Silicon with unified memory
   - Added proper error handling for texture creation and updates

5. **Fixed Frame Presentation**
   - Ensured Metal draws the real frame buffer without any decorations or test patterns
   - Configured the Metal view for proper vsync and frame timing
   - Removed any frame count limits that were in place for testing

## Implementation Details

### Frame Loop Integration

The frame loop is now driven by the Metal renderer through MTKView's refresh cycle. On each vsync pulse:

1. `drawInMTKView` is called by the Metal framework
2. `Metal_RunFrame(true)` is called to advance emulation by one frame
3. The emulation output is copied to the Metal texture
4. The texture is rendered to screen via Metal rendering pipeline

### Frame Buffer Management

To ensure proper rendering, we've implemented a robust frame buffer management system:

```c
// Initialize frame buffer and emulation settings
InitFrameBufferAndEmulationSettings();

// Set global pointers for the emulator core
pBurnDraw = pBurnDraw_Metal;
nBurnPitch = nBurnPitch_Metal;
nBurnBpp = nBurnBpp_Metal;

// Run a frame of emulation
BurnDrvFrame();

// Update the Metal texture with the frame buffer
UpdateMetalFrameTexture(pBurnDraw_Metal, width, height);
```

### Performance Considerations

- Used unified memory on Apple Silicon for zero-copy texture updates
- Implemented efficient blit operations for discrete GPUs
- Reduced debug logging to minimize overhead
- Ensured frame timing matches the original arcade hardware (60 FPS)

## Running the Implementation

1. Build the emulator using the `build_mvsc.sh` script
2. Copy the Marvel vs Capcom ROM (`mvsc.zip`) to the `roms` directory
3. Run the emulator: `./fbneo_metal_core roms/mvsc.zip`

## Known Issues and Future Improvements

1. **Input Latency**: There may be some input latency due to the vsync timing. Future improvements could include input prediction or frame interpolation.

2. **Performance Monitoring**: Currently lacks performance metrics. Future versions should include FPS counter and frame timing statistics.

3. **Scaling Options**: Currently renders at native resolution. Future improvements could include integer scaling and CRT simulation options.

4. **Frame Skipping**: No frame skipping is implemented yet. This could be added for lower-end hardware if needed.

## Testing and Validation

To validate the implementation:
- Check that Marvel vs Capcom intro and character select screens display correctly
- Verify animations run at the correct speed (60 FPS)
- Ensure no test patterns or debug borders appear during gameplay
- Confirm that game visuals match reference footage of the arcade original 