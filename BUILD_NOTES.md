# FBNeo Metal Build Notes

This document provides detailed information about building the Metal port of FBNeo for macOS.

## Project Status

The Metal port is now in a working state with a simplified build system. We have successfully implemented:

1. **Consolidated Build System**:
   - Unified `makefile.metal` with all necessary build rules
   - Proper dependency tracking and build order
   - Support for Objective-C++ and Metal-specific code

2. **Core Components**:
   - M68K emulation core with proper stubs
   - Metal integration with device detection
   - Framework for Metal rendering

## Build Requirements

- macOS 10.14 or later
- Xcode command line tools
- Metal-compatible GPU
- C++11 compatible compiler

## Build Process

The build process has been streamlined into a single makefile:

```bash
# Basic build
make -f makefile.metal

# Build and run
make -f makefile.metal run

# Clean build artifacts
make -f makefile.metal clean

# Install to bin directory
make -f makefile.metal install
```

## Build Structure

The build system creates the following directory structure:

```
obj/metal/           - Object files and intermediate build artifacts
  /cpu               - CPU emulation components
    /m68k            - M68K CPU emulation
  /dep/generated     - Generated files
    /m68kops         - M68K opcode files
  /burner            - Main emulator code
bin/metal/           - Final binary installation directory
```

## Key Components

1. **M68K Core**:
   - `m68kmake` - Code generator for M68K emulation
   - `m68kops.h` and `m68kops.c` - Generated opcode handlers
   - `m68kcpu.c` - M68K CPU core

2. **Metal Integration**:
   - `gami_stub.mm` - Metal initialization and device detection
   - `test_main.cpp` - Test harness with stubs for required functions

## How to Test

To verify that your build environment is correctly set up:

```bash
make -f makefile.metal run
```

This will build and run a minimal test application that:
1. Checks for Metal availability on your system
2. Displays basic Metal device information
3. Confirms the build system is working correctly

## Known Issues and Solutions

1. **Build Failure with HD6309 Files**:
   - We have excluded the problematic HD6309 CPU files from the build
   - This is a temporary solution until proper fixes can be implemented

2. **Metal Framework Issues**:
   - If you encounter "framework not found" errors, ensure Xcode command line tools are installed
   - Run `xcode-select --install` to install or update them

3. **Path Issues**:
   - The build system now uses relative paths consistently
   - Directory creation is handled automatically

## Future Work

1. **Full Metal Renderer**:
   - Implement complete rendering pipeline
   - Add shader support for post-processing effects

2. **UI Integration**:
   - Create native macOS interface
   - Add configuration options for Metal-specific features

3. **Performance Optimization**:
   - Implement triple buffering
   - Add multi-threading support

## Build Tips

1. **Clean Build**:
   - Always start with a clean build when switching between different build configurations
   - Use `make -f makefile.metal clean` to remove all artifacts

2. **Debug Information**:
   - Use `make -f makefile.metal debug` to display detailed build information

3. **Installation**:
   - Use `make -f makefile.metal install` to install to the bin directory
   - This makes it easier to run the emulator from a consistent location 