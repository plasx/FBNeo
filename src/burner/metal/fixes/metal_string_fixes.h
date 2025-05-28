#pragma once

// String handling replacements for Metal builds
// This avoids conflicts with macros and system headers

#include <stdio.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

// Safe replacement for unsafe sprintf
static inline int metal_sprintf(char* buffer, size_t buffer_size, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int result = vsnprintf(buffer, buffer_size, format, args);
    va_end(args);
    return result;
}

// Safe version of strncpy that ensures null termination
static inline char* metal_strncpy(char* dest, const char* src, size_t n) {
    if (n > 0) {
        strncpy(dest, src, n - 1);
        dest[n - 1] = '\0';
    }
    return dest;
}

#ifdef __cplusplus
}
#endif 