# CPS2 Emulation Integration Guide

This guide provides step-by-step instructions for integrating the full CPS2 emulation into the FBNeo Metal project.

## 1. Core Integration Steps

### Phase 1: Initial Setup (Completed)
- [x] Create the `metal_cps2_bridge.cpp` file
- [x] Update the `metal_renderer.mm` to support CPS2 mode
- [x] Modify `main.mm` to support switching between minimal and CPS2 modes
- [x] Update the `metal_renderer.h` header with new functions

### Phase 2: Memory Management Implementation
1. **Implement ROM memory allocation**
   - Hook up the `Metal_CPS2_AllocateMemory` function in `metal_cps2_bridge.cpp`
   - Connect to FBNeo core's memory variables

2. **Implement ROM loading functions**
   - Create a ROM loading system that works with CPS2 ROMs
   - Support loading and decryption of CPS2 ROMs

3. **Implement memory mapping**
   - Set up the CPS2 memory map for 68K, Z80, and other memory regions
   - Connect to FBNeo core's memory handlers

### Phase 3: CPU Core Integration
1. **Integrate M68000 core**
   - Ensure proper timing and synchronization
   - Connect to the FBNeo core's M68000 implementation

2. **Integrate Z80 core for sound**
   - Connect to the FBNeo core's Z80 implementation
   - Set up proper memory access for Z80

### Phase 4: Graphics Integration
1. **Implement CPS2 graphics rendering**
   - Connect to the FBNeo CPS2 rendering functions
   - Ensure proper drawing of sprites and tilemaps

2. **Implement CPS2 palette handling**
   - Connect to the FBNeo CPS2 palette system
   - Ensure proper color conversion for Metal

### Phase 5: Sound Integration
1. **Implement QSound emulation**
   - Connect to the FBNeo QSound implementation
   - Set up proper audio sample handling

2. **Implement audio mixing and output**
   - Connect to the Metal audio system
   - Ensure proper audio timing and synchronization

### Phase 6: Input System Integration
1. **Implement CPS2 input handling**
   - Map Metal input events to CPS2 inputs
   - Support different input configurations for different games

### Phase 7: Testing and Optimization
1. **Test with various CPS2 games**
   - Test with Marvel vs. Capcom, Street Fighter Alpha series, etc.
   - Fix game-specific issues

2. **Optimize performance**
   - Identify and address performance bottlenecks
   - Implement Metal-specific optimizations

## 2. Implementation Details

### CPS2 Memory Map
CPS2 uses the following memory map for the main 68K CPU:
- 0x000000-0x3FFFFF: Main ROM
- 0x800000-0x817FFF: RAM (Work RAM)
- 0x900000-0x92FFFF: Video RAM
- 0x980000-0x98FFFF: Palette RAM
- 0xF00000-0xF0FFFF: QSound communication
- 0xFF0000-0xFFFFFF: GFX configuration registers

### CPS2 Graphics Layers
CPS2 has several graphics layers that need to be rendered in the correct order:
1. Background layer (Scroll1)
2. Middle layer (Scroll2)
3. Foreground layer (Scroll3)
4. Sprite/Object layer

### Integration with Existing FBNeo Functions
The following FBNeo functions need to be integrated:
- `Cps2Init()`: Initialize the CPS2 system
- `CpsExit()`: Shut down the CPS2 system
- `Cps2Frame()`: Process a single frame of CPS2 emulation
- `Cps2LoadTiles()`: Load CPS2 graphics tiles
- `QsndInit()`: Initialize QSound
- `QsndReset()`: Reset QSound
- `QsndExit()`: Shut down QSound
- `QsndUpdate()`: Update QSound output

## 3. Testing Strategy

1. **Initial Integration Testing**
   - Test basic initialization and shutdown
   - Verify memory allocation and ROM loading

2. **Graphics Testing**
   - Test rendering of each layer
   - Verify sprite positioning and transparency
   - Check palette handling

3. **Audio Testing**
   - Test QSound sample playback
   - Verify audio synchronization

4. **Full Game Testing**
   - Test with Marvel vs. Capcom
   - Test with Street Fighter Alpha series
   - Test with other popular CPS2 games

## 4. Debugging Tips

1. **Graphics Debugging**
   - Use the Metal debug overlay to visualize individual layers
   - Compare output with known good emulators

2. **Memory Debugging**
   - Add logging for memory accesses at key addresses
   - Monitor ROM/RAM checksums

3. **Audio Debugging**
   - Log QSound commands
   - Monitor audio buffer contents

## 5. Next Steps (Immediate Tasks)

1. Complete the `Metal_CPS2_AllocateMemory` function implementation
2. Implement ROM loading for CPS2 games
3. Connect to the CPS2 rendering functions
4. Test basic rendering with Marvel vs. Capcom
5. Implement QSound integration

## 6. Resources

- `src/burn/drv/capcom/cps.cpp`: CPS core implementation
- `src/burn/drv/capcom/cps2_crpt.cpp`: CPS2 encryption
- `src/burn/drv/capcom/cps_run.cpp`: CPS2 main execution
- `src/burn/drv/capcom/cps_draw.cpp`: CPS2 rendering
- `src/burn/drv/capcom/qs.cpp`: QSound implementation
- `src/burn/drv/capcom/d_cps2.cpp`: CPS2 game definitions 