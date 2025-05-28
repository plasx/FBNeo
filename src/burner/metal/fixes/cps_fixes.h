#pragma once

// CPS fixes for Metal build
#include "burnint.h"

namespace MetalFixes {

// Provide stub variables needed by the CPS driver
extern UINT32* CpsPal;  // External palette data

// Add additional CPS-related stubs here as needed

} // namespace MetalFixes 

// Define missing CPS2 volume and aspect variables
extern UINT8 Cps2VolUp;
extern UINT8 Cps2VolDwn;
extern UINT8 AspectDIP; 