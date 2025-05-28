#include "burnint.h"
#include "z80_intf.h"

// Forward declaration for cpu_core_config structure
struct cpu_core_config;

// This file provides wrapper functions for QSound-related functionality
// to avoid issues with conflicting declarations

// Special wrapper for BurnTimerAttach that works with both void* and cpu_core_config* parameters
extern "C" {
    extern int (*BurnTimerAttachRedirect)(void* pCC, int nClockspeed);
    
    int BurnTimerAttach(struct cpu_core_config* cpucore, int nClockspeed) {
        // Convert the cpu_core_config pointer to void* and call our function
        return BurnTimerAttachRedirect((void*)cpucore, nClockspeed);
    }
} 