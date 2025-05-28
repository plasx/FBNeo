#ifndef _METAL_INTEROP_H_
#define _METAL_INTEROP_H_

// =============================================================================
// FBNeo Metal - C/C++ Interoperability Layer
// =============================================================================
// This header defines the standard approach for interfacing between C, C++,
// and Objective-C++ code in the FBNeo Metal port to ensure proper linkage.
// =============================================================================

// Include standard types 
#include <stdint.h>

// -----------------------------------------------------------------------------
// C/C++ Linkage Macros
// -----------------------------------------------------------------------------
// These macros standardize and simplify C/C++ interop declarations

// For headers included from both C and C++ files
#ifdef __cplusplus
  #define METAL_BEGIN_C_DECL extern "C" {
  #define METAL_END_C_DECL }
#else
  #define METAL_BEGIN_C_DECL
  #define METAL_END_C_DECL
#endif

// For functions exported from C++ to C
#ifdef __cplusplus
  #define METAL_EXPORT_TO_C extern "C"
#else
  #define METAL_EXPORT_TO_C
#endif

// For functions imported from C to C++
#ifdef __cplusplus
  #define METAL_IMPORT_FROM_C extern "C"
#else
  #define METAL_IMPORT_FROM_C
#endif

// -----------------------------------------------------------------------------
// Common Types
// -----------------------------------------------------------------------------
// Basic types used across the C/C++ boundary

#ifndef TYPES_DEFINED
#define TYPES_DEFINED
typedef int32_t INT32;
typedef uint32_t UINT32;
typedef int16_t INT16;
typedef uint16_t UINT16;
typedef int8_t INT8;
typedef uint8_t UINT8;
#endif

// For C code compatibility
#ifndef __cplusplus
  typedef int bool;
  #define true 1
  #define false 0
#endif

// -----------------------------------------------------------------------------
// Fix for __fastcall on ARM64/macOS
// -----------------------------------------------------------------------------
// On ARM64, the default calling convention is already efficient
// and fastcall is unnecessary and unsupported
#ifndef FASTCALL 
 #undef __fastcall
 #define __fastcall
#endif

// -----------------------------------------------------------------------------
// Debug/Diagnostic Settings
// -----------------------------------------------------------------------------
// Disable warnings for issues we can't fix
#pragma GCC diagnostic ignored "-Wmissing-braces"
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#endif // _METAL_INTEROP_H_ 