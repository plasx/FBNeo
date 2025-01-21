// src/intf/video/vid_interface_metal.cpp
// Minimal bridging for a "Metal" video mode in FBNeo.

#include "burnint.h" // for bprintf() if needed
#include "vid_interface_metal.h"

extern "C" int FBNeo_InitVideoMetal(int width, int height);
extern "C" void FBNeo_ShutdownVideoMetal();
extern "C" void FBNeo_DrawFrameMetal(const void* frameBuffer, int width, int height);

static bool g_metalInited = false;

int FBNeo_InitializeVideoMetal(int width, int height)
{
#ifdef __APPLE__
    int ret = FBNeo_InitVideoMetal(width, height);
    if (ret == 0) {
        bprintf(0, _T("Using Metal backend\n"));
        g_metalInited = true;
        return 0;
    } else {
        bprintf(0, _T("Failed to init Metal backend\n"));
        return -1;
    }
#else
    return -1;
#endif
}

void FBNeo_ShutdownVideoMetalInterface()
{
#ifdef __APPLE__
    if (g_metalInited) {
        FBNeo_ShutdownVideoMetal();
        g_metalInited = false;
    }
#endif
}

void FBNeo_DrawFrameMetalInterface(const void* frameBuffer, int width, int height)
{
#ifdef __APPLE__
    if (g_metalInited) {
        FBNeo_DrawFrameMetal(frameBuffer, width, height);
    }
#endif
}