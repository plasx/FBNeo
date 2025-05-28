#ifndef _QS_C_WRAPPER_H_
#define _QS_C_WRAPPER_H_

// This wrapper prevents header conflicts by including the fixes header 
// before the original file

#define DONT_DECLARE_GLOBALS

// Define NO_CTV_FUNCTIONS to avoid conflicts with the CTV function declarations in cps.h
#define NO_CTV_FUNCTIONS

// Include headers for type definitions
#include "burn.h"
#include "../../../burner/metal/fixes/fix_sound_routing.h"
#include "../../../burner/metal/fixes/cps2_fixes.h"

// Redefine the CtvDoFn type to match what's in cps.h
typedef INT32 (*CtvDoFn)();

// Define dummy arrays with the same names
// These won't actually be used but will prevent redefinition errors
CtvDoFn _dummy_CtvDoX[0x20];
CtvDoFn _dummy_CtvDoXM[0x20];
CtvDoFn _dummy_CtvDoXB[0x20];

// Define macros to redirect the array references to our dummy arrays
#define CtvDoX _dummy_CtvDoX
#define CtvDoXM _dummy_CtvDoXM
#define CtvDoXB _dummy_CtvDoXB

// Include the original file
#include "qs_c.cpp"

// Undefine the macros to avoid affecting other includes
#undef CtvDoX
#undef CtvDoXM
#undef CtvDoXB

#endif // _QS_C_WRAPPER_H_ 