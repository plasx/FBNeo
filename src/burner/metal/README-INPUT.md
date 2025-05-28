# FBNeo Metal Input System for macOS/ARM64

This directory contains the improved input handling system for the FBNeo Metal port on macOS, with specific optimizations for Apple Silicon (ARM64) hardware.

## Features

- Modern GameController framework integration for Xbox, PlayStation, and MFi controllers
- Proper 64-bit data types for ARM64 compatibility
- Support for analog sticks, triggers, and digital buttons
- Keyboard input mapping with macOS-specific key codes
- DirectInput-style mapping to FBNeo's input system
- CPS-specific input mapping for Capcom fighting games
- Input testing tool for debugging controller connections

## Files

- `input/metal_input.h` - Main header file for the input system
- `input/metal_input.mm` - Implementation of the input system (Objective-C++)
- `input/input_test.mm` - Standalone test application for debugging input
- `input/build_input_test.sh` - Build script for the input test application
- `fixes/cps_input_full.h` - Comprehensive CPS input definitions
- `fixes/cps_input_full.c` - Implementation of CPS input variables

## Controller Mappings

The input system maps modern controllers to the CPS input format:

- **D-pad/Left Analog**: Directional inputs
- **A/B/X/Y Buttons**: Light/Medium/Heavy Punch/Kick
- **Shoulder Buttons**: Additional attack buttons
- **Triggers**: Special moves or system functions
- **Menu/Options Buttons**: Start/Coin buttons

## Default Key Mappings

### Player 1:
- **Arrow Keys**: Up/Down/Left/Right
- **A/S/D**: Light/Medium/Heavy Punch
- **Z/X/C**: Light/Medium/Heavy Kick
- **1**: Start
- **5**: Coin

### Player 2:
- **I/K/J/L**: Up/Down/Left/Right
- **Numpad 4/5/6**: Light/Medium/Heavy Punch
- **Numpad 1/2/3**: Light/Medium/Heavy Kick
- **2**: Start
- **6**: Coin

### System Controls:
- **F3**: Reset
- **F4**: Service
- **Escape**: Exit/Menu

## Testing Controllers

You can use the included test application to verify controller functionality:

```bash
cd src/burner/metal/input
./build_input_test.sh
./build/input_test
```

This will display live controller input states and CPS input values.

## Integration with FBNeo

The input system is integrated with FBNeo through the `metal_bridge.h` interface, and directly maps controller inputs to the CPS input arrays required by games (CpsInp000, CpsInp001, etc.).

## Future Improvements

- Configuration interface for remapping inputs
- Support for additional controller types
- Improved force feedback/rumble support
- Input recording and playback for training modes 