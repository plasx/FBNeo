#ifndef _METAL_TCHAR_H_
#define _METAL_TCHAR_H_

#include <limits.h>

// Make sure MAX_PATH is defined
#ifndef MAX_PATH
#ifdef PATH_MAX
#define MAX_PATH PATH_MAX
#else
#define MAX_PATH 512
#endif
#endif

// For Metal builds, we'll use char for TCHAR
#ifndef TCHAR
typedef char TCHAR;
#endif

// For _T macro
#ifndef _T
#define _T(x) x
#endif

// String handling macros for compatibility
#define _tcslen strlen
#define _tcscpy strcpy
#define _tcsncpy strncpy
#define _tprintf printf
#define _vstprintf vsprintf
#define _vsntprintf vsnprintf
#define _sntprintf snprintf
#define _stprintf sprintf
#define _tcstol strtol
#define _tcsicmp strcasecmp

#endif // _METAL_TCHAR_H_ 