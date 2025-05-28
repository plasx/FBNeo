#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// C++ stubs for BurnDrv functions and others that may need extern "C" linkage

extern "C" {
    // BurnDrv functions that need C++ linkage
    const char* BurnDrvGetTextA(unsigned int index) {
        static char buffer[256] = "Marvel vs. Capcom";
        
        // Return different strings based on index
        switch (index) {
            case 0: return "Marvel vs. Capcom"; // Full name
            case 1: return "mvsc";             // Short name
            case 2: return "CPS2";             // System
            default: return buffer;
        }
    }
    
    int BurnDrvGetVisibleSize(int* width, int* height) {
        if (width) *width = 384;
        if (height) *height = 224;
        return 0;
    }
    
    bool ROM_Verify(const char* path) {
        printf("[ROM] Verifying ROM: %s\n", path);
        return true;
    }
    
    int LoadROM_FullPath(const char* path) {
        printf("[ROM] Loading ROM from path: %s\n", path);
        return 0;
    }
} 