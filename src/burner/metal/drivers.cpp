#include "burnint.h"
#include "cps.h"
#include "driverlist.h"

// Only include Marvel vs Capcom
typedef struct BurnDriver BurnDriver;
extern BurnDriver BurnDrvCpsMvsc;
extern BurnDriver* pDriver[];
extern UINT32 nBurnDrvCount;
extern UINT32 nBurnDrvActive;

// All other BurnDrv* and BurnLib* functions have been removed to avoid duplicate/conflicting symbols.
