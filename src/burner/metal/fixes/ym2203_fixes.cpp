#include "burnint.h"

// BurnYM2203 functions
extern "C" {
    void BurnYM2203Update(short* pSoundBuf, int nSegmentEnd) { }
    void BurnYM2203Reset() { }
    void BurnYM2203Exit() { }
    int BurnYM2203Init(int num, int clock, float volume, int (*irqCallback)(int), int (*timerOverCallback)(int, int)) { return 0; }
    void BurnYM2203SetRoute(int nChip, int nIndex, double nVolume, int nRouteDir) { }
    int BurnYM2203Scan(int nAction, int* pnMin) { return 0; }
}
