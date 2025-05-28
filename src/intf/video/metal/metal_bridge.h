#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Initialize the Metal renderer
int Metal_Init(void* windowHandle, int width, int height);

// Shutdown the Metal renderer
void Metal_Shutdown();

// Set the frame buffer pointer and dimensions
void Metal_SetFrameBuffer(unsigned char* buffer, int width, int height, int pitch);

// Update the frame buffer with new data
void Metal_UpdateFrame();

// Render the current frame
void Metal_RenderFrame();

// Resize the Metal viewport
void Metal_Resize(int width, int height);

// Toggle post-processing effects
void Metal_TogglePostProcessing(int enabled);

// Set scanline intensity (0.0 - 1.0)
void Metal_SetScanlineIntensity(float intensity);

// Set CRT curvature (0.0 - 1.0)
void Metal_SetCRTCurvature(float curvature);

// Set vignette effect (0.0 - 1.0)
void Metal_SetVignetteStrength(float strength);

// Set window title
void Metal_SetWindowTitle(const char* title);

#ifdef __cplusplus
}
#endif 