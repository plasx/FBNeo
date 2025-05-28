// Define these first to prevent issues with multiple includes
#define metal_fixes_h
#define m68000_intf_h
#define z80_intf_h
#define burnint_h
#define burn_h
#define eeprom_h
#define timer_h
#define samples_h
#define msm6295_h
#define fix_direct_h
#define metal_eeprom_interface_h

#include "burnint.h"
#include "cps.h"
#include "metal_direct_impl.h"

// Implementation of the Metal direct interface for CPS
// Using C-like implementation to avoid language linkage issues

#ifdef BUILD_METAL

// Metal Direct namespace implementation
namespace MetalDirect {

    INT32 Init() {
        // Initialization code for Metal Direct implementation
        return 0;
    }

    void Exit() {
        // Cleanup code
    }

    INT32 Frame() {
        // Frame processing
        return 0;
    }

    void SetGfx(UINT8 *gfx) {
        // Set the graphics data
        CpsGfx = gfx;
    }

    void SetPalette(UINT8 *pal) {
        // Set palette data
    }

    void SetRom(UINT8 *rom) {
        // Set ROM data
        CpsRom = rom;
    }

    void SetZRom(UINT8 *zrom) {
        // Set Z80 ROM
        CpsZRom = zrom;
    }

    INT32 Scan(INT32 nAction) {
        // State save/load
        return 0;
    }
}

#endif // BUILD_METAL
