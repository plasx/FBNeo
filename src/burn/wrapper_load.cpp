// Final Burn Neo - Metal build wrapper for load.cpp
// This file creates a wrapper around load.cpp for Metal builds

// Standard includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include our Metal-specific type definitions first
#include "burner/metal/burner_metal.h"

// Include burnint.h for necessary includes
#include "burnint.h"

// Forward declare structs
struct BurnDriver;

// Forward declarations for functions used from load.cpp
extern "C" {
    INT32 BurnLoadRom(UINT8* Dest, INT32 i, INT32 nGap);
    INT32 BurnLoadRomExt(UINT8* Dest, INT32 i, INT32 nGap, INT32 nType);
    INT32 BurnXorRom(UINT8* Dest, INT32 i, INT32 nGap);
    INT32 BurnXorBitmap(UINT8* Dest, INT32 i, INT32 nGap);
    
    // External global variables used by load.cpp
    // Use declarations from burnint.h
}

// Create modular metal wrappers for load.cpp
#ifdef METAL_BUILD

// Define pDriver for load.cpp to work with our mock array
#define pDriver MetalDriverArray
extern struct BurnDriver* MetalDriverArray[1];

// Wrap the file include to control what gets included
#ifndef LOAD_CPP_WRAPPED
#define LOAD_CPP_WRAPPED

// Include load.cpp 
#include "load.cpp"

#endif // LOAD_CPP_WRAPPED

// Metal-specific wrapper functions
extern "C" {
    INT32 BurnLoadRom_Metal(UINT8* Dest, INT32 i, INT32 nGap) {
        return BurnLoadRom(Dest, i, nGap);
    }
    
    INT32 BurnLoadRomExt_Metal(UINT8* Dest, INT32 i, INT32 nGap, INT32 nType) {
        return BurnLoadRomExt(Dest, i, nGap, nType);
    }
    
    INT32 BurnXorRom_Metal(UINT8* Dest, INT32 i, INT32 nGap) {
        return BurnXorRom(Dest, i, nGap);
    }
    
    INT32 BurnXorBitmap_Metal(UINT8* Dest, INT32 i, INT32 nGap) {
        return BurnXorBitmap(Dest, i, nGap);
    }
}

#endif // METAL_BUILD 