# FBNeo Metal Implementation

This is a Metal-based frontend for FBNeo (Final Burn Neo) arcade emulator, which allows running CPS2 games like Marvel vs. Capcom on macOS systems.

## Features

* Metal renderer for hardware-accelerated graphics
* Support for CPS2 ROMs
* Configurable display settings
* Frame rate monitoring
* Developer debug information

## Build Instructions

There are two ways to build the Metal implementation:

### Simple Build

The simple build is a streamlined version that focuses on getting the Metal frontend working quickly:

```bash
make -f makefile.metal.simple clean && make -f makefile.metal.simple
```

### Full Build

The full build includes more features but requires more dependencies:

```bash
./build_metal_final.sh
```

## Running Games

Once built, you can run games using the provided scripts:

```bash
# Run Marvel vs Capcom
./run_mvsc_metal.sh

# Run a specific ROM
./fbneo_metal /path/to/your/rom.zip
```

## Troubleshooting

If you encounter issues:

1. **Missing ROM** - Make sure your ROM is in one of the searched directories (see script for paths)
2. **Rendering Issues** - Set `METAL_DEBUG=1` and check console output for errors
3. **Build Failures** - Run `make -f makefile.metal.simple clean` and retry

## Implementation Details

The Metal implementation consists of several key components:

* **metal_standalone_main.mm** - Main entry point and UI
* **metal_renderer_bridge.cpp** - Bridge between FBNeo and Metal renderer
* **metal_c_bridge.c** - C interface for Metal functions
* **fbneo_core_stubs.c** - Core emulation stubs

The implementation uses a shared frame buffer (g_frameBuffer) to transfer pixel data from the emulation core to the Metal renderer.

## System Requirements

* macOS 10.15 or later
* Mac with Metal-compatible GPU

## Development

To add debugging features or extend the implementation:

1. Add implementations to metal_c_bridge.c
2. Update renderer_bridge.cpp for frame buffer handling
3. Modify metal_standalone_main.mm for UI and gameplay changes

## License

This implementation is provided under the same license as FBNeo. 