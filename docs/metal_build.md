# FBNeo Metal Build Guide

This document provides comprehensive instructions for building and running FBNeo with Metal on macOS/Apple Silicon hardware. The Metal implementation provides native rendering support for macOS, optimized for Apple Silicon.

## System Requirements

- macOS 10.15 (Catalina) or newer (macOS 11+ recommended for Apple Silicon)
- Xcode 12.0 or newer with Command Line Tools installed
- Apple Silicon (M1/M2/M3) or Intel Mac with Metal-capable GPU
- At least 4GB of RAM (8GB+ recommended)
- At least 2GB of free disk space

## Environment Setup

### 1. Install Xcode and Command Line Tools

1. Install Xcode from the Mac App Store
2. Install the Command Line Tools:
   ```bash
   xcode-select --install
   ```
3. Accept the license agreement:
   ```bash
   sudo xcodebuild -license accept
   ```

### 2. Install Required Dependencies

FBNeo Metal requires several libraries to build successfully:

```bash
# Install Homebrew if needed
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install build dependencies
brew install cmake pkg-config

# Install runtime dependencies
brew install sdl2

# Install testing dependencies (optional)
brew install googletest
```

## Getting the Source Code

Clone the FBNeo repository:

```bash
git clone https://github.com/finalburnneo/FBNeo.git
cd FBNeo
```

## Building FBNeo with Metal

### Basic Build

The basic build process uses the `makefile.metal` file:

```bash
# Build with default options
make -f makefile.metal

# Build with verbose output
make -f makefile.metal VERBOSE=1
```

### Build Options

You can customize the build with various options:

```bash
# Debug build (includes debug symbols and assertions)
make -f makefile.metal DEBUG=1

# Turn off AI modules
make -f makefile.metal WITH_AI=0

# Clean and rebuild
make -f makefile.metal clean
make -f makefile.metal
```

### Output Files

After a successful build, you'll find:

- `fbneo_metal`: The main FBNeo executable with Metal support
- Any applicable Metal shader files (*.metallib)

## Running Tests

The Metal implementation includes a comprehensive test suite:

```bash
# Run unit tests
make -f makefile.metal test

# Run integration tests
make -f makefile.metal test-integration

# Run performance tests
make -f makefile.metal test-performance

# Run all tests including performance tests
make -f makefile.metal test-all
```

## Using FBNeo with Metal

### Running FBNeo

Run the executable from the terminal:

```bash
./fbneo_metal
```

### Command Line Options

FBNeo Metal supports various command-line options:

```
--help                 Show help information
--list-games           List all supported games
--game=<game-id>       Load a specific game by ID
--fullscreen           Start in fullscreen mode
--vsync=<0|1>          Enable/disable VSync
--shader=<shader>      Specify a Metal shader to use
--rom-path=<path>      Specify custom ROM path
--cfg-path=<path>      Specify custom config path
--save-path=<path>     Specify custom save path
--verbose              Enable verbose logging
--window-size=WxH      Set window size (e.g., 1280x720)
```

### Setting up ROM Paths

FBNeo needs to know where your ROM files are located:

1. By default, it looks in `~/Documents/FBNeo/roms/`
2. You can specify a custom path with `--rom-path=<path>`
3. Multiple ROM directories can be set in the configuration file

### CPS2 ROM Support

For CPS2 games, ROMs should be placed in the appropriate subdirectory:

```
~/Documents/FBNeo/roms/cps2/
```

The CPS2 ROM loader in the Metal build supports various CPS2 ROM formats and will automatically handle decryption where necessary.

## Configuration

### Config Files

FBNeo Metal stores its configuration files in:

```
~/Library/Application Support/FBNeo/
```

Key files include:

- `fbneo.cfg`: Main configuration file
- `fbneo_inputs.cfg`: Input mapping configuration
- `metalconfig.json`: Metal-specific rendering settings

### Input Configuration

FBNeo Metal supports keyboard, game controllers, and arcade controllers:

1. Keyboard: Standard keyboard controls mapped through SDL
2. Game Controllers: Most HID-compliant controllers detected automatically
3. Arcade Controllers: X-Arcade and similar devices

Configure inputs through:
- The in-game input configuration menu
- Directly editing `fbneo_inputs.cfg`

## Debugging

### Enabling Debug Output

For detailed logging information:

```bash
./fbneo_metal --verbose --debug-metal
```

### Common Issues

1. **ROMs Not Found**: Check ROM paths and filenames
2. **Graphics Glitches**: Try different Metal shader settings
3. **Performance Issues**: Use performance tests to identify bottlenecks
4. **CPS2 ROM Load Failures**: Verify ROMs with the built-in ROM verification

## Advanced Features

### Using CoreML AI Features

FBNeo Metal includes AI-based features powered by CoreML:

```bash
# Build with AI support (default)
make -f makefile.metal WITH_AI=1

# Convert AI models
make -f makefile.metal convert_models
```

Enable AI features with:
```bash
./fbneo_metal --enable-ai
```

### Custom Metal Shaders

FBNeo Metal supports custom shaders:

1. Place your `.metal` shader files in `~/Library/Application Support/FBNeo/shaders/`
2. Compile with the Metal Shader Tool:
   ```bash
   xcrun -sdk macosx metal -c shader.metal -o shader.air
   xcrun -sdk macosx metallib shader.air -o shader.metallib
   ```
3. Select your shader with `--shader=<shader>`

## Building for Distribution

To create a redistributable build:

```bash
# Optimized build
make -f makefile.metal DEBUG=0

# Create a macOS app bundle
make -f makefile.metal app_bundle
```

The resulting `.app` bundle can be distributed or packaged into a DMG file.

## Troubleshooting

### Building Issues

- **Missing Headers**: Ensure Xcode and Command Line Tools are properly installed
- **Missing Libraries**: Check Homebrew installation for dependencies
- **Compile Errors**: Use `VERBOSE=1` to see detailed error messages

### Runtime Issues

- **Crashes on Startup**: Check logs in `~/Library/Logs/FBNeo/`
- **Low Performance**: Try disabling VSync or reducing window size
- **Controller Not Detected**: Check USB connections and permissions

## Contributing to Metal Development

When contributing to the Metal implementation:

1. Follow existing code style and conventions
2. Ensure tests pass with `make -f makefile.metal test-all`
3. Consider performance implications on both Intel and Apple Silicon
4. Add documentation for new features

## Reference

### Key Files and Directories

- `makefile.metal`: Main build file for Metal
- `src/burner/metal/`: Metal implementation source code
- `src/burner/metal/metal_bridge.cpp`: Core-to-Metal interface
- `src/burner/metal/metal_cps2_renderer.cpp`: CPS2-specific renderer
- `src/burner/metal/metal_declarations.h`: Metal-specific declarations
- `tests/metal/`: Test suite for Metal implementation

### Support Resources

- GitHub Issues: [https://github.com/finalburnneo/FBNeo/issues](https://github.com/finalburnneo/FBNeo/issues)
- Documentation: [https://github.com/finalburnneo/FBNeo/wiki](https://github.com/finalburnneo/FBNeo/wiki)

## License

FBNeo is licensed under the same terms as the original FinalBurn/FinalBurn Alpha source code. 