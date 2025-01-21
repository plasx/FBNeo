// driverlist_metal.h
// A minimal hand-crafted driver list for Metal standalone builds,
// referencing a few example drivers (CPS1, CPS2, NeoGeo).

#pragma once

// Declare external references to the drivers you want to build/link
extern struct BurnDriver BurnDrvCps1;      // from CPS1
extern struct BurnDriver BurnDrvCps2;      // from CPS2
extern struct BurnDriver BurnDrvNeoGeo;    // from Neo Geo

// You can add more as you add .cpp to your build

// Provide a driver list array
struct BurnDriver *pDriver[] = {
    &BurnDrvCps1,
    &BurnDrvCps2,
    &BurnDrvNeoGeo,
    // Add more if you want
    NULL
};

// The count of drivers in this list
int nBurnDrvCount = 3;