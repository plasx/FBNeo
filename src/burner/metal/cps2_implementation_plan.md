# CPS2 Full Emulation Implementation Plan for FBNeo Metal

## Overview
This document outlines the approach to integrate full CPS2 emulation into the FBNeo Metal project, replacing the minimal simulation currently in place. The goal is to have accurate CPS2 emulation while maintaining good performance on macOS.

## Current Status
- Currently using `metal_minimal_core.cpp` which provides a simulation of Marvel vs. Capcom
- Basic Metal bridge is in place (`metal_bridge_simple.cpp`)
- The original FBNeo CPS2 driver (`d_cps2.cpp`) exists but is not integrated

## Implementation Phases

### Phase 1: Core Integration

1. **Create CPS2 Core Wrapper**
   - Create a bridge between Metal and the core CPS2 emulation components
   - Implement proper memory management for ROM data
   - Add support for the CPS2 encryption/decryption

2. **ROM Loading**
   - Integrate the ROM validation system with actual CPS2 ROM loading
   - Implement CPS2 ROM set detection and validation
   - Support loading of MAME-compatible CPS2 ROM sets

3. **CPU Emulation**
   - Integrate M68000 CPU core with proper timing
   - Integrate Z80 core for sound
   - Implement proper CPU synchronization

### Phase 2: Graphics Subsystem

1. **Graphics Rendering**
   - Integrate CPS2 graphics rendering into Metal pipeline
   - Implement CPS2 sprite rendering
   - Implement CPS2 tilemap rendering (3 layers)
   - Implement CPS2 palette handling

2. **Video Output**
   - Support 384x224 resolution (CPS2 native)
   - Implement scaling and filtering options
   - Add support for VSync

3. **Performance Optimizations**
   - Use Metal compute shaders for graphics processing
   - Optimize memory usage and transfers
   - Implement partial screen updates

### Phase 3: Sound Subsystem

1. **QSound Emulation**
   - Integrate QSound emulation
   - Implement sample loading and processing
   - Support stereo sound output

2. **Audio Output**
   - Implement audio buffer management
   - Synchronize audio with gameplay
   - Add volume control and settings

### Phase 4: Input System

1. **Input Handling**
   - Support gamepad input for CPS2 games
   - Implement keyboard mapping
   - Add input configuration UI

2. **Game-specific Controls**
   - Support different control schemes based on game type
   - Implement 6-button fighting game layout
   - Support for 2+ players

### Phase 5: Testing and Optimization

1. **Game Compatibility Testing**
   - Test core CPS2 games (Street Fighter, Marvel vs Capcom, etc.)
   - Fix game-specific issues
   - Verify performance across different hardware

2. **Final Optimizations**
   - Implement threading model for multicore CPUs
   - Optimize Metal shader performance
   - Add advanced CRT shader effects

## Technical Details

### Memory Map Integration
The CPS2 system uses a complex memory map that needs to be properly implemented:
- 0x000000-0x3FFFFF: Main 68K ROM (4MB)
- 0x800000-0x81FFFF: Main RAM (128KB)
- 0x900000-0x92FFFF: Video RAM
- 0xFF0000-0xFFFFFF: Cache RAM

### Core Integration Functions
To properly integrate with the existing core, we need to:
1. Replace dummy implementations in `metal_minimal_core.cpp` with hooks to real emulation
2. Properly handle memory allocation and deallocation
3. Implement save state functionality

### Graphics Rendering Pipeline
The CPS2 graphics rendering should use the following pipeline:
1. CPS2 core renders to framebuffer
2. Metal converts framebuffer to texture
3. Metal renders texture with optional post-processing
4. Present final frame to screen

## Timeline

- **Week 1-2**: Core CPU integration and memory map
- **Week 3-4**: Graphics rendering and palette handling
- **Week 5**: Sound emulation
- **Week 6**: Input system and testing
- **Week 7-8**: Optimization and polish

## Required Components

1. **Core Files**:
   - `src/burn/drv/capcom/cps.cpp`: Core CPS implementation
   - `src/burn/drv/capcom/cps2_crpt.cpp`: CPS2 encryption
   - `src/burn/drv/capcom/cps_run.cpp`: CPS2 main execution
   - `src/burn/drv/capcom/cps_draw.cpp`: CPS2 rendering
   - `src/burn/drv/capcom/qs.cpp`: QSound implementation

2. **Metal Integration**:
   - `src/burner/metal/metal_cps2_bridge.cpp`: New bridge file for CPS2
   - `src/burner/metal/metal_renderer.mm`: Updates for CPS2 rendering
   - `src/burner/metal/metal_input.mm`: Updates for CPS2 input
   - `src/burner/metal/metal_audio.mm`: Updates for QSound

3. **Supporting Files**:
   - CPU cores (68K, Z80)
   - Memory management
   - Input handling

## Testing Strategy

1. Start with basic rendering test (known good state)
2. Test CPU execution with simple CPS2 games
3. Test sound with known reference samples
4. Full game testing with popular titles
5. Performance testing across different Mac hardware 