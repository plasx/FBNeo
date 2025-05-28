#include <stdlib.h>

void* BurnMalloc(size_t size) {
    return malloc(size);
}

void _BurnFree(void* ptr) {
    free(ptr);
}
