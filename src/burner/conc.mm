#include "burner.h"

#ifdef BUILD_METAL
#include <Foundation/Foundation.h>

// Stub version of ConfigCheatLoad for Metal build
INT32 ConfigCheatLoad() {
    // Not implemented for Metal build
    return 0;
}

#else
// Original code would go here for other platforms
#endif
