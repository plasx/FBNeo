#ifndef _CPS_WRAPPER_H_
#define _CPS_WRAPPER_H_

// This wrapper prevents header conflicts by including our fixes before the original file

// Define this to avoid header conflicts with global variables
#define DONT_DECLARE_GLOBALS

// Define NO_CTV_FUNCTIONS to avoid conflicts with the CTV function declarations in cps.h
#define NO_CTV_FUNCTIONS 

// Include system headers first
#include "burn.h"

// Define CPS2 ROM types to ensure consistency
#ifndef CPS2_PRG_68K
#define CPS2_PRG_68K              1
#define CPS2_PRG_68K_SIMM         2
#define CPS2_PRG_68K_XOR_TABLE    3
#define CPS2_GFX                  5
#define CPS2_GFX_SIMM             6
#define CPS2_GFX_SPLIT4           7
#define CPS2_GFX_SPLIT8           8
#define CPS2_GFX_19XXJ            9
#define CPS2_PRG_Z80             10
#define CPS2_QSND                11
#define CPS2_QSND_SIMM           12
#define CPS2_QSND_SIMM_BYTESWAP  13
#define CPS2_ENCRYPTION_KEY      14
#endif

// Include our minimal direct linkage header
#include "../../../burner/metal/fixes/direct_minimal.h"

// Redefine the CtvDoFn type to match what's in cps.h
typedef INT32 (*CtvDoFn)();

// Declare our own stub for the CTV arrays but don't actually use them
// The real implementations will be linked from our fixes
#ifdef __cplusplus
extern "C" {
#endif
// Stub declarations to satisfy the compiler
extern void CtvDoX(INT32 nField);
extern void CtvDoXB(INT32 nField);
extern void CtvDoXM(INT32 nField);
extern INT32 PsndSyncZ80(INT32 nCycles);
#ifdef __cplusplus
}
#endif

// Now here's the trick: we'll provide dummy arrays with the same names
// These won't actually be used but will prevent redefinition errors
CtvDoFn _dummy_CtvDoX[0x20];
CtvDoFn _dummy_CtvDoXM[0x20];
CtvDoFn _dummy_CtvDoXB[0x20];

// Define macros to redirect the array references to our dummy arrays
#define CtvDoX _dummy_CtvDoX
#define CtvDoXM _dummy_CtvDoXM
#define CtvDoXB _dummy_CtvDoXB

// Include the original file
#include "cps.cpp"

// Undefine the macros to avoid affecting other includes
#undef CtvDoX
#undef CtvDoXM
#undef CtvDoXB

#endif // _CPS_WRAPPER_H_ 