#pragma once

#include "metal_declarations.h" // Include for FrameBuffer definition

// Function declarations for Metal renderer bridge

#ifdef __cplusplus
extern "C" {
#endif

// Initialize the renderer bridge
int Bridge_Init(int width, int height);

// Update frame buffer dimensions if needed
int Bridge_UpdateDimensions(int width, int height);

// Pre-frame processing - call before running a frame
int Bridge_PreFrame(bool drawFrame);

// Post-frame processing - call after running a frame
int Bridge_PostFrame();

// Shutdown the renderer bridge
void Bridge_Shutdown();

#ifdef __cplusplus
}
#endif

// Global frame buffer - already defined in metal_declarations.h
// extern FrameBuffer g_frameBuffer; 