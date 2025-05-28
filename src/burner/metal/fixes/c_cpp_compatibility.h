#pragma once

/**
 * C/C++ Compatibility Header
 * This file provides common macros and definitions to help with 
 * compatibility between C and C++ code in the FBNeo Metal port.
 */

#ifdef __cplusplus
// C++ specific definitions

// For C code included from C++
#define EXTERN_C_BEGIN extern "C" {
#define EXTERN_C_END }

// For C++ code included from C
#define EXTERN_CPP_BEGIN
#define EXTERN_CPP_END

#else
// C specific definitions

// For C code included from C++
#define EXTERN_C_BEGIN
#define EXTERN_C_END

// For C++ code included from C
#define EXTERN_CPP_BEGIN extern
#define EXTERN_CPP_END

// C doesn't have bool, so define it
#ifndef __cplusplus
#ifndef bool
typedef unsigned char bool;
#define true 1
#define false 0
#endif
#endif

#endif

// Common defines for both C and C++
#ifndef NULL
#ifdef __cplusplus
#define NULL 0
#else
#define NULL ((void*)0)
#endif
#endif

// FBNeo basic types
typedef unsigned char UINT8;
typedef unsigned short UINT16;
typedef unsigned int UINT32;
typedef int INT32;

// Fix common struct references that need tags in C but not C++
#ifdef __cplusplus
#define STRUCT_TYPE(name) name
#define ENUM_TYPE(name) name
#else
#define STRUCT_TYPE(name) struct name
#define ENUM_TYPE(name) enum name
#endif

// More compatibility macros

// Using variables from other files
#define EXTERN_VAR extern

// Function attributes
#define INLINE_FUNCTION static inline

// Compiler-specific attributes
#ifdef _MSC_VER
#define CDECL __cdecl
#define STDCALL __stdcall
#define FASTCALL __fastcall
#define FORCE_INLINE __forceinline
#define DEPRECATED __declspec(deprecated)
#define NORETURN __declspec(noreturn)
#else
#define CDECL __attribute__((cdecl))
#define STDCALL __attribute__((stdcall))
#define FASTCALL __attribute__((fastcall))
#define FORCE_INLINE inline __attribute__((always_inline))
#define DEPRECATED __attribute__((deprecated))
#define NORETURN __attribute__((noreturn))
#endif

// Platform detection
#if defined(_WIN32) || defined(_WIN64)
#define PLATFORM_WINDOWS
#elif defined(__APPLE__) && defined(__MACH__)
#define PLATFORM_MACOS
#elif defined(__linux__) || defined(__gnu_linux__)
#define PLATFORM_LINUX
#else
#define PLATFORM_UNKNOWN
#endif

// Endianness detection
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define ENDIAN_BIG
#else
#define ENDIAN_LITTLE
#endif
