#include "metal_common.h"
#include "burn.h"
#include "load.h"

// Metal-specific ROM loading implementation
INT32 MetalBurnLoadRom(UINT8* Dest, INT32* pnWrote, INT32 i) {
    INT32 ret = BurnLoadRom(Dest, i, 1);
    if (pnWrote) *pnWrote = 0; // or actual bytes written if available
    return ret;
}

INT32 MetalBurnLoadRomExt(UINT8* Dest, INT32 i, INT32 nGap, INT32 nType) {
    return BurnLoadRomExt(Dest, i, nGap, nType);
}

// Metal-specific ROM loading initialization
INT32 MetalLoadInit() {
    // Set up Metal-specific ROM loading callbacks
    BurnExtLoadRom = MetalBurnLoadRom;
    return 0;
}

void MetalLoadExit() {
    // Clean up Metal-specific ROM loading
    BurnExtLoadRom = NULL;
} 