#ifndef _CPS_RUN_WRAPPER_H_
#define _CPS_RUN_WRAPPER_H_

// This wrapper prevents header conflicts by including the fixes header before the original file

// Define this to avoid header conflicts
#define DONT_DECLARE_GLOBALS

#include "burn.h"
#include "../../../burner/metal/fixes/cps2_fixes.h"
#include "../../../burner/metal/fixes/fix_sound_routing.h"

// Include the original file
#include "cps_run.cpp"

#endif // _CPS_RUN_WRAPPER_H_ 