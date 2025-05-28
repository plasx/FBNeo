#include "../../../cpu/z80/z80.h"

// Implementation of z80TotalCycles - to be called from z80TotalCycles in metal_stubs.cpp
extern "C" INT32 Z80TotalCyclesImplementation()
{
    // Z80 struct contains a member for tracking cycles
    extern Z80_Regs Z80;
    
    // Return total cycles executed by the Z80 CPU
    // This is the proper implementation that accesses the internal Z80 state
    return z80_ICount;  // z80_ICount is declared extern in z80.h
} 