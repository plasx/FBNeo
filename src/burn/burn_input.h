#ifndef BURN_INPUT_H
#define BURN_INPUT_H

#include "burnint.h"

// Input system functions
INT32 BurnInputInit();
INT32 BurnInputExit();
INT32 BurnInputReset();
INT32 BurnInputSetKey(INT32 i, INT32 nState);
INT32 BurnInputGetKey(INT32 i);
INT32 BurnInputUpdate();
INT32 BurnInputHasChanged(INT32 nInput);

// Maximum inputs
#define INPUT_MAX 256

#endif // BURN_INPUT_H 