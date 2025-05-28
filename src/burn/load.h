#ifndef _LOAD_H_
#define _LOAD_H_

#include "burnint.h"

// ROM loading functions
INT32 BurnLoadRom(UINT8* Dest, INT32 i, INT32 nGap);
INT32 BurnLoadRomExt(UINT8* Dest, INT32 i, INT32 nGap, INT32 nFlags);

// ROM loading flags
#define LD_NORMAL 0
#define LD_BYTESWAP 1
#define LD_XOR 2

#endif // _LOAD_H_ 