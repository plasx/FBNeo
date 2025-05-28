#pragma once

// Metal backend C/C++ compatibility layer
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// Include our fixes for Metal builds
#include "c_cpp_fixes.h"
#include "m68k_fixes.h"

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

// Do NOT define sprintf macro here - it causes conflicts with system headers

// Declare memory functions without using macros
#ifndef METAL_MEMORY_FUNCTIONS
#define METAL_MEMORY_FUNCTIONS

// These functions will be provided as actual implementations
void* Metal_BurnMalloc(size_t size);
void Metal_BurnFree(void* ptr);

#endif // METAL_MEMORY_FUNCTIONS

// Debug flags
#ifndef METAL_DEBUG_FLAGS
#define METAL_DEBUG_FLAGS
#ifndef Debug_BurnGunInitted
extern int Debug_BurnGunInitted;
#endif
#endif // METAL_DEBUG_FLAGS

// Sound initialization declaration
#ifndef SOUND_FUNCTIONS_DECLARED
#define SOUND_FUNCTIONS_DECLARED
void BurnSoundInit(void);
int BurnSoundExit(void);
#endif

// Fix for Metal build
#define BUILD_METAL 1

// Fix missing CPS identifiers if not included elsewhere
#ifndef CPS_VARIABLES_DEFINED
#define CPS_VARIABLES_DEFINED
#include "cps_input_full.h"
#endif
