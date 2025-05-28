#include "cps.h"
#ifdef USE_METAL_FIXES
#include "metal_fixes.h"
#endif
// QSound

extern INT32 nCpsZ80Cycles;

static INT32 nQsndCyclesExtra;

static INT32 qsndTimerOver(INT32, INT32)
{
//	bprintf(PRINT_NORMAL, _T("  - IRQ -> 1.\n"));
	ZetSetIRQLine(0xFF, CPU_IRQSTATUS_HOLD);

	return 0;
}

INT32 QsndInit()
{
	INT32 nRate;

	// Init QSound z80
	if (QsndZInit()) {
		return 1;
	}
	BurnTimerInit(qsndTimerOver, NULL);

	if (Cps1Qs == 1) {
		nCpsZ80Cycles = 8000000 * 100 / nBurnFPS;
		BurnTimerAttachZet(8000000);
	} else {
		if (Cps2Turbo) {
			nCpsZ80Cycles = 16000000 * 100 / nBurnFPS;
			BurnTimerAttachZet(16000000);
		} else {
			nCpsZ80Cycles = 8000000 * 100 / nBurnFPS;
			BurnTimerAttachZet(8000000);
		}
	}

	if (nBurnSoundRate >= 0) {
		nRate = nBurnSoundRate;
	} else {
		nRate = 11025;
	}

	QscInit(nRate);		// Init QSound chip

	return 0;
}

void QsndReset()
{
	ZetOpen(0);
	BurnTimerReset();
	BurnTimerSetRetrig(0, 1.0 / 252.0);
	ZetClose();

	nQsndCyclesExtra = 0;
}

void QsndExit()
{
	QscExit();							// Exit QSound chip
	QsndZExit();
}


void QsndEndFrame()
{
	BurnTimerEndFrame(nCpsZ80Cycles);
	if (pBurnSoundOut) QscUpdate(nBurnSoundLen);
	if (Cps2Turbo && pBurnSoundOut) {
		BurnSampleRender(pBurnSoundOut, nBurnSoundLen);
	}

	nQsndCyclesExtra = ZetTotalCycles() - nCpsZ80Cycles;
	ZetClose();
}
