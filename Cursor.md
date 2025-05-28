# FBNeo Metal Backend: Build & Development Guide

## Project Overview

FBNeo (Final Burn Neo) is a multi-system emulator that supports arcade games as well as console systems. This version includes an enhanced Metal backend for improved performance on macOS systems and experimental AI features.

## Directory Structure

- `src/` - Main source code directory
  - `burn/` - Core emulation code
  - `burner/` - System-specific frontend code
    - `metal/` - Metal backend implementation
      - `app/` - Metal application code
      - `fixes/` - Compatibility fixes between C/C++/Objective-C++
      - `ai/` - AI enhancement features
  - `cpu/` - CPU emulation cores
  - `intf/` - Interface code for video, audio, etc.
  - `dep/` - Dependencies and generated files

## Build System

FBNeo Metal uses a sophisticated build system through makefiles:

- `makefile.metal.new` - New main makefile for Metal backend
- `makefile.metal` - Original makefile (partially working)
- `makefile.burn_rules` - Core emulation build rules

## Building FBNeo Metal

### Prerequisites

- macOS (tested on macOS 12+)
- Xcode Command Line Tools
- Clang compiler with Objective-C/C++ support

### Build Commands

To build the Metal backend:

```bash
# Clean build
make -f makefile.metal.new clean

# Full build
make -f makefile.metal.new -j10
```

### Running FBNeo Metal

After building, run the emulator with:

```bash
./run_fbneo_metal.sh
```

Or, to run a specific ROM:

```bash
./run_fbneo_metal.sh /path/to/rom
```

## Metal Backend Architecture

The Metal backend consists of several key components:

1. **Metal Renderer** (`metal_renderer.mm`): Handles rendering using Metal API
2. **Metal Input** (`metal_input.mm`): Manages input devices
3. **Metal Audio** (`metal_audio.mm`): Handles audio playback
4. **Metal Bridge** (`metal_bridge.cpp`): Connects core emulation to Metal frontend

### Objective-C++ Integration

The Metal backend uses a hybrid approach:
- Core emulation code in C/C++
- System interface in Objective-C++
- Custom bridge layer to connect them

## Shader System

Metal shaders used for rendering are located in:
- `src/burner/metal/Shaders.metal` - Basic shaders
- `src/burner/metal/enhanced_metal_shaders.metal` - Advanced effects

The shader compilation process:
1. Metal source files (.metal) are compiled to .air files
2. Air files are linked into a metallib
3. The app loads the metallib at runtime

## Build System Fixes

Recent fixes to the build system include:

1. **C/Objective-C++ Compatibility**:
   - Created proper separation between C and Objective-C++ code
   - Added language-specific header guards
   - Fixed compilation rules for different file types

2. **Build Process Improvements**:
   - Implemented proper dependency tracking
   - Corrected shader compilation and linking
   - Fixed output directory structure

3. **Shader Fixes**:
   - Fixed undefined variable references in shaders
   - Improved compilation flags

## Debugging

To debug FBNeo Metal:

1. Use the debug script:
   ```bash
   ./run_fbneo_metal_debug.sh
   ```

2. Attach Xcode for visual debugging:
   - Open Xcode
   - Debug > Attach to Process > Select fbneo_metal

## Common Issues

- **Build Failures**: Usually caused by missing headers or incorrect file types
- **Shader Compilation**: Check Metal shader syntax errors
- **Runtime Crashes**: Often related to Metal device initialization or shader issues
- **ROM Loading**: Verify ROM paths and formats

## Contributing

When contributing to the Metal backend:

1. Maintain clean separation between C, C++, and Objective-C++ code
2. Use the appropriate extension for each file type (.c, .cpp, .mm)
3. Update both makefiles if adding new files
4. Test thoroughly on different macOS versions

## Documentation

Additional documentation can be found in:
- `docs/Metal_Build_Fixes.md` - Build system fixes
- `docs/Metal_Implementation_Summary.md` - Implementation details
- `METAL_README.md` - General Metal backend documentation

## License

FBNeo is open source software distributed under a custom license. See license.txt for details. 