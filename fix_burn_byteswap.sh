#!/bin/bash
# Script to fix BurnByteswap redundancy issues

# Print header
echo "====================================================="
echo "FBNeo Metal Cross-Platform Compatibility Patch Script"
echo "====================================================="
echo "This script fixes redundant BurnByteswap implementations"
echo "and resolves BurnMalloc/BurnFree conflicts"
echo ""

# Backup all files we're going to modify
echo "Creating backups of modified files..."
mkdir -p backups
cp -f src/burn/crossplatform.h backups/crossplatform.h.bak 2>/dev/null || :
cp -f src/burner/metal/fixes/burn_patched.cpp backups/burn_patched.cpp.bak 2>/dev/null || :
cp -f src/burner/metal/fixes/load_patched.cpp backups/load_patched.cpp.bak 2>/dev/null || :
cp -f src/burner/metal/fixes/burn_memory.h backups/burn_memory.h.bak 2>/dev/null || :
cp -f src/burner/metal/fixes/burn_memory.cpp backups/burn_memory.cpp.bak 2>/dev/null || :

# Make sure the burn directory exists
if [ ! -d "src/burn" ]; then
    echo "Error: src/burn directory not found!"
    echo "Please run this script from the FBNeo root directory."
    exit 1
fi

# Make sure the metal fixes directory exists
if [ ! -d "src/burner/metal/fixes" ]; then
    echo "Creating src/burner/metal/fixes directory..."
    mkdir -p src/burner/metal/fixes
fi

# Step 1: Update crossplatform.h with canonical BurnByteswap
echo "Step 1: Updating crossplatform.h with canonical BurnByteswap definition..."
cat > src/burn/crossplatform.h << 'EOL'
#ifndef _BURN_CROSSPLATFORM_H
#define _BURN_CROSSPLATFORM_H

// Cross-platform definitions for compatibility between different platforms
#include <stdint.h>

// First include burn.h to get data type definitions
#include "burn.h"

#ifdef __cplusplus
extern "C" {
#endif

// Define PAIR type for 32-bit registers with 16-bit parts access
typedef union {
#ifdef LSB_FIRST
    struct { UINT8 l,h,h2,h3; } b;
    struct { UINT16 l,h; } w;
#else
    struct { UINT8 h3,h2,h,l; } b;
    struct { UINT16 h,l; } w;
#endif
    UINT32 d;
} PAIR;

// Byte swapping function - canonical definition for all files
// This must be used in all places that need to swap bytes
static inline void BurnByteswap(UINT8* pData, INT32 nLen) {
    int i;
    if (!pData) return;
    
    for (i = 0; i < nLen; i += 2) {
        if (i + 1 >= nLen) break; // Prevent buffer overrun
        UINT8 temp = pData[i];
        pData[i] = pData[i+1];
        pData[i+1] = temp;
    }
}

// Provide these functions if they don't exist
#ifndef _WIN32
// BurnSetRefreshRate - Sets emulator frame rate
void BurnSetRefreshRate(double dRefreshRate);

// BurnLoadRom - Load a ROM with byteswapping option
INT32 BurnLoadRom(UINT8* pDest, INT32 nNum, INT32 nPass);

// HiscoreReset - Reset high score system for a new game
INT32 HiscoreReset();
#endif

// Undefine Windows-specific macros that might cause issues
#ifdef _MSC_VER
#undef _T
#undef _TEXT
#undef _tfopen
#undef _stprintf
#endif

// Include platform-specific tchar.h 
#include "tchar.h"

#ifdef __cplusplus
}
#endif

#endif // _BURN_CROSSPLATFORM_H
EOL
echo "  Done."

# Step 2: Create the fixed burn_memory.h
echo "Step 2: Creating fixed burn_memory.h to avoid macro conflicts..."
mkdir -p src/burner/metal/fixes
cat > src/burner/metal/fixes/burn_memory.h << 'EOL'
#ifndef _BURN_MEMORY_H_
#define _BURN_MEMORY_H_

#include <stdlib.h>

// Memory allocation functions for Metal build
// These have names that won't conflict with the macros in other files
void* Metal_BurnMalloc(size_t size);
void Metal_BurnFree(void* ptr);

