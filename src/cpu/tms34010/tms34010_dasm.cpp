#include "tms34010.h"
#ifdef BUILD_METAL
#include <cstdio>
#include <string>
#include "memhandler.h"
// Stub implementation for Metal build
namespace tms {
    const char* dasm_single(UINT32 pc, UINT32* cycles)
    {
        static char buffer[256];
        snprintf(buffer, sizeof(buffer), "Disassembly not supported");
        if (cycles != NULL) *cycles = 0;
        return buffer;
    }
}
#endif
