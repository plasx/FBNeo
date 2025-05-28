#include "burnint.h"
#include "timer.h"
#include "metal_fixes.h"

// Timer stub implementation for Metal build
double dTime = 0.0;

// Use external declarations from metal_fixes.h

static double BurnTimerTimeCallbackDummy()
{
    return 0.0;
}

extern "C" double BurnTimerGetTime()
{
    if (pTimerTimeCallback) {
        return dTime;
    }
    return dTime;
}

INT32 BurnTimerUpdate(INT32 nCycles)
{
    return 0;
}

void BurnTimerEndFrame(INT32 nCycles)
{
    // No-op
}

void BurnTimerUpdateEnd()
{
    // No-op
}

void BurnOPLTimerCallback(INT32 n, INT32 c, double period)
{
    // No-op
}

void BurnOPMTimerCallback(INT32 n, INT32 c, double period)
{
    // No-op
}

void BurnOPNTimerCallback(INT32 n, INT32 c, INT32 cnt, double stepTime)
{
    // No-op
}

void BurnYMFTimerCallback(INT32 n, INT32 c, double period)
{
    // No-op
}

void BurnYMF262TimerCallback(INT32 n, INT32 c, double period)
{
    // No-op
}

void BurnTimerSetRetrig(INT32 c, double period)
{
    // No-op
}

void BurnTimerSetOneshot(INT32 c, double period)
{
    // No-op
}

void BurnTimerSetRetrig(INT32 c, UINT64 timer_ticks)
{
    // No-op
}

void BurnTimerSetOneshot(INT32 c, UINT64 timer_ticks)
{
    // No-op
}

void BurnTimerScan(INT32 nAction, INT32* pnMin)
{
    if (pnMin && *pnMin < 0x029521) {
        *pnMin = 0x029521;
    }
}

void BurnTimerExit()
{
    pCPURun = NULL;
    pTimerTimeCallback = NULL;
}

void BurnTimerReset()
{
    dTime = 0.0;
}

void BurnTimerPreInit()
{
    BurnTimerExit();
}

INT32 BurnTimerInit(INT32 nIndex, INT32 nChips, void* pOverCallback, void* pTimeCallback)
{
    return nIndex;
}

INT32 BurnTimerAttach(cpu_core_config *ptr, INT32 nClockspeed)
{
    return 0;
}

INT32 BurnTimerAttachNull(INT32 nClockspeed)
{
    return 0;
} 