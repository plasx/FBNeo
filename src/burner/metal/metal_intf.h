#pragma once

// Metal interface functions - these are defined in Objective-C++ code

#include "../../burn/driver.h" // Include for INT16/INT32 definitions
#include "../burner.h"
#include "../gameinp.h"
#include "metal_audio.h"
#include "metal_input.h"
#include "metal_renderer.h"
#include "burnint.h"

#ifdef __cplusplus
extern "C" {
#endif

// Define the audio callback function type
typedef INT32 (*AudSoundCallbackFunction)(INT32);

// ===== Metal Renderer Interface =====

// Initialize the Metal rendering system
// view: A pointer to the NSView to attach the Metal renderer to
// Returns: true on success, false on failure
bool InitMetal(void* view);

// Shut down the Metal rendering system
void ShutdownMetal();

// Render a frame of pixel data to the Metal view
// buffer: Pointer to the pixel data buffer
// width: Width of the frame in pixels
// height: Height of the frame in pixels
// pitch: Number of bytes per scanline
void MetalRenderFrame(unsigned char* buffer, int width, int height, int pitch);

// Main Metal game loop - usually handled by main_metal.mm
// Returns 0 on success, non-zero on error or when exiting
int RunMetalGame();

// Set the title of the Metal window
void MetalSetWindowTitle(const char* title);

// Resize the Metal window/view
void MetalResizeWindow(int width, int height);

// Manually burn and render a frame - for testing
void BurnAndRenderFrame();

// ===== Metal Input Interface =====

// Initialize the Metal input system
int MetalInputInit();

// Process keyboard and controller input
void MetalHandleInput(bool bCopy);

// Process key and mouse events
void MetalProcessKeyDown(unsigned short keyCode);
void MetalProcessKeyUp(unsigned short keyCode);
void MetalProcessMouseDown(int button, int x, int y);
void MetalProcessMouseUp(int button, int x, int y);
void MetalProcessMouseMove(int x, int y);

// ===== Metal Audio Interface =====

// Initialize the Metal audio system
int MetalAudioInit();

// Shut down the Metal audio system
void MetalAudioExit();

// Callback for audio samples from FBNeo
void MetalAudioCallback(INT16 *data, INT32 samples);

// Set audio callback - not used in Metal implementation
int MetalSetAudioCallback(AudSoundCallbackFunction callback);

// Callback function that will be called when a frame needs to be rendered
// Implemented in main_metal.mm
void MetalRenderCallback(unsigned char* buffer, int width, int height, int pitch);

// Fix functions from rom_fixes.cpp
void FixRomPaths();
const char* GetGameToLoad();

// AI integration functions
int MetalAIInit();
void MetalAIExit();
void MetalAIProcessFrame();
void MetalAILogFrame(void* inputData, void* outputData, int frameNumber);

#ifdef __cplusplus
}
#endif 