#ifndef _CPS_PAL_WRAPPER_H_
#define _CPS_PAL_WRAPPER_H_

// This wrapper prevents header conflicts by including the fixes header 
// before the original file

#define DONT_DECLARE_GLOBALS

// Include burn.h for type definitions
#include "burn.h"

// Include the fixes header with the correct path
#include "../../../burner/metal/fixes/cps2_fixes.h"

// Include the original file
#include "cps_pal.cpp"

#endif // _CPS_PAL_WRAPPER_H_ 