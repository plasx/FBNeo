/*
 * This file provides C-linkage exports for symbols required by the linker
 * These are simple stub implementations with the proper symbol names
 * that match what the C++ code is looking for.
 */

#include "src/burner/metal/fixes/c_symbols_header.h"

// Explicitly mark all functions as having default visibility for the linker
// and use the standard C calling convention (no name mangling)
#define EXPORT __attribute__((visibility("default")))

// SEK functions
EXPORT INT32 SekInit(INT32 nCount, INT32 nCPUType) { return 0; }
EXPORT void SekExit(void) {}
EXPORT void SekNewFrame(void) {}
EXPORT INT32 SekReset(void) { return 0; }
EXPORT INT32 SekOpen(const INT32 i) { return 0; }
EXPORT INT32 SekClose(void) { return 0; }
EXPORT INT32 SekScan(INT32 nAction) { return 0; }

// CTV function
EXPORT INT32 CtvReady(void) { return 0; }
EXPORT INT32 PsndScan(INT32 nAction, INT32* pnMin) { return 0; } 