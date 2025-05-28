#ifndef _CPS_INPUT_H_
#define _CPS_INPUT_H_

// CPS input definitions for Metal builds

#ifdef __cplusplus
extern "C" {
#endif

// CPS input arrays
extern UINT8 CpsInp000[0x10];
extern UINT8 CpsInp001[0x10];
extern UINT8 CpsInp011[0x10];
extern UINT8 CpsInp020[0x10];
extern UINT8 CpsInp021[0x10];
extern UINT8 CpsInp029[0x10];
extern UINT8 CpsInp176[0x10];

// CPS2 flags and system variables
extern INT32 Cps2Turbo;
extern INT32 nCpsZ80Cycles;
extern INT32 nCpsCycles;
extern UINT8* CpsZRamC0;
extern UINT8* CpsZRamF0;

#ifdef __cplusplus
}
#endif

#endif // _CPS_INPUT_H_ 