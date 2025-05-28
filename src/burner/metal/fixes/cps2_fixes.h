#ifndef _CPS2_FIXES_H_
#define _CPS2_FIXES_H_

// This file contains Metal-specific fixes for CPS2 emulation
// We avoid redeclaring variables that are already in cps.h

// We use these conditionals to avoid conflicts
#ifdef USE_METAL_RENDERER

// Include necessary headers for Metal integration
#include "metal_declarations.h"

// Metal-specific CPS2 functions
#ifdef __cplusplus
extern "C" {
#endif

// Function to initialize CPS2 core for Metal
int Metal_CPS2Init();

// Function to handle frame rendering for Metal
int Metal_CPS2Frame();

// Function to handle CPS2 exit
int Metal_CPS2Exit();

#ifdef __cplusplus
}
#endif

#endif // USE_METAL_RENDERER

#endif // _CPS2_FIXES_H_ 