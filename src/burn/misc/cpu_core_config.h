// Stub header for FBNeo Metal build
// This file is provided as a minimal implementation to satisfy dependencies
// The full struct definition is now maintained in src/burn/burnint.h

#ifndef _CPU_CORE_CONFIG_H
#define _CPU_CORE_CONFIG_H

// Check if burnint.h is already included (it contains the full definition)
#ifndef _BURNINT_H

#ifdef __APPLE__
// For Apple Silicon/Metal builds, provide a minimal stub
struct cpu_core_config {
    int (*run)(int cycles);
    int (*totalcycles)();
    void (*runend)();
    // Other fields can be added if needed by Metal-specific code
    // This minimal struct satisfies the BurnTimerAttach requirements
};
#else
// For other platforms, we explicitly include the full definition
#include "../burnint.h"
#endif

#endif // _BURNINT_H

#endif // _CPU_CORE_CONFIG_H 