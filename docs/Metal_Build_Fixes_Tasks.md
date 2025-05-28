# FBNeo Metal Build Error Fixes - Task List

## Priority 1: C/C++ Compatibility Layer Issues

1. **Fix Struct/Enum Tags in Header Files**
   - [ ] Fix missing `struct` tag for `CheatInfo` in `src/burn/cheat.h`
   - [ ] Fix missing `enum` tag for `BurnCartrigeCommand` in `src/burn/burn.h` 
   - [ ] Fix missing `struct` tag for `RomDataInfo` in `src/burn/burn.h`
   - [ ] Fix missing `struct` tag for `BurnRomInfo` in `src/burn/burn.h`
   - [ ] Fix missing `struct` tag for `tm` in time-related functions
   - [ ] Fix missing `struct` tag for `clip_struct` in `src/burn/burn_bitmap.h`
   - [ ] Fix missing `struct` tag for `GenericTilesGfx` in `src/burn/tiles_generic.h`

2. **Fix Default Arguments in Functions**
   - [ ] Fix `IpsApplyPatches` function (in `burn.h`) with default `readonly` argument
   - [ ] Fix `GenericTilemapDraw` function with default `priority_mask` argument
   - [ ] Create C-compatible wrapper functions without default arguments

3. **Fix Game Genre Macro Expansion Issues**
   - [ ] Replace direct usage of `GBF_HORSHOOT`, `GBF_VERSHOOT` macros in variable declarations
   - [ ] Modify `genre_variables.c` to use proper type casting for bit-shift expressions
   - [ ] Ensure proper scope for genre variable definitions

## Priority 2: Macro Handling Issues

1. **Fix Generic Tilemap Macros**
   - [ ] Rework `GenericTilemapSetOffsets` macro to avoid _Generic C11 expression
   - [ ] Create separate C-compatible implementations for different parameter counts
   - [ ] Ensure function prototypes match actual definitions

2. **Create Header Patch Files**
   - [ ] Create patched versions of problematic headers:
     - [ ] `patched_burn.h` with proper struct/enum tags
     - [ ] `patched_cheat.h` with struct tags
     - [ ] `patched_tiles_generic.h` with fixed macro definitions
   - [ ] Update includes in Metal-specific files to use patched headers

## Priority 3: Build System Improvements

1. **Selective Compilation Modes**
   - [ ] Update makefile to compile problematic files in C++ mode
   - [ ] Identify minimal set of files that must be compiled in C mode
   - [ ] Create separate compiler flag sets for C/C++ files

2. **Include Path Management**
   - [ ] Create a separate include directory for patched headers
   - [ ] Modify include search order to prioritize patched headers
   - [ ] Add header guards to prevent double inclusion issues

3. **Stub Implementation Improvements**
   - [ ] Create unified stub system with dependency injection capability
   - [ ] Replace direct access to problematic structures with accessor functions
   - [ ] Implement isolation layer between Metal code and core FBNeo code

## Priority 4: AI Integration Preparation

1. **Clean API Design**
   - [ ] Finalize C API for AI interface in `ai_interface.h`
   - [ ] Create type-safe pointer passing between C/C++ layers
   - [ ] Define clear ownership semantics for resources

2. **Metal Framework Integration**
   - [ ] Create isolated Metal interface that doesn't depend on FBNeo internals
   - [ ] Add proper bridging between Objective-C++ and C
   - [ ] Build CoreML wrapper that works in C context

## Implementation Notes

### Struct Tag Fixing Approach

Replace declarations like:
```c
extern CheatInfo* pCheatInfo;
```

With properly tagged versions:
```c
extern struct CheatInfo* pCheatInfo;
```

### Default Argument Handling

Replace functions with default arguments:
```c
void IpsApplyPatches(unsigned char* base, char* rom_name, unsigned int rom_crc, bool readonly = false);
```

With C-compatible wrapper functions:
```c
// Original C++ function (in C++ files)
void IpsApplyPatches(unsigned char* base, char* rom_name, unsigned int rom_crc, bool readonly = false);

// C wrapper function (for C files)
void IpsApplyPatches_C(unsigned char* base, char* rom_name, unsigned int rom_crc, bool readonly);
```

### Macro Expansion Issues

Replace direct usage of bit-shift macros:
```c
void* GBF_HORSHOOT = NULL; // Expands to: void* (1 << 0) = NULL; (invalid)
```

With proper variable declarations:
```c
// In a header file
extern const unsigned int GBF_HORSHOOT_VALUE;

// In an implementation file
const unsigned int GBF_HORSHOOT_VALUE = (1 << 0);
``` 