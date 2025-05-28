# FBNeo GameController Framework Integration

This document describes the integration of Apple's GameController framework in the Metal renderer for FBNeo on macOS, providing native support for both keyboard and game controller input.

## Overview

The Metal port of FBNeo uses Apple's GameController framework to provide a high-quality, low-latency input experience for modern game controllers. This includes support for:

- Standard and Extended MFi controllers
- PlayStation DualShock/DualSense controllers
- Xbox Wireless controllers
- Third-party Bluetooth controllers
- Apple remote (Micro gamepad)

## Features

- **Hot-plugging**: Controllers can be connected and disconnected during gameplay
- **Multi-player support**: Multiple controllers can be used simultaneously
- **Per-game configurations**: Save and load controller configurations for specific games
- **Configuration GUI**: Built-in UI for remapping controls
- **Persistent settings**: All configurations are automatically saved

## Configuration GUI

To access the configuration interface, use one of these methods:

1. From the menu bar: **Configuration → Input Settings for Game** (⌘⇧I)
2. From the menu bar: **Configuration → Global Input Settings** (⌘I)

The configuration window contains three tabs:

### Keyboard Tab

Configure keyboard mappings for the current game or globally. Click on a key binding to change it.

### Controller Tab

Configure controller mappings. If multiple controllers are connected, use the dropdown to select which controller to configure.

### Display Tab

Adjust display settings like shader type.

## Per-Game Configurations

When configuring inputs, you can:

1. Save settings as the global default (applies to all games without specific configurations)
2. Save settings for just the current game (applies only to this game)

To save a game-specific configuration:
1. Open the configuration window (⌘⇧I)
2. Make your changes
3. Enter a game name in the field at the bottom
4. Click "Save For Game"

## Default Controller Mappings

Here are the default controller button mappings:

| FBNeo Function | Default Controller Button |
|----------------|---------------------------|
| D-Pad Up       | D-Pad Up                  |
| D-Pad Down     | D-Pad Down                |
| D-Pad Left     | D-Pad Left                |
| D-Pad Right    | D-Pad Right               |
| Button 1       | A Button                  |
| Button 2       | B Button                  |
| Button 3       | X Button                  |
| Button 4       | Y Button                  |
| Button 5       | Left Shoulder (L1)        |
| Button 6       | Right Shoulder (R1)       |
| Coin           | Options/Back/Select       |
| Start          | Menu/Start                |

## Advanced Configuration

The controller handler automatically supports several advanced features:

- Analog sticks can also function as digital inputs (with configurable deadzones)
- Trigger buttons can be configured as digital or analog inputs
- Hardware-specific features (e.g., PlayStation controller lightbar) are supported

## Troubleshooting Controller Issues

If you experience controller problems:

1. **Controller not detected**:
   - Make sure the controller is paired with your Mac
   - Try disconnecting and reconnecting the controller
   - Check that the controller has sufficient battery

2. **Button mappings are incorrect**:
   - Use the configuration GUI to check and correct mappings
   - Reset to default configuration if necessary

3. **Controller disconnects during gameplay**:
   - Check battery level
   - Reduce distance from controller to Mac
   - Check for Bluetooth interference

## Programming Interface

For developers extending the FBNeo Metal port, the following C functions are available:

```c
// Initialize/cleanup controller support
int Metal_InitControllerSupport();
void Metal_ExitControllerSupport();

// Controller information
int Metal_GetControllerCount();
const char* Metal_GetControllerName(int controllerIndex);

// Status checks
int Metal_IsControllerButtonPressed(int playerIndex, int buttonCode);
void Metal_GetFBNeoInputStateForPlayer(int playerIndex, unsigned char* output, int outputSize);

// Configuration
int Metal_GetControllerMapping(int controllerIndex, int* mapping, int maxMappings);
int Metal_SetControllerMapping(int controllerIndex, int* mapping, int mappingSize);
int Metal_SaveControllerMappingForGame(const char* gameName, int controllerIndex);
int Metal_LoadControllerMappingForGame(const char* gameName, int controllerIndex);

// UI
void Metal_ShowInputConfig(const char* gameName);
```

For more detailed information about the GameController framework, refer to Apple's official documentation. 