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
