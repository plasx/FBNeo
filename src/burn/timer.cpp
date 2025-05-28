// Timers (for Yamaha FM chips and generic)
#include "burnint.h"
#include "timer.h"
#include "metal_fixes.h"

#define MAX_TIMER_VALUE ((1 << 30) - 65536)

double dTime;									// Time elapsed since the emulated machine was started

static INT32 nIndex;

#define TIMER_MAX 8
static INT32 nTimerCount[TIMER_MAX], nTimerStart[TIMER_MAX];

static INT32 nTimerChips[TIMER_MAX];

// Timer function pointer types
typedef INT32 (*BurnTimerTotalCycles)();
typedef void (*BurnTimerRun)(INT32, INT32);
typedef void (*BurnTimerOverCallback)(INT32, INT32);
typedef void (*BurnTimerTimeCallback)(INT32, INT32);

// Global timer function pointers
BurnTimerTotalCycles BurnTimerCPUTotalCycles = NULL;
BurnTimerRun BurnTimerCPURun = NULL;
BurnTimerOverCallback BurnTimerCPUOver = NULL;
BurnTimerTimeCallback BurnTimerCPUTime = NULL;

INT32 BurnTimerCPUClockspeed = 0;

static INT32 nTicksTotal, nTicksDone, nTicksExtra;

// Timer chip data
static BurnTimerOverCallback pTimerOverCallback[TIMER_MAX] = { NULL };

// Dummy functions
INT32 dummy_total_cycles()
{
    return 0;
}

void dummy_newframe(INT32 cycles, INT32 cycles_per_frame)
{
    // Do nothing
}

void dummy_idle()
{
    // Do nothing
}

double dummy_time()
{
    return 0.0;
}

// Dummy configuration
struct metal_cpu_core_config dummy_config = {
    dummy_total_cycles,
    dummy_newframe,
    dummy_idle,
    dummy_time
};

// ---------------------------------------------------------------------------
// Running time

static double BurnTimerTimeCallbackDummy()
{
	return 0.0;
}

extern "C" double BurnTimerGetTime()
{
	return dTime + pTimerTimeCallback();
}

// ---------------------------------------------------------------------------
// Update timers

INT32 BurnTimerUpdate(INT32 nCycles)
{
	INT32 nIRQStatus = 0;

	nTicksTotal = MAKE_TIMER_TICKS(nCycles, BurnTimerCPUClockspeed);

//	bprintf(PRINT_NORMAL, _T(" -- Ticks: %08X, cycles %i\n"), nTicksTotal, nCycles);

	while (nTicksDone < nTicksTotal) {
		INT32 nTimer, nFirstTimer, nCyclesSegment, nTicksSegment;

		// Determine which timer fires first
		nFirstTimer = 0;
		for (INT32 i = 0; i < TIMER_MAX; i++) {
			if (nTimerCount[i] < nTimerCount[nFirstTimer]) {
				nFirstTimer = i;
			}
		}

		nTicksSegment = nTimerCount[nFirstTimer];

		if (nTicksSegment > nTicksTotal) {
			nTicksSegment = nTicksTotal;
		}

		nCyclesSegment = MAKE_CPU_CYLES(nTicksSegment + nTicksExtra, BurnTimerCPUClockspeed);
//		bprintf(PRINT_NORMAL, _T("  - Timer: %08X, %08X, %08X, cycles %i, %i\n"), nTicksDone, nTicksSegment, nTicksTotal, nCyclesSegment, BurnTimerCPUTotalCycles());

		pCPURun(nCyclesSegment - BurnTimerCPUTotalCycles());

		nTicksDone = MAKE_TIMER_TICKS(BurnTimerCPUTotalCycles() + 1, BurnTimerCPUClockspeed) - 1;
//		bprintf(PRINT_NORMAL, _T("  - ticks done -> %08X cycles -> %i\n"), nTicksDone, BurnTimerCPUTotalCycles());

		nTimer = 0;

		for (INT32 i = 0; i < TIMER_MAX; i++) {
			if (nTicksDone >= nTimerCount[i]) {
				if (nTimerStart[i] == MAX_TIMER_VALUE) {
					nTimerCount[i] = MAX_TIMER_VALUE;
				} else {
					nTimerCount[i] += nTimerStart[i];
				}
				//bprintf(PRINT_NORMAL, _T("  - timer %d fired\n"), i);

//				bprintf(0, _T("over_callback[%d](%d, %d)\n"), i>>1,nTimerChips[i>>1], i&1);
				nIRQStatus |= pTimerOverCallback[i>>1](nTimerChips[i>>1], i&1);
			}
		}
	}

	return nIRQStatus;
}

