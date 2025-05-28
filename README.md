# FBNeo Metal Build Complete Fix

This script provides a comprehensive fix for the FBNeo Metal build, addressing multiple issues:

1. **BurnDriver struct redefinition** - Prevents the struct from being defined in multiple places
2. **Type inconsistencies** - Makes nBurnDrvActive consistently 'int' across all files
3. **Missing CPU structs** - Adds empty struct declarations for Megadrive and FD1094 CPUs
4. **const qualifier issues** - Fixes string literal assignment to non-const pointers
5. **Missing function definitions** - Adds implementations for functions like BurnSoundInit

## How the fixes work:

1. **Type Consistency Fix**:
   - Makes `nBurnDrvActive` definition use `int` in both burn.h and burnint.h
   - Explicitly changes the declaration in burn.cpp to match

2. **BurnDriver Struct Fix**:
   - Creates special headers for burn.cpp and similar files that need the full struct definition
   - Avoids redefinition by using SKIP_DRIVER_DEFINITION in other files
   - Makes struct definitions consistent across all files

3. **Const Qualifier Fix**:
   - Provides macros to safely cast const strings to non-const when needed
   - Makes string literals usable with functions expecting non-const char pointers

4. **CPU Struct Fix**:
   - Provides empty struct declarations for CPU structs referenced in metal_stubs.c
   - Avoids the "tentative definition" errors for CPU structs

5. **Build System Fix**:
   - Creates a modified makefile with special rules for problematic files
   - Uses custom include paths to ensure patched headers take precedence
   - Excludes known problematic files that can't be fixed easily

## How to build:

1. Run the script:
   ```
   ./fix_metal_build.sh
   ```

2. The script will:
   - Create all necessary directories, files and symlinks
   - Apply fixes to the source code
   - Set up a patched build system

3. Build using the fixed makefile:
   ```
   make -f makefile.metal.fixed
   ```

## Note on Modified Files

The script makes backups of any files it modifies:
- burn.h.bak - Original burn.h file
- burn.cpp.bak - Original burn.cpp file
- burnint.h.bak - Original burnint.h file

These will be automatically restored when you exit the script with Ctrl+C.

## Troubleshooting:

If you still see errors:

1. Try cleaning the build directory:
   ```
   make -f makefile.metal.fixed clean
   ```

2. If specific files are causing issues, you can add them to the exclude pattern in the makefile.

# FBNeo Metal Renderer for macOS

Final Burn Neo (FBNeo) emulator with Metal renderer for macOS.

## Features

- **Native Metal Renderer** - Optimized for Apple Silicon and Intel Macs
- **Modern Controller Support** - Complete GameController framework integration
- **Shader Support** - Multiple shader options including CRT emulation
- **Input Configuration** - GUI for configuring keyboard and controller mappings
- **Per-Game Configurations** - Save and load settings per game

## Building

To build the enhanced version with all features (recommended):

```bash
make -f makefile.metal enhanced
```

Or use the included build script:

```bash
./build_and_run.sh [optional_rom_path]
```

## GameController Support

The Metal renderer has complete support for the Apple GameController framework, providing:

- Support for Xbox, PlayStation, and MFi controllers
- Hot-plugging and automatic player assignment
- Per-game controller configurations
- Full button remapping through the configuration UI

## Input Configuration

To access the input configuration:

1. From the menu bar: **Configuration → Input Settings for Game** (⌘⇧I)
2. From the menu bar: **Configuration → Global Input Settings** (⌘I)

The configuration window allows you to:

- Remap keyboard keys for game controls
- Configure controller buttons
- Save controller configurations per game
- Adjust display settings

## Default Key Mappings

The default keyboard controls are:

- **Arrow Keys**: D-Pad
- **Z, X, C, A, S, D**: Buttons 1-6
- **1, 2**: Coin, Start
- **Space**: Pause
- **F1**: Menu
- **F3, F4**: Save/Load State
- **Escape**: Quit

## Documentation

For more detailed information, see:

- [CONTROLLER_CONFIG.md](src/burner/metal/CONTROLLER_CONFIG.md) - Controller configuration documentation
- [CONTROLLER_SUMMARY.md](src/burner/metal/CONTROLLER_SUMMARY.md) - Technical summary of controller implementation
- [SUMMARY.md](src/burner/metal/SUMMARY.md) - Overall renderer implementation summary
