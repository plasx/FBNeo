#ifndef _CPS_OBJ_WRAPPER_H_
#define _CPS_OBJ_WRAPPER_H_

// This wrapper prevents header conflicts by including the fixes header
// before including the original file

// Define a preprocessor variable to avoid header conflicts
#define DONT_DECLARE_GLOBALS

// Include burn.h for type definitions
#include "burn.h"

// Include the fixes header with the correct relative path
#include "../../../burner/metal/fixes/wrapper_fixes.h"

// Include the original file
#include "cps_obj.cpp"

#endif // _CPS_OBJ_WRAPPER_H_ 