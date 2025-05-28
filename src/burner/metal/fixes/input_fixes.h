#ifndef _INPUT_FIXES_H_
#define _INPUT_FIXES_H_

#include "burnint.h"

#ifdef __cplusplus
extern "C" {
#endif

// Input-related function declarations
void BurnGunExit();
void FreezeInput(unsigned char **buf, int *size);
void SekNewFrame();

#ifdef __cplusplus
}
#endif

#endif // _INPUT_FIXES_H_ 