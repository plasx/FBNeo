#ifndef WRAPPER_D_CPS2_CPP
#define WRAPPER_D_CPS2_CPP

// Define preprocessor variables to avoid header conflicts
#define eeprom_h
#define INCLUDE_EEPROM_H
#define m68000_intf_h
#define msm6295_h
#define samples_h
#define z80_intf_h
#define timer_h
#define burn_ym2151_h
#define burnint_h
#define tiles_generic_h

// Include burn.h from a fixed path to get BRF_ definitions
#include "../../../burn/burn.h"

// Define ROM types needed by driver
#define CPS2_PRG_68K         1
#define CPS2_PRG_68K_SIMM    2
#define CPS2_PRG_68K_XOR_TABLE 3
#define CPS2_PRG_Z80         4
#define CPS2_GFX             5
#define CPS2_QSND            6
#define CPS2_ENCRYPTION_KEY  7
#define CPS2_GFX_19XXJ       8  // Adding missing constant for 19XXJ graphics
#define CPS2_GFX_SIMM        9  // SIMM Graphics ROMs
#define CPS2_QSND_SIMM_BYTESWAP 10 // SIMM QSound ROMs with byte swapping
#define CPS2_GFX_SPLIT4      11 // Split Graphics ROMs (4)
#define CPS2_GFX_SPLIT6      12 // Split Graphics ROMs (6) 
#define CPS2_GFX_SPLIT8      13 // Split Graphics ROMs (8)
#define CPS2_QSND_SIMM       14 // SIMM QSound ROMs without byte swapping

// Define BRF flags if not already defined
#ifndef BRF_PRG
#define BRF_PRG             0x01
#endif
#ifndef BRF_GRA
#define BRF_GRA             0x02
#endif
#ifndef BRF_SND
#define BRF_SND             0x04
#endif
#ifndef BRF_ESS
#define BRF_ESS             0x08
#endif

// Define missing input variables for CPS2 games
// Include our fixes header
#include "../../../burner/metal/fixes/cps2_fixes.h"

// Now include the original file
#include "d_cps2.cpp"

#include "burnint.h"
// Don't include direct_linkage_force.cpp here as it causes conflicts
// #include "../../burner/metal/fixes/direct_linkage_force.cpp"

#endif // WRAPPER_D_CPS2_CPP 