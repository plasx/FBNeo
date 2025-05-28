#ifndef MACOS_COMPAT_H
#define MACOS_COMPAT_H

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

// macOS compatibility functions
#ifndef _stprintf
static inline int _stprintf(char *str, const char *format, ...) {
    va_list args;
    va_start(args, format);
    int result = vsnprintf(str, MAX_PATH, format, args);
    va_end(args);
    return result;
}
#endif

#ifndef _tfopen
static inline FILE* _tfopen(const char *filename, const char *mode) {
    return fopen(filename, mode);
}
#endif

#endif // MACOS_COMPAT_H 