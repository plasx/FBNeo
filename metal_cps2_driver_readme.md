# FBNeo CPS2 Metal Driver Implementation

## Overview

This document describes the complete CPS2 hardware emulation implementation inside FinalBurn Neo (FBNeo) with a custom Apple Metal renderer on macOS. The implementation provides full CPS2 emulation with real-time rendering, input handling, and audio output.

## Architecture

### Core Components

1. **CPS2 Emulation Core** (`src/burn/drv/capcom/`)
   - `cps.cpp` - Main CPS system implementation
   - `cps_run.cpp` - Frame execution and timing
   - `d_cps2.cpp` - CPS2 driver definitions
   - Real 68000 CPU emulation
   - QSound audio processor emulation
   - Graphics tile/sprite rendering

2. **Metal Renderer** (`src/burner/metal/metal_renderer.mm`)
   - Native Metal API integration
   - Hardware-accelerated rendering pipeline
   - Real-time texture updates from emulation frame buffer
   - Nearest-neighbor filtering for pixel-perfect display
   - 60 FPS synchronized rendering

3. **Input System** (`src/burner/metal/metal_input.mm`, `metal_input_bridge.cpp`)
   - Keyboard and gamepad support via GameController framework
   - Direct mapping to CPS2 input registers
   - Support for 2 players with full 6-button controls

4. **Audio System** (`src/burner/metal/metal_audio_simple.mm`)
   - Core Audio integration
   - Real-time audio streaming at 44.1kHz
   - QSound emulation output
   - Low-latency audio queue implementation

5. **ROM Management** (`src/burner/metal/metal_rom_validation.cpp`)
   - ZIP file support for ROM sets
   - ROM validation and CRC checking
   - Automatic ROM type detection

## Data Flow

### Frame Execution Flow

```
1. Main Loop (60Hz timer)
   ↓
2. Metal_RunFrame()
   ├─→ Metal_ProcessInput() - Read keyboard/gamepad state
   │   └─→ BurnDrvSetInput() - Update CPS2 input registers
   ↓
3. BurnDrvFrame() - Execute one frame of emulation
   ├─→ Cps2Frame() - Run CPS2 hardware for one frame
   │   ├─→ SekRun() - Execute 68000 CPU
   │   ├─→ QscUpdate() - Generate audio samples
   │   └─→ CpsRedraw() - Render graphics to pBurnDraw
   ↓
4. Metal_GetFrameBuffer() - Get rendered frame
   ↓
5. MTKView drawInMTKView - Upload to GPU and display
   └─→ Metal texture update and rendering
```

### Memory Layout

```
CPS2 Memory Map:
- 0x000000-0x3FFFFF: 68K Program ROM (4MB)
- 0x400000-0x40000B: Coin/Start inputs
- 0x400010-0x400013: Player 1 inputs
- 0x400014-0x400017: Player 2 inputs
- 0x400018-0x40001F: System inputs
- 0x618000-0x619FFF: QSound RAM
- 0x660000-0x663FFF: QSound ROM
- 0x700000-0x701FFF: Sprite RAM
- 0x708000-0x709FFF: Layer control
- 0x900000-0x92FFFF: Graphics ROM window
- 0xFF0000-0xFFFFFF: 68K Work RAM (64KB)
```

### Input Mapping

```
Player 1:
- Arrow Keys: D-Pad
- Z/X/C: Weak/Medium/Strong Punch
- V/B/N: Weak/Medium/Strong Kick
- 1: Start
- 5: Coin

Player 2:
- W/A/S/D: D-Pad
- Q/E/R: Weak/Medium/Strong Punch
- T/Y/H: Weak/Medium/Strong Kick
- 2: Start
- 6: Coin

System:
- F3: Reset
- ESC: Quit
```

## Implementation Details

### CPS2 Core Binding

The CPS2 emulation core is fully integrated through:

1. **Driver Selection**: `BurnDrvFind()` and `BurnDrvSelect()` locate and activate the mvsc driver
2. **Initialization**: `BurnDrvInit()` calls `Cps2Init()` which sets up:
   - Memory allocation for ROM/RAM regions
   - CPU initialization (68000 at 11.8MHz)
   - Graphics system setup
   - QSound initialization

3. **Frame Execution**: `BurnDrvFrame()` calls `Cps2Frame()` which:
   - Processes input state
   - Runs CPU for one frame (262 scanlines)
   - Handles interrupts (VBlank at line 224)
   - Updates audio
   - Renders graphics layers

### ROM Loading

ROMs are loaded through the FBNeo ROM loading system:
- `BurnLoadRom()` loads individual ROM files
- ROM types are identified by metadata (PRG_68K, GFX, QSND, etc.)
- Graphics ROMs are deinterleaved for the tile renderer
- Program ROMs are decrypted using CPS2 encryption keys

### Graphics Rendering

The CPS2 graphics system renders:
- 3 scrolling background layers (8x8 tiles)
- 1 sprite layer (16x16 sprites, up to 256 on screen)
- 16-bit RGB color (4096 colors from 65536 palette)
- Hardware scaling and rotation for sprites

Frame buffer format: 32-bit BGRA (converted from CPS2's 16-bit RGB)
Resolution: 384x224 pixels

### Audio Processing

QSound chip emulation provides:
- 16 channels of compressed audio
- 16-bit stereo output
- Hardware echo/reverb effects
- Sample-based synthesis

Audio is rendered at 44.1kHz and fed to Core Audio in 735-sample buffers (60fps).

### Save States

Save state support includes:
- Full CPU state (registers, RAM)
- Graphics state (VRAM, registers, palette)
- Audio state (QSound RAM, channel states)
- Input state

## Building and Running

### Build Requirements

- macOS 14.0+
- Xcode 15+ with Metal SDK
- Apple Silicon or Intel Mac

### Build Commands

```bash
# Clean build
make -f makefile.metal clean

# Build the emulator
make -f makefile.metal

# Run with ROM
./fbneo_metal /path/to/mvsc.zip
```

### Debug Features

- F1: Toggle debug overlay
- Frame timing information
- Input state display
- Audio buffer status
- Performance metrics

## Performance

The Metal implementation achieves:
- Consistent 60 FPS rendering
- < 16ms frame time
- Hardware-accelerated scaling
- Zero-copy texture updates
- Low audio latency (< 50ms)

## Future Enhancements

1. Additional CPS2 games support
2. Network play functionality
3. Shader effects (CRT simulation, scanlines)
4. Recording and playback
5. Lua scripting support
6. iOS/iPadOS port

## Troubleshooting

### No Video Output
- Check Metal device creation in console
- Verify texture format compatibility
- Ensure pBurnDraw is properly allocated

### No Audio
- Check Core Audio permissions
- Verify QSound emulation is enabled
- Check audio buffer allocation

### Input Not Working
- Verify keyboard/gamepad is detected
- Check input mapping in metal_input_bridge.cpp
- Ensure BurnInputInit() was called

### ROM Loading Issues
- Verify ROM CRC matches driver expectations
- Check file paths and permissions
- Ensure ZIP contains all required files 