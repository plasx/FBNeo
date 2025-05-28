#!/bin/bash

# FBNeo Metal Full Build Script
# This script builds the Metal backend with all necessary fixes

set -e  # Exit on error

echo "=== FBNeo Metal Full Build Script ==="
echo "Starting build process..."

# Ensure required directories exist
mkdir -p obj/metal/burner/metal/fixes
mkdir -p bin/metal

# Clean previous build artifacts
echo "Cleaning previous build..."
make -f makefile.metal clean

# Fix shader issues first - replace undefined variables in shader file
echo "Fixing shader file..."
sed -i '' 's/color = (frac.x < 0.5) ? colorL : colorR;/color = (frac.x < 0.5) ? colorTL : colorTR;/g' src/burner/metal/Shaders.metal

# Add include guards to problematic header files
echo "Adding include guards to problematic header files..."

# Create a list of header files that need guards
HEADERS_TO_GUARD=(
  "src/burn/tiles_generic.h"
  "src/burn/tilemap_generic.h"
)

for header in "${HEADERS_TO_GUARD[@]}"; do
  if [ -f "$header" ]; then
    # Check if the header already has include guards
    if ! grep -q "#ifndef" "$header" || ! grep -q "#define" "$header"; then
      # Get the header name for the guard
      GUARD_NAME=$(basename "$header" | tr '[:lower:].' '[:upper:]_')
      
      # Create a temporary file with guards
      echo "#ifndef ${GUARD_NAME}_INCLUDE" > "${header}.tmp"
      echo "#define ${GUARD_NAME}_INCLUDE" >> "${header}.tmp"
      echo "" >> "${header}.tmp"
      cat "$header" >> "${header}.tmp"
      echo "" >> "${header}.tmp"
      echo "#endif // ${GUARD_NAME}_INCLUDE" >> "${header}.tmp"
      
      # Replace the original file
      mv "${header}.tmp" "$header"
      echo "Added include guards to $header"
    else
      echo "$header already has include guards"
    fi
  else
    echo "Warning: $header not found"
  fi
done

# Create necessary patch directories if they don't exist
mkdir -p src/burner/metal/fixes

# Create C-compatible function declarations for Metal compatibility
echo "Creating compatibility layer for Metal..."
cat > src/burner/metal/fixes/metal_patched_includes.h << 'EOF'
#pragma once

// Metal backend C/C++ compatibility layer
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// Common type definitions to prevent redefinition issues
#ifndef TYPES_DEFINED
#define TYPES_DEFINED
typedef int32_t INT32;
typedef uint32_t UINT32;
typedef int16_t INT16;
typedef uint16_t UINT16;
typedef int8_t INT8;
typedef uint8_t UINT8;
#endif

// Declare missing memory functions if needed
#ifndef METAL_MEMORY_FUNCTIONS
#define METAL_MEMORY_FUNCTIONS
#ifndef BurnMalloc
#define BurnMalloc(size) malloc(size)
#endif

#ifndef BurnFree
#define BurnFree(ptr) free(ptr)
#endif
#endif // METAL_MEMORY_FUNCTIONS

// Debug flags
#ifndef METAL_DEBUG_FLAGS
#define METAL_DEBUG_FLAGS
#ifndef Debug_BurnGunInitted
extern int Debug_BurnGunInitted;
#endif
#endif // METAL_DEBUG_FLAGS

// Sound initialization
#ifndef BurnSoundInit
void BurnSoundInit() {
    // Stub implementation - actual implementation in burn_sound.cpp
}
#endif

// Fix for Metal build
#define BUILD_METAL 1
EOF

# Create implementations for missing functions
echo "Creating implementations for missing functions..."
cat > src/burner/metal/fixes/metal_stubs.c << 'EOF'
#include "metal_patched_includes.h"

// Define debug variables
int Debug_BurnGunInitted = 0;

// Additional Metal-specific stubs
void BurnSetMouseDivider(int divider) {
    // Stub implementation
}

int AnalogDeadZone(int input) {
    // Simple deadzone implementation
    const int DEADZONE = 10;
    if (input > -DEADZONE && input < DEADZONE) {
        return 0;
    }
    return input;
}

int ProcessAnalog(int value, int min, int deadzone, int center, int max) {
    // Basic analog processing
    if (value < center) {
        if (value < center - deadzone) {
            return min + (center - value) * (center - min) / (center - deadzone);
        }
        return center;
    } else {
        if (value > center + deadzone) {
            return center + (value - center) * (max - center) / (deadzone);
        }
        return center;
    }
}

// Define other required globals
#define INPUT_DEADZONE 10
EOF

# Create a missing header for compatibility
echo "Creating missing headers..."
cat > src/burner/metal/fixes/c_cpp_compatibility.h << 'EOF'
#pragma once

