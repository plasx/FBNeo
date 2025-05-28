# FBNeo Metal Quick Start Guide

This guide will help you get started with the FBNeo Metal implementation for macOS, which provides hardware-accelerated emulation with advanced features including AI-assisted gameplay.

## Installation

1. Download the latest FBNeo Metal build from the releases page or build it from source.

### Building from Source

To build FBNeo Metal from source:

```bash
git clone https://github.com/finalburnneo/fbneo.git
cd fbneo
chmod +x build_metal_complete.sh
./build_metal_complete.sh
```

The executable will be created at `bin/metal/fbneo_metal`.

## Basic Usage

### Running Games

1. Place your ROM files in the `bin/metal/roms` directory.
2. Launch the emulator:

```bash
cd bin/metal
./fbneo_metal
```

3. From the game selection menu, choose the ROM you want to play.

### Default Controls

| Action | Default Key |
|--------|-------------|
| D-Pad Up | Up Arrow |
| D-Pad Down | Down Arrow |
| D-Pad Left | Left Arrow |
| D-Pad Right | Right Arrow |
| Button 1 | A |
| Button 2 | S |
| Button 3 | D |
| Button 4 | Z |
| Button 5 | X |
| Button 6 | C |
| Start | 1 |
| Coin | 5 |
| Exit | Esc |
| Pause | P |
| Screenshot | F12 |

## Advanced Features

### AI-Assisted Gameplay

The Metal implementation includes AI features that can help you play games or enhance CPU opponents:

1. **Enable AI Assistance**: Press F1 to toggle AI assistance.
2. **Adjust AI Difficulty**: Press F2 to cycle through difficulty levels.
3. **Training Mode**: Press F3 to toggle AI training mode.
4. **Debug Overlays**: Press F4 to toggle AI debug information overlay.

### Configuration

Configuration files are stored in the `bin/metal/config` directory:

- `input.cfg`: Input configuration
- `video.cfg`: Video settings
- `audio.cfg`: Audio settings
- `ai.cfg`: AI settings

### Custom AI Models

To use custom AI models:

1. Place your CoreML models in the `bin/metal/models` directory.
2. In the game, press F5 to open the AI model selection menu.
3. Choose the model you want to use for your game.

### CRT Simulation and Visual Effects

The Metal renderer includes several visual effects:

- Press F6 to toggle CRT simulation
- Press F7 to toggle scanlines
- Press F8 to adjust screen scaling
- Press F9 to toggle V-sync

## Troubleshooting

### Common Issues

1. **Game crashes on startup**:
   - Verify your ROM files are correct and complete
   - Check the console output for specific error messages

2. **Poor performance**:
   - Lower the rendering resolution in the video settings
   - Disable advanced visual effects
   - Check if other applications are consuming system resources

3. **AI features not working**:
   - Verify you have the necessary AI models in the models directory
   - Check macOS permissions for accessing machine learning features

### Getting Help

If you encounter problems:

1. Check the full documentation in the `docs` directory
2. Visit the project's GitHub issues page
3. Join the FBNeo community forums

## Next Steps

- Explore the [Metal Build Status](Metal_Build_Status.md) document for current support status
- Read the [Complete Implementation Summary](Complete_Implementation_Summary.md) for technical details
- Check [AI Features](FBNeo_AI_Features.md) for more information on AI capabilities 