void BurnTimerEndFrame(INT32 nCycles)
{
	INT32 nTicks = MAKE_TIMER_TICKS(nCycles, BurnTimerCPUClockspeed);

	BurnTimerUpdate(nCycles);

	for (INT32 i = 0; i < TIMER_MAX; i++) {
		if (nTimerCount[i] < MAX_TIMER_VALUE) {
			nTimerCount[i] -= nTicks;
		}
	}

	nTicksDone -= nTicks;
	if (nTicksDone < 0) {
//		bprintf(PRINT_ERROR, _T(" -- ticks done -> %08X\n"), nTicksDone);
		nTicksDone = 0;
	}
}

void BurnTimerUpdateEnd()
{
//	bprintf(PRINT_NORMAL, _T("  - end %i\n"), BurnTimerCPUTotalCycles());

	pCPURun();

	nTicksTotal = 0;
}


// ---------------------------------------------------------------------------
// Callbacks for the sound cores
/*
static INT32 BurnTimerExtraCallbackDummy()
{
	return 0;
}
*/
void BurnOPLTimerCallback(INT32 n, INT32 c, double period)
{
	pCPURun();

	if (period == 0.0) {
		nTimerCount[(n << 1) + c] = MAX_TIMER_VALUE;
//		bprintf(PRINT_NORMAL, _T("  - timer %i stopped\n"), c);
		return;
	}

	nTimerCount[(n << 1) + c]  = (INT32)(period * (double)TIMER_TICKS_PER_SECOND);
	nTimerCount[(n << 1) + c] += MAKE_TIMER_TICKS(BurnTimerCPUTotalCycles(), BurnTimerCPUClockspeed);

//	bprintf(PRINT_NORMAL, _T("  - timer %i started, %08X ticks (fires in %lf seconds)\n"), c, nTimerCount[c], period);
}

void BurnOPMTimerCallback(INT32 n, INT32 c, double period) // ym2151
{
	pCPURun();
	
	if (period == 0.0) {
		nTimerCount[(n << 1) + c] = MAX_TIMER_VALUE;
		return;
	}

	nTimerCount[(n << 1) + c]  = (INT32)(period * (double)TIMER_TICKS_PER_SECOND);
	nTimerCount[(n << 1) + c] += MAKE_TIMER_TICKS(BurnTimerCPUTotalCycles(), BurnTimerCPUClockspeed);
}

void BurnOPNTimerCallback(INT32 n, INT32 c, INT32 cnt, double stepTime) // ym2203, ym2610, ym2612, ym2608
{
	pCPURun();
	
	if (cnt == 0) {
		nTimerCount[(n << 1) + c] = MAX_TIMER_VALUE;

//		bprintf(PRINT_NORMAL, _T("  - timer %i stopped\n"), c);

		return;
	}

	nTimerCount[(n << 1) + c]  = (INT32)(stepTime * cnt * (double)TIMER_TICKS_PER_SECOND);
	nTimerCount[(n << 1) + c] += MAKE_TIMER_TICKS(BurnTimerCPUTotalCycles(), BurnTimerCPUClockspeed);
	//bprintf(PRINT_NORMAL, _T("  - chip %i timer %i started, %08X ticks (fires in %lf seconds)\n"), n, c, nTimerCount[(n << 1) + c], stepTime * cnt);
}

