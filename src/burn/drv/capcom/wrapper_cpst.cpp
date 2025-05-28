#ifndef _CPST_WRAPPER_H_
#define _CPST_WRAPPER_H_

// This wrapper prevents header conflicts by including the fixes header 
// before the original file

#define DONT_DECLARE_GLOBALS

// Define NO_CTV_FUNCTIONS to avoid conflicts with the CTV function declarations in cps.h
#define NO_CTV_FUNCTIONS

// Include burn.h for type definitions
#include "burn.h"

// Include the fixes header with the correct path
#include "../../../burner/metal/fixes/cps2_fixes.h"
#include "../../../burner/metal/fixes/fix_sound_routing.h"

// Forward declare the CTV variables with extern "C"
#ifdef __cplusplus
extern "C" {
#endif
extern UINT32 _nCtvRollX;
extern UINT32 _nCtvRollY;
extern int _nCtvTileAdd;
extern UINT8* _pCtvLine;
extern unsigned char* _pCtvTile;
#ifdef __cplusplus
}
#endif

// Map original variable names to our fix variables
#define nCtvRollX _nCtvRollX
#define nCtvRollY _nCtvRollY
#define nCtvTileAdd _nCtvTileAdd
#define pCtvLine _pCtvLine
#define pCtvTile _pCtvTile

// Redefine the CtvDoFn type to match what's in cps.h
typedef INT32 (*CtvDoFn)();

// We need to handle CtvDoX etc. differently since cpst.cpp actually uses these
// as function arrays, not just declarations

// First declare our CTV functions
#ifdef __cplusplus
extern "C" {
#endif
// Function declarations
extern INT32 CtvDoX_Function();
extern INT32 CtvDoXB_Function();
extern INT32 CtvDoXM_Function();
extern INT32 PsndSyncZ80(INT32 nCycles);
#ifdef __cplusplus
}
#endif

// Create our own implementation of the CTV functions that will be called from cpst.cpp
INT32 CtvDoX_Function() {
    // This is just a stub that returns success
    return 0;
}

INT32 CtvDoXB_Function() {
    // This is just a stub that returns success
    return 0;
}

INT32 CtvDoXM_Function() {
    // This is just a stub that returns success
    return 0;
}

// Now create arrays of function pointers
CtvDoFn CtvDoX_Array[0x20];
CtvDoFn CtvDoXB_Array[0x20];
CtvDoFn CtvDoXM_Array[0x20];

// Initialize them all to point to our stub functions
static void InitCtvFunctionArrays() {
    for (int i = 0; i < 0x20; i++) {
        CtvDoX_Array[i] = CtvDoX_Function;
        CtvDoXB_Array[i] = CtvDoXB_Function;
        CtvDoXM_Array[i] = CtvDoXM_Function;
    }
}

// Call the initialization function
static int dummy = (InitCtvFunctionArrays(), 0);

// Define macros to redirect the array references to our arrays
#define CtvDoX CtvDoX_Array
#define CtvDoXM CtvDoXM_Array
#define CtvDoXB CtvDoXB_Array

// Include the original file
#include "cpst.cpp"

// Undefine the macros to avoid affecting other includes
#undef CtvDoX
#undef CtvDoXM
#undef CtvDoXB
#undef nCtvRollX
#undef nCtvRollY
#undef nCtvTileAdd
#undef pCtvLine
#undef pCtvTile

#endif // _CPST_WRAPPER_H_ 