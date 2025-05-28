#include "burn_memory.h"
#include <stdlib.h>
#include <stdio.h>

// Memory allocation tracking for the emulator
void* Metal_BurnMalloc(size_t size)
{
    void* ptr = malloc(size);
    if (!ptr) {
        printf("Metal_BurnMalloc failed to allocate %zu bytes\n", size);
        return NULL;
    }
    return ptr;
}

void Metal_BurnFree(void* ptr)
{
    if (ptr) {
        free(ptr);
    }
}
