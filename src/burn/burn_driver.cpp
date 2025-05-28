#include "burnint.h"
#include "metal_fixes.h"
#include <stdio.h>

// Driver function implementations for Metal build

// Additional driver support functions
int BurnDrvGetDriverInfo(int* info) {
    if (!info) {
        return 1;
    }
    
    // Default CPS2 dimensions
    *info = 0;
    return 0;
} 