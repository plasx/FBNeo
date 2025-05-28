#!/bin/bash
# A focused script that only fixes the CPU struct definition issues
set -e

echo "FBNeo Metal Build - CPU Structs Fix Only"
echo "====================================="

# Create necessary directories
mkdir -p src/burner/metal/fixes

# Create a C-style header for functions like cpu_core_config
echo "Creating c_cpp_fixes.h with CPU struct definitions..."
cat > src/burner/metal/fixes/c_cpp_fixes.h << 'EOL'
// General C fixes for the Metal build
#ifndef METAL_C_CPP_FIXES_H
#define METAL_C_CPP_FIXES_H

// Metal build compatibility
#define METAL_BUILD

// TCHAR definition for Metal build
typedef char TCHAR;

// cpu_core_config structure definition
struct cpu_core_config {
    int cpu_type;
    unsigned int clock_speed;
    unsigned int cycles_per_frame;
    int address_bits;
    int address_mask;
    int data_bits;
    int (*init)();
    void (*exit)();
    void (*reset)();
    int (*execute)(int cycles);
    void (*burn_cycles)(int cycles);
    int (*idle_cycles)();
    void (*irq_set)(int state);
    void *data;
};

#endif // METAL_C_CPP_FIXES_H
EOL

# Create the CPU stubs file
echo "Creating metal_cpu_stubs.c..."
cat > src/burner/metal/fixes/metal_cpu_stubs.c << 'EOL'
// Metal CPU stubs for FBNeo
#include "c_cpp_fixes.h"

// Declare missing CPU variables used by metal stub code
// These are empty struct declarations with no initializers

// MegadriveZ80 CPU - referenced from metal_stubs.c
struct cpu_core_config MegadriveZ80 = {0};

// FD1094CPU - referenced from metal_stubs.c
struct cpu_core_config FD1094CPU = {0};  

// MegadriveCPU - referenced from metal_stubs.c
struct cpu_core_config MegadriveCPU = {0};
EOL

# Try to compile just the metal_stubs.c file
echo "Compiling metal_stubs.c with our fixes..."
mkdir -p build/metal/obj/src/burner/metal

echo "Running: clang -g -O2 -Wall -Isrc/dep/generated -Isrc -Isrc/burn -Isrc/burn/devices -Isrc/burn/snd -Isrc/burn/drv -Isrc/burn/drv/capcom -Isrc/burner -Isrc/burner/metal -Isrc/burner/metal/app -Isrc/burner/metal/fixes -Isrc/cpu -Isrc/cpu/m68k -Isrc/cpu/z80 -Isrc/intf/video -Isrc/intf/audio -Isrc/intf/input -I/opt/homebrew/include -Isrc/burn/cpu_generated -DMETAL_BUILD -fcommon -include src/burner/metal/fixes/c_cpp_fixes.h -c src/burner/metal/metal_stubs.c -o build/metal/obj/src/burner/metal/metal_stubs.o"

clang -g -O2 -Wall -Isrc/dep/generated -Isrc -Isrc/burn -Isrc/burn/devices -Isrc/burn/snd -Isrc/burn/drv -Isrc/burn/drv/capcom -Isrc/burner -Isrc/burner/metal -Isrc/burner/metal/app -Isrc/burner/metal/fixes -Isrc/cpu -Isrc/cpu/m68k -Isrc/cpu/z80 -Isrc/intf/video -Isrc/intf/audio -Isrc/intf/input -I/opt/homebrew/include -Isrc/burn/cpu_generated -DMETAL_BUILD -fcommon -include src/burner/metal/fixes/c_cpp_fixes.h -c src/burner/metal/metal_stubs.c -o build/metal/obj/src/burner/metal/metal_stubs.o 2>&1

# Check if the compilation succeeded
if [ $? -eq 0 ]; then
    echo "Successfully compiled metal_stubs.c with our fixes!"
else
    echo "Failed to compile metal_stubs.c. See the error messages above."
    echo "You may need to add more fixes to handle specific issues."
fi

# Also try to compile our CPU stubs file
echo "Compiling metal_cpu_stubs.c..."
clang -g -O2 -Wall -Isrc/dep/generated -Isrc -Isrc/burn -Isrc/burn/devices -Isrc/burn/snd -Isrc/burn/drv -Isrc/burn/drv/capcom -Isrc/burner -Isrc/burner/metal -Isrc/burner/metal/app -Isrc/burner/metal/fixes -Isrc/cpu -Isrc/cpu/m68k -Isrc/cpu/z80 -Isrc/intf/video -Isrc/intf/audio -Isrc/intf/input -I/opt/homebrew/include -Isrc/burn/cpu_generated -DMETAL_BUILD -fcommon -c src/burner/metal/fixes/metal_cpu_stubs.c -o build/metal/obj/src/burner/metal/fixes/metal_cpu_stubs.o 2>&1

# Check if the compilation succeeded
if [ $? -eq 0 ]; then
    echo "Successfully compiled metal_cpu_stubs.c!"
else
    echo "Failed to compile metal_cpu_stubs.c. See the error messages above."
fi

# Create a simplified README with instructions
echo "Creating README.md..."
cat > README.cpu_structs.md << 'EOL'
# FBNeo Metal CPU Structs Fix

This script addresses the "tentative definition has type 'struct cpu_core_config' that is never completed" errors:

1. **cpu_core_config struct definition** - Provides a complete definition in c_cpp_fixes.h
2. **CPU struct instances** - Creates empty instances of MegadriveZ80, FD1094CPU, and MegadriveCPU

## How to use

1. Include these files in your build process:
   - src/burner/metal/fixes/c_cpp_fixes.h
   - src/burner/metal/fixes/metal_cpu_stubs.c

2. Make sure to use -include src/burner/metal/fixes/c_cpp_fixes.h in your compiler flags.

This is a focused fix for just the CPU struct definition issue.
EOL

echo "Fix completed. metal_stubs.c should now compile correctly." 