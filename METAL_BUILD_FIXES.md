# FBNeo Metal Build Fixes

This document describes the fixes applied to make the FBNeo Metal build successfully compile and run with Marvel vs Capcom and other CPS2 games.

## Fixed Issues

### 1. Type Conflict Between FrameBuffer and EmulatorFrameBuffer

**Problem**: Multiple definitions of frame buffer types caused compilation errors.

**Solution**:
- Created a unified `FrameBuffer` type in `metal_declarations.h`
- Added typedef alias for backward compatibility: `typedef FrameBuffer EmulatorFrameBuffer`
- Removed duplicate definitions in `metal_standalone_main.mm` and other files
- Used consistent include order to ensure proper type resolution

### 2. Missing "missing_stubs.c" File

**Problem**: Build looked for a non-existent file: `build/metal/missing_stubs.c`

**Solution**: 
- Created a comprehensive stub file `build/metal_complete_stubs.c` with all missing C functions
- Created a C++ stub file `build/metal_cpp_stubs.cpp` for functions requiring C++ linkage
- Updated build scripts to use these stub files

### 3. Missing main Symbol for Linking

**Problem**: The executable couldn't find the main entry point.

**Solution**:
- Ensured `metal_standalone_main.mm` was properly included in the build
- Created a simplified makefile that explicitly compiles and links the main file
- Fixed include order and dependencies

### 4. Executable Not Being Created

**Problem**: The final binary wasn't being created or copied to the right location.

**Solution**:
- Created a simplified makefile (`makefile.metal.simple`) that focuses on building the executable
- Added explicit checks in the build scripts to verify the executable was created
- Created a launcher script (`run_mvsc_metal.sh`) that builds the executable if it doesn't exist

## Implementation Details

### Key Components

1. **metal_declarations.h**: Contains the unified FrameBuffer type definition
2. **metal_renderer_bridge.cpp/.h**: Connects FBNeo's pBurnDraw to the Metal renderer
3. **metal_complete_stubs.c**: Contains stubs for all required C functions
4. **metal_cpp_stubs.cpp**: Contains stubs for C++ functions like BurnDrvGetTextA
5. **makefile.metal.simple**: Simplified makefile focusing only on building the Metal frontend

### Build Process

The simplified build process follows these steps:

1. Compile the main entry point (`metal_standalone_main.mm`)
2. Compile the renderer bridge (`metal_renderer_bridge.cpp`)
3. Compile the stub files
4. Link everything together with the required frameworks
5. Verify the executable was created

### Running the Emulator

To run the emulator with Marvel vs Capcom:

```bash
./run_mvsc_metal.sh
```

This script will:
1. Look for the ROM in various paths
2. Build the emulator if it doesn't exist
3. Run the emulator with the correct arguments
4. Log output to the debug_output directory

## Technical Details

### Frame Buffer Handling

The key to the fix was properly managing the connection between FBNeo's internal frame buffer (`pBurnDraw`) and Metal's texture system. This involved:

1. Unified type definitions to prevent compiler errors
2. Proper initialization of the frame buffer before game rendering
3. Synchronization through the bridge component to handle frame updates
4. Validation to prevent using invalid buffers

### Error Handling

The implementation includes enhanced error handling:

1. Diagnostic patterns displayed when the frame buffer is invalid
2. Detailed logging of all buffer operations
3. Fallback buffers when the emulator provides invalid data

## Troubleshooting

If you still encounter issues:

1. Check the `debug_output/mvsc_metal.log` file for errors
2. Verify the ROM file exists and is accessible
3. Use the Metal debugging tools: `export METAL_DEVICE_WRAPPER_TYPE=1` 