#pragma once

// Metal-specific tchar.h replacement
// This provides Metal-compatible definitions without conflicts

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <limits.h>
#include <stdlib.h>

// Define MAX_PATH for our use
#ifndef MAX_PATH
#ifdef PATH_MAX
#define MAX_PATH PATH_MAX
#else
#define MAX_PATH 512
#endif
#endif

// Only define TCHAR if not already defined
#ifndef TCHAR_DEFINED
#define TCHAR_DEFINED

#ifdef _UNICODE
// Unicode support (not used on macOS)
typedef wchar_t TCHAR;
#define _T(x) L##x
#else
// Regular char-based version
typedef char TCHAR;
#define _T(x) x
#endif

#endif // TCHAR_DEFINED

// String handling functions for Metal
#define _stprintf snprintf
#define _tprintf printf
#define _tcscpy strcpy
#define _tcslen strlen
#define _tcscmp strcmp
#define _tcsncmp strncmp
#define _tfopen fopen
#define _vsntprintf vsnprintf
#define _tcstol strtol
#define _tcsstr strstr
#define _ttoi atoi
#define _stscanf sscanf

// Provide a safer snprintf wrapper for old sprintf code
static inline int metal_snprintf(char* buf, size_t size, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int result = vsnprintf(buf, size, fmt, args);
    va_end(args);
    return result;
} 