void BurnYMFTimerCallback(INT32 /* n */, INT32 c, double period)
{
	pCPURun();

	if (period == 0.0) {
		nTimerStart[c] = nTimerCount[c] = MAX_TIMER_VALUE;

//		bprintf(PRINT_NORMAL, _T("  - timer %i stopped\n"), c);

		return;
	}

	nTimerStart[c]  = nTimerCount[c] = (INT32)(period * (double)(TIMER_TICKS_PER_SECOND / 1000000));
	nTimerCount[c] += MAKE_TIMER_TICKS(BurnTimerCPUTotalCycles(), BurnTimerCPUClockspeed);

//	bprintf(PRINT_NORMAL, _T("  - timer %i started, %08X ticks (fires in %lf seconds)\n"), c, nTimerCount[c], period);
}

void BurnYMF262TimerCallback(INT32 /* n */, INT32 c, double period)
{
	pCPURun();

	if (period == 0.0) {
		nTimerCount[c] = MAX_TIMER_VALUE;
		return;
	}

	nTimerCount[c]  = (INT32)(period * (double)TIMER_TICKS_PER_SECOND);
	nTimerCount[c] += MAKE_TIMER_TICKS(BurnTimerCPUTotalCycles(), BurnTimerCPUClockspeed);

//	bprintf(PRINT_NORMAL, _T("  - timer %i started, %08X ticks (fires in %lf seconds)\n"), c, nTimerCount[c], period);
}

void BurnTimerSetRetrig(INT32 c, double period)
{
	pCPURun();

	if (period == 0.0) {
		nTimerStart[c] = nTimerCount[c] = MAX_TIMER_VALUE;

//		bprintf(PRINT_NORMAL, _T("  - timer %i stopped\n"), c);

		return;
	}

	nTimerStart[c]  = nTimerCount[c] = (INT32)(period * (double)(TIMER_TICKS_PER_SECOND));
	nTimerCount[c] += MAKE_TIMER_TICKS(BurnTimerCPUTotalCycles(), BurnTimerCPUClockspeed);

//	bprintf(PRINT_NORMAL, _T("  - timer %i started, %08X ticks (fires in %lf seconds)\n"), c, nTimerCount[c], period);
}

void BurnTimerSetOneshot(INT32 c, double period)
{
	pCPURun();

	if (period == 0.0) {
		nTimerStart[c] = nTimerCount[c] = MAX_TIMER_VALUE;

//		bprintf(PRINT_NORMAL, _T("  - timer %i stopped\n"), c);

		return;
	}

	nTimerCount[c]  = (INT32)(period * (double)(TIMER_TICKS_PER_SECOND));
	nTimerCount[c] += MAKE_TIMER_TICKS(BurnTimerCPUTotalCycles(), BurnTimerCPUClockspeed);

//	bprintf(PRINT_NORMAL, _T("  - timer %i started, %08X ticks (fires in %lf seconds)\n"), c, nTimerCount[c], period / 1000000.0);
}

void BurnTimerSetRetrig(INT32 c, UINT64 timer_ticks)
{
	pCPURun();

	if (timer_ticks == 0) {
		nTimerStart[c] = nTimerCount[c] = MAX_TIMER_VALUE;

		// bprintf(PRINT_NORMAL, L"  - timer %i stopped\n", c);

		return;
	}

	nTimerStart[c] = nTimerCount[c] = (UINT32)(timer_ticks);
	nTimerCount[c] += MAKE_TIMER_TICKS(BurnTimerCPUTotalCycles(), BurnTimerCPUClockspeed);

	// bprintf(PRINT_NORMAL, L"  - timer %i started, %08X ticks (fires in %lf seconds)\n", c, nTimerCount[c], (double)(TIMER_TICKS_PER_SECOND) / timer_ticks);
}

void BurnTimerSetOneshot(INT32 c, UINT64 timer_ticks)
{
	pCPURun();

	if (timer_ticks == 0) {
		nTimerStart[c] = nTimerCount[c] = MAX_TIMER_VALUE;

		// bprintf(PRINT_NORMAL, L"  - timer %i stopped\n", c);

		return;
	}

	nTimerCount[c] = (UINT32)(timer_ticks);
	nTimerCount[c] += MAKE_TIMER_TICKS(BurnTimerCPUTotalCycles(), BurnTimerCPUClockspeed);

	// bprintf(PRINT_NORMAL, L"  - timer %i started, %08X ticks (fires in %lf seconds)\n", c, nTimerCount[c], (double)(TIMER_TICKS_PER_SECOND) / timer_ticks);
}

