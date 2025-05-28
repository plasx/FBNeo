# FBNeo Metal Input System

## Overview
This document describes the implementation of the input system for the FBNeo Metal port on macOS, focusing on keyboard and gamepad support for Marvel vs Capcom (CPS2).

## Input System Components

### 1. Input Definitions (metal_input_defs.h)
- Defines macOS keyboard virtual key codes
- Maps input IDs for FBNeo controls (P1/P2 directions, buttons, system functions)
- Provides constants for special functions (save/load state, quit, etc.)

### 2. Input Implementation (metal_input_stub.mm)
- Handles keyboard input via Metal_HandleKeyDown/Up functions
- Processes gamepad input via GameController framework
- Updates input state on each frame via Metal_ProcessInput
- Supports both keyboard and gamepad simultaneously
- Provides keyboard mapping for both Player 1 and Player 2

### 3. Input Stubs Interface (metal_input_stubs.h)
- Declares function prototypes for the input system
- Ensures consistent interface across the codebase

## Keyboard Mapping

### Player 1 Controls
- Directional: Arrow Keys
- Buttons: Z (Weak Punch), X (Medium Punch), C (Strong Punch), V (Weak Kick), B (Medium Kick), N (Strong Kick)
- Start: Enter
- Coin: Space

### Player 2 Controls
- Directional: WASD
- Buttons: Q (Weak Punch), W (Medium Punch), E (Strong Punch), R (Weak Kick), T (Medium Kick), Y (Strong Kick)
- Start: Tab
- Coin: Caps Lock

### System Controls
- Save State: F5
- Load State: F8
- Reset Game: F3
- Toggle Debug Overlay: F1
- Quit: Escape

## Gamepad Support
- Uses Apple's GameController framework
- Automatically detects connected controllers
- Maps gamepad buttons to appropriate game controls
- Supports both Xbox and PlayStation controller layouts

## Testing
- A standalone test application is provided (fbneo_input_test)
- Shows key presses and their mapping to FBNeo inputs
- Useful for verifying input configurations

## How to Build and Test

### Build Input Test
```
./build_input_simple.sh
```

### Run Input Test
```
./fbneo_input_test
```

### Building Full Emulator with Input Support
```
./build_input.sh
```

## Implementation Notes

1. The input system is designed to be frame-based, calling Metal_ProcessInput() once per frame to update all input states.

2. Special inputs (save/load state, quit, etc.) are handled immediately when the key is pressed.

3. The keyboard mapping follows standard arcade conventions with arrow keys for P1 and WASD for P2.

4. Gamepad support automatically maps controller buttons to appropriate game functions, with first controller mapped to P1 and second controller to P2.

5. The implementation integrates with the existing FBNeo Metal port by calling BurnDrvSetInput for each input on every frame.

## Future Enhancements

1. Add support for remappable controls via a configuration file

2. Implement a GUI for input configuration

3. Add support for additional controllers (fight sticks, etc.)

4. Support hotplugging of controllers 