#include "burnint.h"
#include "../../burn/snd/msm6295.h"

// Simple MSM6295 wrapper functions to resolve build issues
void MSM6295SetVolume(int chip, double vol) {}
void MSM6295Exit(int chip) {}
void MSM6295Reset(int chip) {}
int MSM6295Init(int chip, int sample_rate, float volume, int bAdd) { return 0; }
void MSM6295Command(int chip, int command) {}
INT32 MSM6295Render(INT32 nChip, INT16* pSoundBuf, INT32 nSegmentLength) { return 0; }
int MSM6295Scan(int chip, int nAction) { return 0; } 