// ------------------------------------ ---------------------------------------
// Initialisation etc.

void BurnTimerScan(INT32 nAction, INT32* pnMin)
{
	if (pnMin && *pnMin < 0x029521) {
		*pnMin = 0x029521;
	}

	if (nAction & ACB_DRIVER_DATA) {
		SCAN_VAR(nTimerCount);
		SCAN_VAR(nTimerStart);
		SCAN_VAR(dTime);

		SCAN_VAR(nTicksDone);
	}
}

void BurnTimerExit()
{
	BurnTimerCPUClockspeed = 0;
	BurnTimerCPUTotalCycles = NULL;
	pCPURun = NULL;

	pTimerTimeCallback = NULL;

	return;
}

void BurnTimerReset()
{
	for (INT32 i = 0; i < TIMER_MAX; i++) {
		nTimerCount[i] = nTimerStart[i] = MAX_TIMER_VALUE;
	}

	dTime = 0.0;

	nTicksDone = 0;
}

// inits in BurnDrvInit()
void BurnTimerPreInit()
{
	BurnTimerExit();

	nIndex = 0;
}

// initted by soundcore[s], (or anything)
// Each chip gets 2 timers

// BurnTimerSetOneshot / BurnTimerSetRetrig, use (baseIndex << 1) + timer#
// for the first param.  baseIndex is the return value from BurnTimerInit()
// timer# can be 0, 1

INT32 BurnTimerInit(INT32 nIndex, INT32 nChips, BurnTimerOverCallback pOverCallback, BurnTimerTimeCallback pTimeCallback)
{
	INT32 nChipBaseIndex = nIndex;
	bprintf(0, _T("BurnTimerInit: base index %d, #chips %d\n"), nIndex, nChips);

	if (nIndex + (nChips * 2) >= TIMER_MAX) {
		bprintf(PRINT_ERROR, _T("BurnTimer: Init overflows, increase TIMER_MAX?\n"));
		return 0;
	}

	for (INT32 chipnum = 0; chipnum < nChips; chipnum++) {
		pTimerOverCallback[nIndex] = pOverCallback;
		nTimerChips[nIndex] = chipnum; // base chip#
		//bprintf(0, _T("nTimerChips[%d] = %d\n"), nIndex, chipnum);
		nIndex++;
	}

	pTimerTimeCallback = pTimeCallback ? pTimeCallback : BurnTimerTimeCallbackDummy;

	BurnTimerReset();

	return nChipBaseIndex;
}

// Null CPU, for a FM timer that isn't attached to anything.
static INT32 NullCyclesTotal;

void NullNewFrame()
{
	NullCyclesTotal = 0;
}

INT32 NullTotalCycles()
{
	return NullCyclesTotal;
}

INT32 NullRun(const INT32 nCycles)
{
	NullCyclesTotal += nCycles;

	return nCycles;
}

void NullRunEnd()
{
}

INT32 BurnTimerAttach(cpu_core_config *ptr, INT32 nClockspeed)
{
	BurnTimerCPUClockspeed = nClockspeed;
	BurnTimerCPUTotalCycles = (BurnTimerTotalCycles)ptr->totalcycles;
	pCPURun = (BurnTimerRun)ptr->run;

	nTicksExtra = MAKE_TIMER_TICKS(1, BurnTimerCPUClockspeed) - 1;

	return 0;
}

INT32 BurnTimerAttachNull(INT32 nClockspeed)
{
	BurnTimerCPUClockspeed = nClockspeed;
	BurnTimerCPUTotalCycles = NullTotalCycles;
	pCPURun = NullRun;

	nTicksExtra = MAKE_TIMER_TICKS(1, BurnTimerCPUClockspeed) - 1;

//	bprintf(PRINT_NORMAL, _T("--- timer cpu speed %iHz, one cycle = %i ticks.\n"), nClockspeed, MAKE_TIMER_TICKS(1, BurnTimerCPUClockspeed));

	return 0;
}
