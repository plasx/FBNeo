# FBNeo Metal Implementation for macOS

This is a Metal-based frontend for the FinalBurn Neo (FBNeo) arcade emulator, specifically designed for macOS. This implementation focuses on running CPS2 arcade games like Marvel vs. Capcom, Street Fighter Alpha 3, and other Capcom arcade titles.

## Features

- Native Metal rendering for modern macOS systems
- Support for Apple Silicon (M1/M2/M3) and Intel Macs
- AVAudioEngine for high-quality audio output
- Controller and keyboard support
- Simple, clean interface
- Test pattern generation for diagnostics

## Requirements

- macOS 11.0 or later (Big Sur or later recommended)
- Xcode command-line tools installed
- CPS2 ROM files (.zip format)

## Building

To build the Metal frontend, run:

```bash
./build_metal_final.sh
```

This will create an executable named `fbneo_metal` in the current directory.

## Running Games

To run a game using the Metal frontend:

```bash
./run_fbneo_metal.sh [ROM_DIRECTORY]
```

This will:
1. Show a list of available ROMs in the specified directory
2. Let you select from popular CPS2 games or specify a custom ROM
3. Launch the selected ROM with the Metal renderer

If you don't specify a ROM directory, it will default to `~/ROMs/CPS2`.

You can also run a game directly:

```bash
./fbneo_metal /path/to/your/rom.zip
```

## Controls

Default keyboard controls:
- Arrow keys: Movement
- A, S, D, Z, X, C: Buttons 1-6
- 1: Insert coin
- 5: Start
- ESC: Exit
- F: Toggle fullscreen

Controller support is also available if you have a compatible gamepad connected.

## Troubleshooting

If you encounter issues:

1. Make sure your ROM files are valid CPS2 ROM sets (e.g., mvsc.zip, xmvsf.zip)
2. Check that your macOS version is up-to-date
3. Try rebuilding the application using the build script
4. Examine the console output for error messages

## Known Issues

- Only CPS2 games are well-tested at this point
- Some advanced features like netplay are not yet implemented
- Save states and NVRAM saving may not work perfectly

## Development Notes

This implementation creates a clean layer between Metal-specific code and the original FBNeo components using stub implementations and simplified tracking systems. The code focuses on:

- Memory management with proper tracking
- Input handling with keyboard/controller mapping
- Audio routing using AVAudioEngine
- Graphics rendering with Metal 

To modify the Metal rendering code, look at:
- `src/burner/metal/metal_renderer_complete.mm`

For ROM loading:
- `src/burner/metal/metal_zip_extract.cpp`
- `src/burner/metal/rom_verify.h`

## License

This Metal implementation follows the same license as the core FBNeo project. 