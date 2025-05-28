# FBNeo Metal Build Fixes

This directory contains compatibility fixes for building FBNeo with Metal on macOS.

## Key Fixes

1. **C/C++ Compatibility**
   - Fixed `bool` type for C code
   - Fixed struct tag requirements for C code
   - Fixed enum tag requirements for C code
   - Avoided conflicts with Lua's `tm` variable name

2. **BurnDriver Fixes**
   - Implemented a modified BurnDriver struct with void* pointers for function pointers
   - Added stubs for Megadrive driver functions
   - Fixed missing braces warnings in array initializers

3. **Build System Integration**
   - Added special handling for Lua files to avoid conflicts with our fixes
   - Suppressed unnecessary warnings that don't affect functionality

## Usage

These fixes are automatically applied when building with `makefile.metal`:

```bash
make -f makefile.metal clean && make -f makefile.metal -j10
```

## Running FBNeo Metal

Once built, you can run the emulator with:

```bash
./fbneo_metal /path/to/your/rom.zip
```

For example:
```bash
./fbneo_metal /Users/plasx/dev/ROMs/mvsc.zip
```

## Technical Details

The fixes in this directory address several compatibility issues:

1. **BurnDriver Initialization**
   - `burndriver_fix.h`: Provides a modified BurnDriver struct that uses void* for function pointers to avoid type conversion errors during initialization.

2. **C/C++ Compatibility**
   - `c_cpp_fixes.h`: Addresses differences between C and C++ (bool, struct tags, etc.)

3. **Megadrive Driver Stubs**
   - `burn_stubs.cpp`: Provides stub implementations for Megadrive driver functions to fix initialization issues in `d_megadrive.cpp`.

## Troubleshooting

If you encounter build errors:

1. Ensure all fix files are in place
2. Check for missing frameworks
3. Make sure the Metal headers are correctly imported 