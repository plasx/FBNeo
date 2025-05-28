#!/bin/bash
# Fix burnint.h to avoid cpu_core_config redefinition
set -e

echo "Creating a patched version of burnint.h..."

# 1. Create a patched version of burnint.h that defines CPU_CORE_CONFIG_DEFINED before the struct
sed '123i\
#define CPU_CORE_CONFIG_DEFINED
' src/burn/burnint.h > src/burn/burnint.h.patched

# 2. Backup original file and replace with patched version
mv src/burn/burnint.h src/burn/burnint.h.original
mv src/burn/burnint.h.patched src/burn/burnint.h

echo "burnint.h patched successfully." 