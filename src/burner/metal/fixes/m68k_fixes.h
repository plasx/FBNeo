#ifndef _M68K_FIXES_H_
#define _M68K_FIXES_H_

// Include our fastcall and EXT_* macros fix
#include "c_cpp_fixes.h"

// Addressing the function overloading issue with SekGetPC
// The SekGetPC function in m68000_intf.h has a potential function overloading conflict
// where return types differ. This is a fix for that.

// If the function had been previously defined with UINT32 return type
#ifdef UINT32_SekGetPC
    #undef UINT32_SekGetPC
#endif

// In our metal codebase, ensure we only have one SekGetPC definition that returns UINT32
// This is included before m68000_intf.h to ensure proper definition
UINT32 SekGetPC(INT32 n);

#endif // _M68K_FIXES_H_ 