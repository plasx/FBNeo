// tchar.h - Header for TCHAR string handling compatibility for metal platform
#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <string>
#include <ctype.h>

// Exclude Foundation for C++ files
#ifdef __OBJC__
#include <Foundation/Foundation.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Base types
typedef char TCHAR;
typedef char _TCHAR;

// String literals
#ifndef _T
#define _T(s)      s
#endif

#ifndef _TEXT
#define _TEXT(s)   s
#endif

// Safe string functions
inline int safe_sprintf(char* buffer, size_t bufferSize, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int result = vsnprintf(buffer, bufferSize, format, args);
    va_end(args);
    return result;
}

// String handling functions
#ifndef _tprintf
#define _tprintf   printf
#endif

#ifndef _ftprintf
#define _ftprintf  fprintf
#endif

// Only define _stprintf if it's not already defined
#ifndef _stprintf
#define _stprintf(buffer, format, ...)  safe_sprintf(buffer, 256, format, ##__VA_ARGS__)
#endif

#ifndef _sntprintf
#define _sntprintf snprintf
#endif

#ifndef _vsntprintf
#define _vsntprintf vsnprintf
#endif

#ifndef _tcscpy
#define _tcscpy    strcpy
#endif

#ifndef _tcsncpy
#define _tcsncpy   strncpy
#endif

#ifndef _tcscat
#define _tcscat    strcat
#endif

#ifndef _tcsncat
#define _tcsncat   strncat
#endif

#ifndef _tcslen
#define _tcslen    strlen
#endif

#ifndef _tcscmp
#define _tcscmp    strcmp
#endif

#ifndef _tcsicmp
#define _tcsicmp   strcasecmp
#endif

#ifndef _tcsnicmp
#define _tcsnicmp  strncasecmp
#endif

#ifndef _tcstol
#define _tcstol    strtol
#endif

#ifndef _itot
#define _itot      itoa
#endif

#ifndef _ttoi
#define _ttoi      atoi
#endif

#ifndef _tfopen
#define _tfopen    fopen
#endif

#ifndef _fgetts
#define _fgetts    fgets
#endif

#ifndef _istspace
#define _istspace  isspace
#endif

#ifndef _stscanf
#define _stscanf   sscanf
#endif

#ifndef _tcschr
#define _tcschr    strchr
#endif

#ifndef _tcsrchr
#define _tcsrchr   strrchr
#endif

// TCHAR versions of ctype functions
#ifndef _istdigit
#define _istdigit  isdigit
#endif

#ifndef _istascii
#define _istascii  isascii
#endif

#ifndef _istprint
#define _istprint  isprint
#endif

#ifndef _totupper
#define _totupper  toupper
#endif

#ifndef _totlower
#define _totlower  tolower
#endif

// Debug helper
#ifdef DEBUG
inline void custom_dprintf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
}
#else
inline void custom_dprintf(const char* /*format*/, ...) {}
#endif

#ifdef __cplusplus
}
#endif 