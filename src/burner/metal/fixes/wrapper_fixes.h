#pragma once

#ifndef WRAPPER_FIXES_H
#define WRAPPER_FIXES_H

#include "../../../burn/burnint.h"

#ifdef __cplusplus
extern "C" {
#endif

// Functions declarations with compatible types for wrapper files
// These are used to avoid type conflicts with the original declarations

// CPS2 functions
extern UINT8* CpsFindGfxRam(INT32 nOffset, INT32 nLen);
extern void GetPalette(INT32 nStart, INT32 nCount);

#ifdef __cplusplus
}
#endif

#endif // WRAPPER_FIXES_H 