#ifndef _BURN_MEMORY_H
#define _BURN_MEMORY_H

#include <stdlib.h>

// First include burn.h for data types
#include "burn.h"

#ifdef __cplusplus
extern "C" {
#endif

// Memory allocation functions
UINT8 *_BurnMalloc(INT32 size, const char *file, INT32 line);
void _BurnFree(void *ptr);

#define BurnMalloc(x) _BurnMalloc(x, __FILE__, __LINE__)
#define BurnFree(x) do {_BurnFree(x); x = NULL; } while (0)

#ifdef __cplusplus
}
#endif

#endif // _BURN_MEMORY_H 