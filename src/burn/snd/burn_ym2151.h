// burn_ym2151.h
#include "driver.h"
extern "C" {
#include "ym2151.h"
}
#include "timer.h" // Added for BurnTimer functions

// Ensure all exported functions have explicit extern "C" linkage
#ifdef __cplusplus
extern "C" {
#endif

void BurnYM2151SetMultiChip(INT32 bYes); // set before init for 2 ym2151's!

// C API declarations
INT32 BurnYM2151Init(INT32 nClock, INT32 bAddSignalToStream);
void BurnYM2151SetRoute(INT32 chip, INT32 nIndex, double nVolume, INT32 nRouteDir);
void BurnYM2151SetAllRoutes(INT32 chip, double vol, INT32 route);
void BurnYM2151Reset();
void BurnYM2151Exit();
void BurnYM2151Render(INT16* pSoundBuf, INT32 nSegmentEnd);
void BurnYM2151Scan(INT32 nAction, INT32 *pnMin);
void BurnYM2151SetIrqHandler(INT32 chip, void (*irqHandler)(INT32));
void BurnYM2151SetPortHandler(INT32 chip, write8_handler portHandler); 
UINT8 BurnYM2151Read(INT32 chip);

// Expose the no-argument version with a different name to avoid conflicts
UINT8 BurnYM2151ReadNoArg();

void BurnYM2151InitBuffered(INT32 nClockFrequency, INT32 use_timer, INT32 (*StreamCallback)(INT32), INT32 bAdd);
void BurnYM2151SelectRegister(UINT8 nRegister);
void BurnYM2151WriteRegister(UINT8 nValue);
void BurnYM2151Write(INT32 chip, INT32 offset, const UINT8 nData);

#ifdef __cplusplus
}
#endif

// Define some constants
#define BURN_SND_YM2151_YM2151_ROUTE_1		0
#define BURN_SND_YM2151_YM2151_ROUTE_2		1

