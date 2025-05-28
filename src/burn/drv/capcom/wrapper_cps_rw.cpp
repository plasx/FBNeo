#ifndef _CPS_RW_WRAPPER_H_
#define _CPS_RW_WRAPPER_H_

// This wrapper prevents header conflicts by including the fixes header
// before including the original file

// Define a preprocessor variable to avoid header conflicts
#define DONT_DECLARE_GLOBALS

// Include burn.h for type definitions
#include "burn.h"

// Include the minimal direct linkage header
#include "../../../burner/metal/fixes/direct_minimal.h"

// Include the original file
#include "cps_rw.cpp"

#endif
