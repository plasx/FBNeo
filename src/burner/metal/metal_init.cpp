#include "metal_common.h"
#include "burn.h"
#include "burn_memory.h"
#include "burn_sound.h"

// Forward declarations
INT32 MetalLoadInit();
void MetalLoadExit();
INT32 MetalInputInit();
void MetalInputExit();

// Metal-specific initialization
INT32 MetalBurnInit() {
    // Initialize memory manager
    BurnInitMemoryManager();
    
    // Initialize Metal-specific subsystems
    if (MetalLoadInit() != 0) {
        return 1;
    }
    
    if (MetalInputInit() != 0) {
        return 1;
    }
    
    // Initialize sound system
    BurnSoundInit();
    
    // Initialize the active driver
    if (pDriver[nBurnDrvActive]->Init) {
        return pDriver[nBurnDrvActive]->Init();
    }
    
    return 0;
}

void MetalBurnExit() {
    // Exit the active driver
    if (pDriver[nBurnDrvActive]->Exit) {
        pDriver[nBurnDrvActive]->Exit();
    }
    
    // Exit Metal-specific subsystems
    MetalLoadExit();
    MetalInputExit();
    
    // Exit memory manager
    BurnExitMemoryManager();
}

// Metal-specific input
INT32 MetalBurnInputSetKey(INT32 i, INT32 nState) {
    return BurnDrvSetInput(i, nState);
}

// Metal-specific ROM loading
// INT32 MetalBurnLoadRom(UINT8* Dest, INT32* pnWrote, INT32 i) {
//     // Implementation removed - use the one in metal_load.cpp
// } 