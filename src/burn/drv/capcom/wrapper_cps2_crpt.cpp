#ifndef _CPS2_CRPT_WRAPPER_H_
#define _CPS2_CRPT_WRAPPER_H_

// This wrapper prevents header conflicts by including the fixes header 
// before the original file

#define DONT_DECLARE_GLOBALS

// Include burn.h for type definitions
#include "burn.h"

// Include the fixes header with the correct path
#include "../../../burner/metal/fixes/cps2_fixes.h"

// Additional declarations needed by cps2_crpt.cpp
// extern UINT8 *CpsKey;

// Include the original file
#include "cps2_crpt.cpp"

#endif // _CPS2_CRPT_WRAPPER_H_ 