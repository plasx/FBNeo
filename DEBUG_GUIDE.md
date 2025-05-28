# FBNeo Metal Debug Output Guide

This guide explains how to use the enhanced debug output features in the Metal-based FBNeo emulator.

## Overview

The debug output system provides a structured, section-based format for understanding the ROM loading and emulation initialization process. This helps developers quickly identify where issues occur and understand the internal workings of the emulator.

## Debug Format

The debug output is organized into the following sections:

| Section | Description |
|---------|-------------|
| `[ROM CHECK]` | ROM presence, integrity, and encryption checks |
| `[MEM INIT]` | Memory allocations for CPU, graphics, and audio |
| `[HW INIT]` | Emulated CPS2 hardware initialization |
| `[GRAPHICS INIT]` | Graphics decoding and palette setup |
| `[AUDIO INIT]` | Audio hardware (QSound DSP) initialization |
| `[INPUT INIT]` | Controller and keyboard input mapping initialization |
| `[EMULATOR]` | CPU emulation main loop entry |
| `[MTKRenderer]` | Metal renderer backend initialization |
| `[RENDERER LOOP]` | Graphics rendering loop processes |
| `[AUDIO LOOP]` | Audio streaming and synchronization |
| `[INPUT LOOP]` | Input polling and controller support |
| `[GAME START]` | Final confirmation that game is running successfully |

## How to View Debug Output

There are two primary ways to view the debug output:

### 1. Using the Debug-Only Mode

Run the emulator with the `--debug-only` flag to see the debug output without launching the GUI:

```sh
./fbneo_metal --debug-only /path/to/rom.zip
```

This will show the complete debug output in your terminal and then exit.

### 2. Using the Debug Scripts

We provide two convenience scripts:

#### a. Terminal-Only Debug Mode

```sh
./debug_run.sh [path/to/rom.zip]
```

This script:
- Recompiles the emulator
- Runs it in debug-only mode
- Filters duplicated output for cleaner display

#### b. GUI Debug Mode

```sh
./debug_gui_run.sh [path/to/rom.zip]
```

This script:
- Recompiles the emulator
- Shows debug output in the terminal
- Also launches the GUI for normal gameplay

## Internal Debug Implementation

For developers looking to modify or extend the debug system, the key files are:

- `src/burner/metal/debug_controller.c`: Main debug output implementation
- `src/burner/metal/debug_controller.h`: Debug section definitions and API
- `src/burner/metal/metal_standalone_main.mm`: Debug initialization and ROM loading
- `src/burner/metal/metal_rom_loader.c`: ROM loading debug hooks

## Why This Format?

This standardized debug format:
- Clearly communicates each step to the developer
- Facilitates debugging by pinpointing exactly where issues occur
- Ensures easy tracking of initialization stages and real-time feedback on emulation status

## Extending the Debug System

To add new debug output sections:

1. Define a new section in `debug_controller.h`
2. Use `Debug_Log()` or `Debug_PrintSectionHeader()` with your section
3. Group related messages under the same section

Example:

```c
// Add a log message
Debug_Log(DEBUG_RENDERER, "Created frame buffer %dx%d (%d bytes)", width, height, size);

// Add a section header (for major steps)
Debug_PrintSectionHeader(DEBUG_AUDIO_INIT, "Initializing QSound DSP...");
``` 