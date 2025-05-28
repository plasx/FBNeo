#!/bin/bash

# Script to fix CPS2 build issues in FBNeo Metal build

# Make backup of original makefile
cp makefile.metal makefile.metal.cps2.bak

# Create wrappers directory if it doesn't exist
mkdir -p src/burn/drv/capcom/wrappers

# Create directory for fixes if it doesn't exist
mkdir -p src/burner/metal/fixes

# Create the fixes header file with proper inclusion of necessary headers
cat > src/burner/metal/fixes/cps2_fixes.h << 'EOF'
#pragma once

#ifndef CPS2_FIXES_H
#define CPS2_FIXES_H

#include "burnint.h"

// Define missing input variables for CPS2 games and functions needed for compilation
#ifdef __cplusplus
extern "C" {
#endif

extern UINT8 Cps2VolUp;
extern UINT8 Cps2VolDwn;
extern UINT8 AspectDIP;

// Define functions needed by cps_draw.cpp
extern INT32 CpsFindGfxRam(INT32 nLen, INT32 nSize);
extern UINT16* GetPalette(INT32 i);

#ifdef __cplusplus
}
#endif

#endif // CPS2_FIXES_H
EOF

# Create the wrapper for cps_draw.cpp in the capcom driver directory
cat > src/burn/drv/capcom/wrapper_cps_draw.cpp << 'EOF'
#ifndef _CPS_DRAW_WRAPPER_H_
#define _CPS_DRAW_WRAPPER_H_

// This wrapper prevents header conflicts by including the fixes header
// before including the original file

// Define a preprocessor variable to avoid header conflicts
#define DONT_DECLARE_GLOBALS

// Include burn.h for type definitions
#include "burn.h"

// Include the fixes header with the correct relative path
#include "../../../burner/metal/fixes/cps2_fixes.h"

// Include the original file
#include "cps_draw.cpp"

#endif
EOF

# Create the wrapper for cps_rw.cpp
cat > src/burn/drv/capcom/wrapper_cps_rw.cpp << 'EOF'
#ifndef _CPS_RW_WRAPPER_H_
#define _CPS_RW_WRAPPER_H_

// This wrapper prevents header conflicts by including the fixes header
// before including the original file

// Define a preprocessor variable to avoid header conflicts
#define DONT_DECLARE_GLOBALS

// Include burn.h for type definitions
#include "burn.h"

// Include the fixes header with the correct relative path
#include "../../../burner/metal/fixes/cps2_fixes.h"

// Include the original file
#include "cps_rw.cpp"

#endif
EOF

# Create the wrapper for cps_mem.cpp
cat > src/burn/drv/capcom/wrapper_cps_mem.cpp << 'EOF'
#ifndef _CPS_MEM_WRAPPER_H_
#define _CPS_MEM_WRAPPER_H_

// This wrapper prevents header conflicts by including the fixes header
// before including the original file

// Define a preprocessor variable to avoid header conflicts
#define DONT_DECLARE_GLOBALS

// Include burn.h for type definitions
#include "burn.h"

// Include the fixes header with the correct relative path
#include "../../../burner/metal/fixes/cps2_fixes.h"

// Include the original file
#include "cps_mem.cpp"

#endif
EOF

# Update makefile to use wrapper files - use consistent backup extension
sed -i.bak 's|src/burn/drv/capcom/cps_draw.cpp|src/burn/drv/capcom/wrapper_cps_draw.cpp|g' makefile.metal
sed -i.bak 's|src/burn/drv/capcom/cps_rw.cpp|src/burn/drv/capcom/wrapper_cps_rw.cpp|g' makefile.metal
sed -i.bak 's|src/burn/drv/capcom/cps_mem.cpp|src/burn/drv/capcom/wrapper_cps_mem.cpp|g' makefile.metal

# Clean and rebuild
make -f makefile.metal clean
make -f makefile.metal

echo "Build process completed." 