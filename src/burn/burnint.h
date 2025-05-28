#ifndef _BURNINT_H
#define _BURNINT_H

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stddef.h>
#include <stdint.h>

// Include main burn header first for data types
#include "burn.h"

// Screen dimensions - declared once here and externally referenced
extern INT32 nScreenWidth, nScreenHeight;

// Then platform-specific headers for cross-platform support
#include "tchar.h"
#include "endian.h"
#include "crossplatform.h"
#include "burn_memory.h"
#include "burn_debug.h"

// Metal/macOS specific fixes
#ifdef __APPLE__
#include "../burner/metal/fixes/burndriver_fixes.h"
#endif

// Metal build specific fixes
#ifdef USE_METAL_FIXES
#include "metal_fixes.h"
#endif

// Standard functions for dealing with ROM and input info structures
#include "stdfunc.h"

// ---------------------------------------------------------------------------
// BURN Sound defines

// Sound related macros
#define BURN_SND_CLIP(A) ((A) < -0x8000 ? -0x8000 : (A) > 0x7fff ? 0x7fff : (A))

#define BURN_SND_ROUTE_LEFT			1
#define BURN_SND_ROUTE_RIGHT		2
#define BURN_SND_ROUTE_BOTH			(BURN_SND_ROUTE_LEFT | BURN_SND_ROUTE_RIGHT)

// ---------------------------------------------------------------------------
// BURN steppers

#define STEP1 1
#define STEP2 2
#define STEP3 4
#define STEP4 8
#define STEP5 16
#define STEP6 32
#define STEP7 64
#define STEP8 128
#define STEP9 256
#define STEP10 512

// ---------------------------------------------------------------------------

// cpu_core_config structure
#define CPU_CORE_CONFIG_DEFINED
#ifndef CPU_CORE_CONFIG_STRUCT_DEFINED
#define CPU_CORE_CONFIG_STRUCT_DEFINED
struct cpu_core_config {
    INT32 nCpu;                       // The CPU's index
    const char* cpu_name;            // CPU name
    double (*BurnCpuGetTotalCycles)();
    UINT32 (*BurnCpuGetNextIRQLine)();
    void (*open)(INT32 nCPU);        // Open CPU for access
    void (*close)();                 // Close CPU access
    UINT32 (*read)(UINT32 address);  // Read from CPU memory
    void (*write)(UINT32 address, UINT32 data); // Write to CPU memory
    double (*totalcycles)();         // Get total CPU cycles
    void (*run)(INT32 nCycles);      // Run CPU for n cycles
    void (*runend)();                // End CPU run
    int nAddressFlags;               // Address flags
    int nMemorySize;                 // Memory size
    int (*active)();                 // Active CPU callback
};
#endif

// Structure for cheat core
struct cheat_core {
    struct cpu_core_config *cpuconfig;
    int nCPU;            
};

// Function prototypes
void CpuCheatRegister(int type, struct cpu_core_config *config);
struct cheat_core *GetCpuCheatRegister(int nCPU);

// Import global variables from burn.h - use the same types!
extern bool bBurnOkay;

// Include tiles_generic.h after we've declared nScreenWidth and nScreenHeight
#include "tiles_generic.h"

#endif
