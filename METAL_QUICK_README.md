# FBNeo Metal Implementation - Quick Start Guide

This is a quick start guide for building and running the Metal backend of FBNeo for macOS.

## Building

### Prerequisites

- macOS with Metal support
- Command Line Tools for Xcode
- Homebrew (optional, for dependencies)

### Steps

1. **Fix build issues** (if necessary):
   ```
   ./fix_metal_build.sh
   ```

2. **Build the Metal backend**:
   ```
   make -f makefile.metal clean && make -f makefile.metal -j10
   ```

3. **Create a symlink to the executable**:
   ```
   ln -sf ./bin/metal/fbneo_metal .
   ```

## Running

### Quick Run

Use the provided scripts to build and run:

```
./build_and_run_fbneo_metal.sh
```

Or just run without rebuilding:

```
./run_fbneo_metal.sh
```

### Manual Run

```
./fbneo_metal /path/to/your/rom.zip
```

## Troubleshooting

### Common Issues

1. **Build errors with Objective-C syntax**:
   - Run `./fix_metal_build.sh` to apply the necessary fixes
   - If errors persist, check the C/C++/Objective-C compatibility in affected files

2. **Executable not found**:
   - The executable is created in `./bin/metal/fbneo_metal`
   - Create a symlink with `ln -sf ./bin/metal/fbneo_metal .`

3. **Black screen or graphics issues**:
   - Ensure your Mac supports Metal
   - Check ROM paths and file integrity

## Testing

To verify the Metal renderer's C interface works correctly:

```
./build_and_run_test.sh
```

## Documentation

For more detailed information, refer to:

- `docs/METAL_BUILD_FIXES.md` - Details about build fixes
- `docs/Metal_Implementation_Summary.md` - Implementation summary
- `METAL_README.md` - Comprehensive documentation 