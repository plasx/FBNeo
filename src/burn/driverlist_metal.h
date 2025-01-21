// driverlist_metal.h
// Minimal, handcrafted list for a Metal-only build.

#pragma once

#include "burn.h"  // For BurnDriver struct & UINT32 definition

// Forward declare the drivers you want to include:
extern struct BurnDriver BurnDrvCps1;
extern struct BurnDriver BurnDrvCps2;
extern struct BurnDriver BurnDrvNeoGeo;

// Define pDriver[] and nBurnDrvCount in C++ linkage
#ifdef __cplusplus
extern "C" {
#endif

// The emulator expects these names/variables:
struct BurnDriver* pDriver[] = {
    &BurnDrvCps1,
    &BurnDrvCps2,
    &BurnDrvNeoGeo,
    NULL
};

UINT32 nBurnDrvCount = 3; // Must match the type in burn.h

#ifdef __cplusplus
}
#endif