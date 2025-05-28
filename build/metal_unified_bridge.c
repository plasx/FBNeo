#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// Memory tracking functions
void* BurnMalloc(size_t size) {
    return malloc(size);
}

void _BurnFree(void* ptr) {
    free(ptr);
}

// Essential bridge functions needed by Metal renderer
int FBNeoInit() {
    printf("FBNeoInit: Initializing emulator core\n");
    return 0;
}

int FBNeoExit() {
    printf("FBNeoExit: Shutting down emulator core\n");
    return 0;
}

int RunFrame(int bDraw) {
    return 0;
}

// ROM loading functions
bool LoadROM_FullPath(const char* szPath) {
    printf("LoadROM_FullPath: Loading ROM from %s\n", szPath);
    return true;
}

bool Metal_LoadAndInitROM(const char* path) {
    printf("Metal_LoadAndInitROM: %s\n", path);
    return true;
}

void Metal_UnloadROM() {
    printf("Metal_UnloadROM called\n");
}

// BurnDrv related functions
int BurnDrvExit() {
    printf("BurnDrvExit called\n");
    return 0;
}

const char* BurnDrvGetTextA(unsigned int iIndex) {
    static char gameName[] = "Metal CPS2 Game";
    return gameName;
}

int BurnDrvGetVisibleSize(int* pnWidth, int* pnHeight) {
    if (pnWidth) *pnWidth = 384;  // Standard CPS2 resolution
    if (pnHeight) *pnHeight = 224;
    return 0;
}

// Input related functions
void Metal_ProcessKeyDown(int keyCode) {
    printf("Metal_ProcessKeyDown: %d\n", keyCode);
}

void Metal_ProcessKeyUp(int keyCode) {
    printf("Metal_ProcessKeyUp: %d\n", keyCode);
}

void Metal_UpdateInputState() {
    // Nothing to do
}

// Required global variable
bool bDoIpsPatch = false;
