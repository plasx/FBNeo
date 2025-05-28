# ROM Loading System Fixes for FBNeo Metal

This document summarizes the changes made to fix the ROM loading system in the Metal port of FBNeo for Marvel vs Capcom.

## Main Changes

1. **Implemented Real ROM Loading in `burn_rom.cpp`**
   - Replaced dummy pattern generation with actual file loading
   - Added proper error checking and reporting
   - Added CRC verification using zlib's crc32 function
   - Added file existence and size verification

2. **Enhanced `Metal_LoadROM` in `metal_bridge.cpp`**
   - Added detailed ROM verification before driver initialization
   - Added better error reporting with specific messages
   - Added system to detect and report missing ROM files
   - Added support for finding drivers by name

3. **Removed Test Pattern Generation**
   - Added deprecation warnings to `Metal_GenerateTestPattern`
   - Updated `Metal_RunFrame` to avoid falling back to test patterns
   - Ensured proper error reporting when frame buffer is empty

4. **Updated Build System**
   - Added `burn_rom.o` to the build objects
   - Ensured zlib is properly linked with `-lz`
   - Added additional zlib components for compression support
   - Created build script for easy compilation

5. **Added Documentation**
   - Created `ROM_SETUP.md` with detailed ROM setup instructions
   - Added comments explaining the ROM loading process
   - Added debug output to help diagnose ROM loading issues

## Technical Details

### ROM Loading Process

The ROM loading process now follows these steps:

1. User specifies ROM directory path
2. System verifies ROM directory exists
3. `Metal_LoadROM` scans for all required ROM files before initialization
4. During driver initialization, `BurnLoadRom` loads each ROM file
5. Each ROM is verified for:
   - Existence (file must exist)
   - Size (file must be at least as large as expected)
   - CRC (optional verification, warnings if mismatched)
6. All ROM data is loaded into memory for the emulator

### ROM File Format

The system is designed to work with ROM files extracted from the original ZIP archive. It:

1. Looks for individual ROM files in the specified directory
2. Reports detailed errors if files are missing or incorrect
3. Provides guidance on extracting ROMs from ZIP archives

## Future Improvements

Future improvements could include:

1. Direct loading from ZIP archives
2. Support for compressed ROM formats
3. More robust ROM set validation
4. Enhanced error recovery
5. Better reporting of required vs. optional ROMs

## Testing

The ROM loading system has been tested with:

1. Complete ROM sets extracted to the correct directory
2. Partial ROM sets with missing files
3. Invalid ROM files with incorrect sizes
4. Various ROM paths and configurations

In all cases, the system provides appropriate error messages and behaves correctly.

## Dependencies

The ROM loading implementation depends on:
- zlib (for CRC calculation)
- Standard POSIX file I/O functions
- dirent.h for directory operations 