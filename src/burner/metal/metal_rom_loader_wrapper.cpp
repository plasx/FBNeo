#include <cstdio>
#include <cstdlib>

// Include our headers
#include "metal_declarations.h"
#include "metal_rom_loader.h"
#include "metal_zip_extract.h"
#include "rom_loading_debug.h"

// These function prototypes must exactly match the ones used in the obj/metal_rom_loader.o file
extern "C" {
    // Functions that might be called externally but not declared in headers
    void Metal_SetCurrentROM(const char* romName);
    const char* Metal_GetCurrentROM();
    int Metal_IsROMLoaded();
    int Metal_IsGameRunning();
    void Metal_SetGameRunning(int running);
}

// We don't need to implement the functions as they exist in the .o file
// We just need to ensure the proper linkage between them 