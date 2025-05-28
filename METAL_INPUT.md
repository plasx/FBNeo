# FBNeo Metal - Input Handling System

This document describes the input handling system for the Metal port of FBNeo on macOS.

## Overview

The Metal port provides a comprehensive input handling system that maps macOS keyboard events to the FBNeo emulator's internal input system. The implementation consists of:

1. **Key Mapping System** - Configurable key mappings for multiple players
2. **Event Handling** - Integration with macOS event system
3. **FBNeo Core Integration** - Connects to the emulator's input handling

## Default Key Mappings

### Player 1 Controls

| Function | Key           |
|----------|---------------|
| Up       | Up Arrow      |
| Down     | Down Arrow    |
| Left     | Left Arrow    |
| Right    | Right Arrow   |
| Button 1 | Z             |
| Button 2 | X             |
| Button 3 | C             |
| Button 4 | A             |
| Button 5 | S             |
| Button 6 | D             |
| Coin     | 1             |
| Start    | 2             |

### Player 2 Controls

| Function | Key           |
|----------|---------------|
| Up       | I             |
| Down     | K             |
| Left     | J             |
| Right    | L             |
| Button 1 | R             |
| Button 2 | T             |
| Button 3 | Y             |
| Button 4 | U             |
| Button 5 | O             |
| Button 6 | P             |
| Coin     | 6             |
| Start    | 7             |

### System Controls

| Function     | Key           |
|--------------|---------------|
| Pause        | Space         |
| Reset        | R             |
| Service      | 0             |
| Diagnostic   | F2            |
| Save State   | F3            |
| Load State   | F4            |
| Fast Forward | Tab           |
| Screenshot   | F12           |
| Quit         | Escape        |
| Menu         | F1            |
| Fullscreen   | Return/Enter  |

## Architecture

The input system is composed of several components:

### 1. FBNeoInputHandler Class

This Objective-C class manages:
- Key mappings for multiple players
- Event handling for key presses/releases
- Tracking which keys are currently pressed

### 2. C Interface Functions

These functions bridge between Objective-C and C:
- `Metal_InitInput()` - Initialize the input system
- `Metal_ExitInput()` - Clean up the input system
- `Metal_HandleKeyDown()` - Process key down events
- `Metal_HandleKeyUp()` - Process key up events
- `Metal_ResetInputState()` - Reset all input state
- `Metal_SetPlayerKeyMap()` - Configure custom key mappings
- `Metal_SetDefaultKeyMaps()` - Reset to default key mappings

### 3. NSApplication Extensions

Extensions to integrate with the macOS event system:
- `setupFBNeoInputMonitoring` - Sets up event monitors
- Categories for NSWindow and NSApplication to handle key events

## Customizing Key Mappings

Key mappings can be customized programmatically by calling:

```objc
Metal_SetPlayerKeyMap(int player, int* keyMap, int keyMapSize);
```

Where:
- `player` is the player number (0-based)
- `keyMap` is an array of macOS key codes paired with FBNeo key codes
- `keyMapSize` is the size of the keyMap array

## FBNeo Key Codes

The following key codes are defined for use with the input system:

```c
#define FBNEO_KEY_UP            0x01
#define FBNEO_KEY_DOWN          0x02
#define FBNEO_KEY_LEFT          0x03
#define FBNEO_KEY_RIGHT         0x04
#define FBNEO_KEY_BUTTON1       0x05
#define FBNEO_KEY_BUTTON2       0x06
#define FBNEO_KEY_BUTTON3       0x07
#define FBNEO_KEY_BUTTON4       0x08
#define FBNEO_KEY_BUTTON5       0x09
#define FBNEO_KEY_BUTTON6       0x0A
#define FBNEO_KEY_COIN          0x0B
#define FBNEO_KEY_START         0x0C
#define FBNEO_KEY_SERVICE       0x0D
#define FBNEO_KEY_RESET         0x0E
#define FBNEO_KEY_PAUSE         0x0F
...
```

## Future Enhancements

1. **GUI Key Configuration** - Add a graphical interface for configuring key mappings
2. **Game Controller Support** - Support for macOS GameController framework
3. **Input Recording/Playback** - For demo recording and playback
4. **Hotplug Support** - Detecting controllers being connected/disconnected
5. **Per-Game Configurations** - Save and load input configurations per game 