// Create macros only if they aren't already defined
#ifndef BurnMalloc
#define BurnMalloc(x) Metal_BurnMalloc(x)
#endif

#ifndef BurnFree
#define BurnFree(x) do { Metal_BurnFree(x); x = NULL; } while (0)
#endif

#endif // _BURN_MEMORY_H_
EOL
echo "  Done."

# Step 3: Create the fixed burn_memory.cpp
echo "Step 3: Creating fixed burn_memory.cpp to avoid macro conflicts..."
cat > src/burner/metal/fixes/burn_memory.cpp << 'EOL'
#include "burn_memory.h"
#include <stdlib.h>
#include <stdio.h>

// Memory allocation tracking for the emulator
void* Metal_BurnMalloc(size_t size)
{
    void* ptr = malloc(size);
    if (!ptr) {
        printf("Metal_BurnMalloc failed to allocate %zu bytes\n", size);
        return NULL;
    }
    return ptr;
}

void Metal_BurnFree(void* ptr)
{
    if (ptr) {
        free(ptr);
    }
}
EOL
echo "  Done."

# Step 4: Update burn_patched.cpp to use the canonical BurnByteswap
if [ -f "src/burner/metal/fixes/burn_patched.cpp" ]; then
    echo "Step 4: Updating burn_patched.cpp to use the canonical BurnByteswap..."
    # Check if the file contains BurnByteswap
    if grep -q "void BurnByteswap" src/burner/metal/fixes/burn_patched.cpp; then
        # Use sed to remove the BurnByteswap function and add the include
        sed -i.bak '
        # Add include for crossplatform.h
        /^#include "metal_declarations.h"/a \
#include "../../../burn/crossplatform.h" // Include for the canonical BurnByteswap

        # Remove BurnByteswap function
        /^\/\/ Add missing BurnByteswap function/,/^}/c \
// No need for BurnByteswap function here - use the one from crossplatform.h
        ' src/burner/metal/fixes/burn_patched.cpp
        
        # Remove backup file
        rm src/burner/metal/fixes/burn_patched.cpp.bak
        echo "  Done."
    else
        echo "  No BurnByteswap function found in burn_patched.cpp."
    fi
else
    echo "Step 4: burn_patched.cpp not found, skipping."
fi

# Step 5: Update load_patched.cpp to use the canonical BurnByteswap and fix BurnMalloc/Free
if [ -f "src/burner/metal/fixes/load_patched.cpp" ]; then
    echo "Step 5: Updating load_patched.cpp to use canonical functions..."
    # Check if the file contains BurnByteswap
    if grep -q "void BurnByteswap" src/burner/metal/fixes/load_patched.cpp; then
        # Use sed to remove the BurnByteswap function and add the include
        sed -i.bak '
        # Add include for crossplatform.h
        /^#include "metal_declarations.h"/a \
#include "../../../burn/crossplatform.h" // Include for the canonical BurnByteswap

        # Remove BurnByteswap function
        /^\/\/ Byte swap function/,/^}/c \
// No need for BurnByteswap function here - use the one from crossplatform.h
        ' src/burner/metal/fixes/load_patched.cpp
    fi
    
    # Fix BurnMalloc/BurnFree
    if grep -q "void\* BurnMalloc" src/burner/metal/fixes/load_patched.cpp; then
        sed -i.bak '
        # Change BurnMalloc to Metal_BurnMalloc
        s/void\* BurnMalloc/void* Metal_BurnMalloc/g
        s/printf("BurnMalloc called/printf("Metal_BurnMalloc called/g
        
        # Change BurnFree to Metal_BurnFree
        s/void BurnFree/void Metal_BurnFree/g
        s/printf("BurnFree called/printf("Metal_BurnFree called/g
        
        # Update RomFind to use Metal_BurnMalloc
        s/return (UINT8\*)BurnMalloc/return (UINT8*)Metal_BurnMalloc/g
        ' src/burner/metal/fixes/load_patched.cpp
    fi
    
    # Remove backup file
    rm -f src/burner/metal/fixes/load_patched.cpp.bak
    echo "  Done."
else
    echo "Step 5: load_patched.cpp not found, skipping."
fi

echo ""
echo "Patch complete! You can now run the build_metal_fixed.sh script."
echo "If you run into problems, original files were backed up to the 'backups' directory." 