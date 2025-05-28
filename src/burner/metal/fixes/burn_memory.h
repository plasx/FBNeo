#ifndef _BURN_MEMORY_H_
#define _BURN_MEMORY_H_

#include <stdlib.h>

// Memory allocation functions for Metal build
// These have names that won't conflict with the macros in other files
void* Metal_BurnMalloc(size_t size);
void Metal_BurnFree(void* ptr);

// Create macros only if they aren't already defined
#ifndef BurnMalloc
#define BurnMalloc(x) Metal_BurnMalloc(x)
#endif

#ifndef BurnFree
#define BurnFree(x) do { Metal_BurnFree(x); x = NULL; } while (0)
#endif

#endif // _BURN_MEMORY_H_
