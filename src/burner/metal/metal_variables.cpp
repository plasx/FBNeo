#include "metal_declarations.h"

// Metal integration variables for frame buffer access
UINT8* pBurnDraw_Metal = NULL;
INT32 nBurnPitch_Metal = 0;
INT32 nBurnBpp_Metal = 32;

// BurnDrv Info structure
struct BurnDrvMeta BurnDrvInfo = {0}; 