// Common type definitions
typedef int32_t INT32;
typedef uint32_t UINT32;
typedef int16_t INT16; 
typedef uint16_t UINT16;
typedef int8_t INT8;
typedef uint8_t UINT8;
EOF

# Create a Metal shader compilation script
echo "Creating Metal shader compilation script..."
cat > compile_metal_shaders.sh << 'EOF'
#!/bin/bash
echo "Compiling Metal shaders..."
mkdir -p obj/metal/burner/metal
xcrun -sdk macosx metal -c src/burner/metal/Shaders.metal -o obj/metal/burner/metal/Shaders.air
xcrun -sdk macosx metallib obj/metal/burner/metal/Shaders.air -o obj/metal/burner/metal/Shaders.metallib
mkdir -p bin/metal
cp obj/metal/burner/metal/Shaders.metallib bin/metal/
echo "Metal shaders compiled and copied."
EOF

chmod +x compile_metal_shaders.sh

# Compile the Metal shaders
echo "Compiling Metal shaders..."
./compile_metal_shaders.sh

# Now we'll manually compile the necessary C files first
echo "Compiling critical Metal files..."

# Create obj directories
mkdir -p obj/metal/burner/metal/fixes

# Compile metal_renderer_stubs.c
echo "Compiling metal_renderer_stubs.c..."
clang -g -O2 -Wall -Isrc/dep/generated -Isrc -Isrc/burn -Isrc/burn/devices \
      -Isrc/burn/snd -Isrc/burn/drv -Isrc/burn/drv/capcom -Isrc/burner \
      -Isrc/burner/metal -Isrc/burner/metal/app -Isrc/burner/metal/fixes \
      -Isrc/cpu -Isrc/cpu/m68k -Isrc/cpu/z80 -Isrc/intf/video -Isrc/intf/audio \
      -Isrc/intf/input -I/opt/homebrew/include -Isrc/burn/cpu_generated \
      -DMETAL_BUILD -fcommon -c src/burner/metal/fixes/metal_renderer_stubs.c \
      -o obj/metal/burner/metal/fixes/metal_renderer_stubs.o

# Compile metal_stubs.c with C++ compiler
echo "Compiling metal_stubs.c with C++..."
clang++ -g -O2 -Wall -Isrc/dep/generated -Isrc -Isrc/burn -Isrc/burn/devices \
       -Isrc/burn/snd -Isrc/burn/drv -Isrc/burn/drv/capcom -Isrc/burner \
       -Isrc/burner/metal -Isrc/burner/metal/app -Isrc/burner/metal/fixes \
       -Isrc/cpu -Isrc/cpu/m68k -Isrc/cpu/z80 -Isrc/intf/video -Isrc/intf/audio \
       -Isrc/intf/input -I/opt/homebrew/include -Isrc/burn/cpu_generated \
       -DMETAL_BUILD -fcommon -std=c++17 -Wno-deprecated-declarations \
       -Wno-writable-strings -Wno-c++11-narrowing -Wno-missing-braces \
       -Wno-incompatible-pointer-types -fpermissive \
       -c src/burner/metal/metal_stubs.c -o obj/metal/burner/metal/metal_stubs.o

# Create a custom makefile that excludes our manually compiled files
echo "Creating custom makefile..."
cat > makefile.metal.custom << 'EOF'
# Include the original makefile
include makefile.metal

# Override the rule for metal_stubs.c since we compile it manually
$(OBJDIR)/src/burner/metal/metal_stubs.o: src/burner/metal/metal_stubs.c
	@echo "Skipping metal_stubs.c (compiled manually)"

# Add the fixes directory to the include path
INCLUDES += -Isrc/burner/metal/fixes

# Add C++ flags to properly support Metal and Objective-C++
CXXFLAGS += -Wno-everything

# Ensure we link with our manually compiled objects
ADDITIONAL_OBJS = obj/metal/burner/metal/fixes/metal_renderer_stubs.o

# Modify the linking rule to include our manually compiled objects
$(TARGET): $(OBJS) $(METAL_LIB)
	@mkdir -p $(BUILDDIR)
	$(CXX) -o $@ $(OBJS) $(ADDITIONAL_OBJS) $(LDFLAGS)
	@echo "Build complete: $(TARGET)"
EOF

# Now run the build with our custom makefile
echo "Starting main build process..."
make -f makefile.metal.custom -j10

# Check if build was successful
if [ -f "fbneo_metal" ]; then
    echo "=== Build successful! ==="
    echo "Binary created: fbneo_metal"
    
    # Create a directory for the binary if it doesn't exist
    mkdir -p bin/metal
    
    # Copy binary to bin/metal/
    cp fbneo_metal bin/metal/
    echo "Copied binary to bin/metal/"
    
    # Make sure it's executable
    chmod +x bin/metal/fbneo_metal
    echo "Made binary executable"
else
    echo "=== Build failed! ==="
    exit 1
fi

echo "Build process completed."
exit 0 