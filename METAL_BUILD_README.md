# FBNeo Metal Backend

This is the Metal backend for FBNeo (Final Burn Neo) emulator, optimized for macOS.

## Building

To build the Metal backend, run:

```bash
./build_metal.sh
```

This will compile the Metal backend and create a binary in `bin/metal/fbneo_metal`.

## Running

To run the Metal backend, use:

```bash
./run_fbneo_metal.sh /path/to/your/rom.zip
```

Example:

```bash
./run_fbneo_metal.sh /Users/username/ROMs/mvsc.zip
```

## Architecture

The Metal backend consists of several key components:

1. **Metal Renderer**: Handles rendering emulated frames to the screen using Metal
2. **Input Handling**: Processes keyboard and gamepad inputs
3. **Audio Processing**: Handles audio playback
4. **C/C++/Objective-C Compatibility Layer**: Provides seamless integration between different language components

## Build System

The build system uses a makefile-based approach with the following key files:

- `makefile.metal`: Main makefile for the Metal backend
- `build_metal.sh`: Script to build the Metal backend
- `run_fbneo_metal.sh`: Script to run the Metal backend

## CPS2 Support

The Metal backend currently supports CPS2 games like Marvel vs. Capcom. Additional platform support is being added incrementally.

## AI Integration

The Metal backend includes support for AI features:

- Neural network-based opponents
- Frame analysis
- Training modes
- Pattern recognition

To enable AI features, you need to download the AI models first:

```bash
./download_models.sh
```

## Troubleshooting

If you encounter build issues:

1. Make sure you have Xcode Command Line Tools installed
2. Check that Metal is supported on your Mac
3. Ensure all required headers are available

## Contributing

When contributing to the Metal backend:

1. Follow the existing code style
2. Maintain C/C++/Objective-C compatibility
3. Test thoroughly before submitting changes

## License

Same as the main FBNeo project. 