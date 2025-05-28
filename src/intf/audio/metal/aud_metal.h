#ifndef _AUD_METAL_H_
#define _AUD_METAL_H_

#include "burner.h"

extern int MetalAudioInit();
extern int MetalAudioExit();
extern int MetalAudioPlay();
extern int MetalAudioStop();
extern int MetalAudioGetSettings(InterfaceInfo* pInfo);
extern struct AudOut AudOutMetal;

#endif // _AUD_METAL_H_ 