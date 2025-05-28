#ifndef _CPSR_WRAPPER_H_
#define _CPSR_WRAPPER_H_

// This wrapper prevents header conflicts by including our fixes before the original file

// Define this to avoid header conflicts with global variables
#define DONT_DECLARE_GLOBALS

// Include system headers first
#include "burn.h"

// Include our minimal direct linkage header
#include "../../../burner/metal/fixes/direct_minimal.h"

// Include the original file
#include "cpsr.cpp"

#endif // _CPSR_WRAPPER_H_ 