# FBNeo Metal Backend Usage Guide

This document provides instructions for building and running the FBNeo Metal backend on macOS.

## Requirements

- macOS with Metal support
- Xcode Command Line Tools installed
- ROMs in .zip format

## Building and Running

There are several ways to build and run the FBNeo Metal backend:

### Option 1: Manual Build and Run

```bash
# Build the Metal backend
make -f makefile.metal clean && make -f makefile.metal -j10

# Create a symlink to the binary
ln -sf ./bin/metal/fbneo_metal .

# Run with a ROM
./fbneo_metal /path/to/rom.zip
```

### Option 2: Using Run Script

```bash
# Build separately
make -f makefile.metal clean && make -f makefile.metal -j10

# Then run using the run script
./run_fbneo_metal.sh /path/to/rom.zip
```

### Option 3: All-in-One Script for Any ROM

```bash
# Build and run any ROM in one step
./build_and_run_game.sh /path/to/rom.zip
```

### Option 4: Marvel vs. Capcom Specific Script

```bash
# Build and run Marvel vs. Capcom
./build_and_run_mvsc.sh
```

## Troubleshooting

### Binary Not Found

If you encounter errors about the binary not being found, ensure:

1. The build completed successfully
2. The binary exists in `./bin/metal/fbneo_metal`
3. The symlink is correctly created in the root directory

Fix with:

```bash
ln -sf ./bin/metal/fbneo_metal .
chmod +x ./fbneo_metal
```

### Build Errors

If you encounter build errors:

1. Make sure all source files are available
2. Check that necessary headers are properly included
3. Verify that the structure definitions in patched headers match those needed by the code

### ROM Issues

If you encounter issues with a specific ROM:

1. Make sure the ROM file exists at the specified path
2. Verify the ROM is a valid .zip file
3. Check if the ROM is supported by the FBNeo emulator

## Supported Features

The current Metal backend supports:

- CPS2 games (including Marvel vs. Capcom)
- Basic rendering functionality
- Stub implementations of core FBNeo functions

## Additional Information

For more detailed documentation, refer to:

- `METAL_BUILD_README.md` - Comprehensive build documentation
- `docs/METAL_BUILD_FIXES.md` - Details about build fixes 