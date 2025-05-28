#include "burnint.h"
#include "burn_led.h"

#define MAX_LED		8

static struct LedItem {
	int nLed;
	unsigned int nStatus;
	int nXPos, nYPos;
	int nWidth, nHeight;
} LedList[MAX_LED];

int nNumLed;
static int nLedSize = 2;

#if defined FBA_DEBUG
static int Debug_BurnLedInitted = 0;
#endif

// Use these externally defined screen dimensions
// Don't redefine them here
extern INT32 nScreenWidth, nScreenHeight;

// Forward declaration of BurnLedRenderSingle
void BurnLedRenderSingle(int nLed);

int BurnLedInit(int nLedCount, int nLedSizeXY)
{
#if defined FBA_DEBUG
	if (Debug_BurnLedInitted) {
		bprintf(PRINT_ERROR, _T("BurnLedInit called when Leds already inited\n"));
		return 1;
	}
#endif

	nNumLed = 0;
	nLedSize = nLedSizeXY;
	
	if (nLedCount > MAX_LED) nLedCount = MAX_LED;
	
	for (int i = 0; i < nLedCount; i++) {
		nNumLed = i + 1;

		LedList[i].nLed = i;
		LedList[i].nStatus = 0;
		LedList[i].nXPos = 0;
		LedList[i].nYPos = 0;
		LedList[i].nWidth = nScreenWidth / (150 / nLedSize);
		LedList[i].nHeight = nScreenHeight / (150 / nLedSize);
	}
	
#if defined FBA_DEBUG
	Debug_BurnLedInitted = 1;
#endif

	return 0;
}

int BurnLedSetPosition(int nLed, int nXPos, int nYPos)
{
#if defined FBA_DEBUG
	if (!Debug_BurnLedInitted) {
		bprintf(PRINT_ERROR, _T("BurnLedSetPosition called without init\n"));
		return 1;
	}
#endif

	if (nLed >= nNumLed) return 1;
	
	LedList[nLed].nXPos = (nXPos + 100) * nScreenWidth / 200 - (LedList[nLed].nWidth / 2);
	LedList[nLed].nYPos = (nYPos + 100) * nScreenHeight / 200 - (LedList[nLed].nHeight / 2);

	return 0;
}

int BurnLedSetSize(int nLed, int nWidth, int nHeight)
{
#if defined FBA_DEBUG
	if (!Debug_BurnLedInitted) {
		bprintf(PRINT_ERROR, _T("BurnLedSetSize called without init\n"));
		return 1;
	}
#endif

	if (nLed >= nNumLed) return 1;
	
	LedList[nLed].nWidth = nScreenWidth / (100 / nWidth);
	LedList[nLed].nHeight = nScreenHeight / (100 / nHeight);

	return 0;
}

void BurnLedSetStatus(int nLed, unsigned int nStatus)
{
#if defined FBA_DEBUG
	if (!Debug_BurnLedInitted) {
		bprintf(PRINT_ERROR, _T("BurnLedSetStatus called without init\n"));
		return;
	}
#endif

	if (nLed >= nNumLed) return;

	LedList[nLed].nStatus = nStatus;
}

void BurnLedRender(void)
{
#if defined FBA_DEBUG
	if (!Debug_BurnLedInitted) {
		bprintf(PRINT_ERROR, _T("BurnLedRender called without init\n"));
		return;
	}
#endif
	
	for (int i = 0; i < nNumLed; i++) {
		BurnLedRenderSingle(i);
	}
}

void BurnLedRenderSingle(int nLed)
{
#if defined FBA_DEBUG
	if (!Debug_BurnLedInitted) {
		bprintf(PRINT_ERROR, _T("BurnLedRenderSingle called without init\n"));
		return;
	}
#endif

	if (nLed >= nNumLed) return;
		
	unsigned int nColour = 0;
	
	if (LedList[nLed].nStatus) {
		nColour = 0xff00ff00;
	} else {
		nColour = 0xff000000;
	}
	
	for (int y = 0; y < LedList[nLed].nHeight; y++) {
		unsigned int* pPlot = (unsigned int*)pBurnDraw + (LedList[nLed].nYPos + y) * nBurnPitch / 4;
		pPlot += LedList[nLed].nXPos;
		for (int x = 0; x < LedList[nLed].nWidth; x++) {
			*pPlot++ = nColour;
		}
	}
}

void BurnLedExit(void)
{
#if defined FBA_DEBUG
	if (!Debug_BurnLedInitted) {
		bprintf(PRINT_ERROR, _T("BurnLedExit called without init\n"));
		return;
	}
#endif

#if defined FBA_DEBUG
	Debug_BurnLedInitted = 0;
#endif
}

void BurnLedScan(int nAction, int* pnMin)
{
	if (nAction & ACB_DRIVER_DATA) {
		SCAN_VAR(LedList);
		SCAN_VAR(nNumLed);
	}
}
