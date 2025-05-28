#ifndef _CPS_CONFIG_WRAPPER_H_
#define _CPS_CONFIG_WRAPPER_H_

// This wrapper prevents header conflicts by including the fixes header before the original file
#define DONT_DECLARE_GLOBALS

#include "burn.h"
#include "../../../burner/metal/fixes/cps2_fixes.h"

// Include the original file
#include "cps_config.cpp"

#endif // _CPS_CONFIG_WRAPPER_H_ 