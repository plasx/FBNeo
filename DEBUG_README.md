# FBNeo Metal Debug System Documentation

## Overview

This document describes the comprehensive debugging systems implemented for the FBNeo Metal emulator. These systems provide detailed visibility into the ROM loading process, memory management, hardware emulation, and runtime execution.

## Debug Systems Implemented

### 1. Enhanced Debug Output
- Severity-based logging (ERROR, WARNING, INFO, DETAIL, VERBOSE)
- Formatted output with timestamps and prefixes
- Direct console output with unbuffered writing
- Log file output for persistent debugging

### 2. CRC32 Validation
- ROM integrity verification
- Checksum validation against expected values
- Corruption detection and reporting

### 3. Memory Allocation Tracking
- Allocation/deallocation tracking
- Memory usage statistics
- Leak detection

### 4. Graphics Initialization Tracking
- Asset loading and decoding monitoring
- Sprite and background tracking
- Frame buffer management

### 5. Audio Initialization Tracking
- Sound subsystem monitoring
- Buffer statistics and underrun detection
- Sample rate and format validation

### 6. Hardware Emulation Tracking
- CPU component initialization
- Graphics hardware emulation
- Sound hardware emulation

### 7. Direct Output System
- Unbuffered console output
- Multi-channel logging
- Color-coded output (via debug script)

## Viewing Debug Output

There are several ways to view the debug output:

1. **Debug Mode Script**:
   - Run `./fbneo_debug_mode.sh` for an interactive launcher with multiple debug options
   
2. **Debug Preview Program**:
   - Run `./debug_preview` to see the exact debug format output without running the emulator
   - This shows the precise format requested with all sections and explanations
   
3. **Command Line Options**:
   - `--debug-format`: Shows the standardized debug format and exits
   - `--standardized-output`: Shows the standardized debug format and runs the emulator
   - `--enhanced-debug`: Enables enhanced debug during operation
   - `--enhanced-debug-only`: Shows debug format and exits (same as --debug-format)

4. **Direct Display Script**: 
   - Run `./fbneo_debug_display.sh` to see a colored, formatted display of the debug output

5. **Log Files**:
   - `fbneo_direct.log`: Contains all direct console output
   - `fbneo_debug_sample.log`: Contains a sample of the expected debug format

## Debug Format

The debug output follows a structured format with prefixed sections:

```
[ROM CHECK] - ROM loading and verification
[MEM INIT] - Memory allocation and initialization
[HW INIT] - Hardware emulation setup
[GRAPHICS INIT] - Graphics asset loading and processing
[AUDIO INIT] - Audio system initialization
[INPUT INIT] - Input controller setup
[EMULATOR] - Emulation core operations
[RENDERER LOOP] - Rendering pipeline operations
[AUDIO LOOP] - Audio processing loop
[INPUT LOOP] - Input polling and controller support
[GAME START] - Final confirmation that game is running successfully
```

## Quick Start Commands

```bash
# Interactive debug mode launcher
./fbneo_debug_mode.sh

# Show exact debug format without running emulator (preferred for format viewing)
./debug_preview

# Show colored debug format
./fbneo_debug_display.sh

# Show standardized debug format
./build/metal/fbneo_metal --debug-format

# Run with ROM and show standardized debug output
./build/metal/fbneo_metal /path/to/rom.zip --standardized-output
```

## Example Output

```
[ROM CHECK] Located ROM: /Users/plasx/dev/ROMs/mvsc.zip
[ROM CHECK] CRC32 validation passed for all ROM components.
[ROM CHECK] CPS2 encryption keys verified and ROM successfully decrypted.

[MEM INIT] Allocating memory for CPS2 emulation components...
[MEM INIT] Main CPU (Motorola 68000) memory allocated.
[MEM INIT] Sound CPU (Z80) memory allocated.
[MEM INIT] Graphics and palette memory allocated.
[MEM INIT] Audio (QSound DSP) memory allocated.

[HW INIT] CPS2 hardware emulation components initialized successfully.

...and so on...
```

## Benefits

- **Clear Debugging**: Each stage of ROM loading is clearly identified
- **Component Tracking**: Memory, graphics, audio, and hardware components are individually tracked
- **Error Detection**: Issues are caught and reported earlier in the loading process
- **Consistent Format**: Output is standardized for easier reading and parsing
- **Multiple Viewing Options**: Choose from console, colored display, or log files

## Note on macOS GUI Applications

Because FBNeo Metal runs as a graphical macOS application, standard console output may not be visible during normal operation. This is due to how macOS handles stdout/stderr for GUI applications. For this reason, we provide multiple ways to see the debug output:

1. Use `./debug_preview` for the exact format without running the emulator
2. Use `./fbneo_debug_display.sh` for a colored version of the format 
3. Use the emulator's log files for persistent debugging information 