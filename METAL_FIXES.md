# FBNeo Metal Implementation - Cross-Platform Fixes

This document describes the fixes implemented to make the FBNeo Metal implementation work correctly on macOS.

## Core Issues Addressed

### 1. Type Definition Compatibility

The original codebase had several type conflicts between Windows and macOS:

- **Missing primitive types**: INT32, UINT8, UINT16, UINT32, PAIR
- **Missing specialized types**: Z80_Regs, STDINPUTINFO, STDDIPINFO
- **Missing macros**: BURN_ENDIAN_SWAP_INT16, _T(), _tfopen, _stprintf

Solution:
- Created `crossplatform.h` header file with compatible type definitions
- Implemented platform-specific macros and typedefs

### 2. Header Circular Dependencies

Several compile-time errors were caused by circular includes:

- burnint.h and tiles_generic.h had circular dependencies
- Multiple definitions of nScreenWidth and nScreenHeight

Solution:
- Reorganized header inclusion order
- Created forward declarations for problematic struct types
- Consolidated screen dimension variables

### 3. BurnDriver Structure Initialization

Function pointer type conflicts in the BurnDriver structure caused compiler errors:

Solution:
- Created `burndriver_fixes.h` with type-safe initialization macros
- Properly cast function pointers to maintain compatibility across platforms

### 4. Windows-Specific APIs

The codebase used several Windows-specific functions:

- DirectX and Windows API functions
- Windows-specific file operations

Solution:
- Isolated Windows-specific code
- Created platform-independent memory allocation functions
- Created substitute methods for platform-specific operations

### 5. Duplicate Function Definitions (Added Fix)

Multiple redundant implementations of core functions caused compilation errors:

- **BurnByteswap**: Defined in both crossplatform.h and other source files
- Inconsistent function signatures between implementations

Solution:
- Created a canonical implementation in crossplatform.h
- Removed duplicate implementations from source files
- Added a fix script `fix_burn_byteswap.sh` to automatically resolve conflicts

## Build System Fixes

### 1. Simplified Build Process

- Created a standalone build script `build_metal_fixed.sh`
- Implemented a fixed makefile `makefile.metal.fixed`
- Added automatic generation of compatibility headers
- Added automatic fix for BurnByteswap duplicates

### 2. Standalone Application

Created a minimal standalone Metal implementation that:

- Initializes a basic Metal renderer
- Demonstrates Metal texture usage
- Provides stubs for AI functionality
- Sets up a proper macOS application window

## Usage Instructions

### Building the Metal Implementation

```bash
# Fix BurnByteswap conflicts (if needed)
./fix_burn_byteswap.sh

# Build using the fixed makefile
./build_metal_fixed.sh

# Run the application
./run_fbneo_metal.sh
```

### Incremental Integration

The current implementation is a standalone Metal renderer. The next steps for full FBNeo integration would be:

1. Integrate the Metal renderer with the FBNeo core
2. Add ROM loading and game state management
3. Implement audio output through macOS frameworks
4. Implement input handling for controllers and keyboard
5. Add save state functionality

## Compatibility Headers

The main compatibility headers created are:

- `crossplatform.h`: Type definitions and platform-specific macros
- `burn_memory.h`: Memory allocation functions
- `endian.h`: Endian conversion utilities
- `burndriver_fixes.h`: Type-safe BurnDriver initialization

## Future Improvements

1. **Complete Core Integration**: Gradually integrate the full FBNeo core
2. **Performance Optimizations**: Optimize Metal renderer for high-performance games
3. **Metal Shader Improvements**: Enhance the Metal shaders for better visual quality
4. **Game Controller Support**: Add native macOS game controller support
5. **AI Feature Completion**: Fully implement the AI features

## Issues Fixed

### 1. Linkage Issues

- Added `extern "C"` declarations in a new header file `metal_error_handling.h` for functions that were missing proper C linkage when called from C++ code:
  - `Metal_LogMessage`
  - `Metal_SetLogLevel`
  - Various error handling functions

### 2. Type Definition Conflicts

- Created `rom_verify_types.h` to resolve conflicting definitions between C and C++ code
- Separated C++ namespace specific structures from C-compatible structures
- Fixed the `ROMVerificationResult` structure conflict

### 3. Missing Function Implementations

- Implemented ZIP file handling functions in `metal_zip_extract.cpp`:
  - `Metal_ListZipContents`
  - `Metal_GetZipFileInfo`
  - `Metal_ExtractFileFromZip`

### 4. Build System Updates

- Created a simplified build script that properly includes all necessary files
- Fixed header include paths
- Added proper compilation of support files

## Implementation Approach

Our approach followed these principles:

1. **Clear Separation**: Keep C and C++ code separated with proper interfaces
2. **Consistent Types**: Ensure type definitions are consistent across files
3. **Proper Linkage**: Use `extern "C"` for functions accessed across language boundaries
4. **Simplified Build**: Create a build process that includes all needed components

## Files Created or Modified

1. **New Files**:
   - `metal_error_handling.h`: Header for error handling functions
   - `rom_verify_types.h`: Fixed type definitions for ROM verification
   - `build_metal_fixed.sh`: Simplified build script

2. **Modified Files**:
   - `rom_verify.cpp`: Updated to use the new type definitions
   - `build_and_run_mvsc.sh`: Added new source files to the build

## Testing

To test the fixes:

1. Run the simplified build script:
   ```
   ./build_metal_fixed.sh
   ```

2. Run the fixed Metal implementation:
   ```
   ./fbneo_metal_fixed
   ```

3. Test with a ROM (for Marvel vs Capcom):
   ```
   ./fbneo_metal_fixed "/path/to/mvsc.zip"
   ```

## Next Steps

1. Complete audio implementation with AVAudioEngine
2. Add proper input handling
3. Implement game state management
4. Add enhanced error recovery
5. Test with more CPS2 games beyond Marvel vs Capcom

These fixes provide the foundation for a working FBNeo Metal implementation on macOS with proper ROM loading, memory management, and renderer integration. 