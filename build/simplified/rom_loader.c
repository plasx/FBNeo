#include <stdio.h>
#include <stdbool.h>

bool Metal_LoadAndInitROM(const char* path) {
    printf("Loading ROM: %s\n", path);
    return true;
}

void Metal_UnloadROM(void) {
    printf("Unloading ROM\n");
}
