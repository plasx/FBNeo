#!/bin/bash
# Fix for metal_c_linkage_functions.c bool type issue
set -e

echo "Fixing metal_c_linkage_functions.c bool type issue..."

# Add stdbool.h to the file
sed -i '' '8i\
#include <stdbool.h> /* For bool type */
' src/burner/metal/fixes/metal_c_linkage_functions.c

echo "Fix applied. Try rebuilding with:"
echo "make -f makefile.metal.fixed" 