#include <stdio.h>
#include <stdbool.h>

// Minimal set of functions to avoid duplication with metal_bridge.mm
int FBNeoInit(void) {
    printf("FBNeoInit: Initializing emulator core\n");
    return 0;
}

int FBNeoExit(void) {
    printf("FBNeoExit: Shutting down emulator core\n");
    return 0;
}

int RunFrame(int bDraw) {
    return 0;
}

bool bDoIpsPatch = false;
