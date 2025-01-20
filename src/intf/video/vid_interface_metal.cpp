// vid_interface_metal.cpp
// Minimal bridging code for Metal video in FBNeo.

#include "burnint.h"    // For bprintf if needed
#include "vid_interface_metal.h"

extern "C" int FBNeo_InitVideoMetal(int width, int height);
extern "C" void FBNeo_ShutdownVideoMetal();
extern "C" void FBNeo_DrawFrameMetal(const void* frameBuffer, int width, int height);

// We'll define a global or static variable to note if Metal is initialized
static bool g_metalInited = false;

// Initialize the Metal backend
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
	return -1; // Not on Apple, skip
#endif
}

// Shutdown the Metal backend
void FBNeo_ShutdownVideoMetalInterface()
{
#ifdef __APPLE__
	if (g_metalInited) {
		FBNeo_ShutdownVideoMetal();
		g_metalInited = false;
	}
#endif
}

// Draw a frame with the Metal backend
void FBNeo_DrawFrameMetalInterface(const void* frameBuffer, int width, int height)
{
#ifdef __APPLE__
	if (g_metalInited) {
		FBNeo_DrawFrameMetal(frameBuffer, width, height);
	}
#endif
}