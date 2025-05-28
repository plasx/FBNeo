#pragma once

// Metal-specific fixes for joyprocess.h

#include "burnint.h"

namespace MetalFixes {

// Variants of joyprocess functions that avoid type conflicts
UINT8 ProcessAnalogMetal(INT16 anaval, INT32 reversed, INT32 flags, UINT8 scalemin, UINT8 scalemax, UINT8 centerval);

// Add other joyprocess-related functions if needed

} // namespace MetalFixes

// Export fixed versions with C linkage for Metal
extern "C" {
    UINT8 METAL_ProcessAnalog(INT16 anaval, INT32 reversed, INT32 flags, UINT8 scalemin, UINT8 scalemax, UINT8 centerval);
} 