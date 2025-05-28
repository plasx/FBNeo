#pragma once
// conc.h - Concurrency utilities for Metal FBNeo
// This file provides threading and concurrency support for the Metal renderer

#ifdef __cplusplus
extern "C" {
#endif

// Initialize concurrency support
int ConcurrencyInit();

// Clean up concurrency resources
void ConcurrencyExit();

// Signal that a new frame is ready
void SignalFrameReady();

// Wait for a frame to be ready, with timeout in milliseconds
// Returns true if a frame became ready, false on timeout
bool WaitForFrameReady(int timeoutMs);

// Check if threads should exit
bool ShouldThreadsExit();

// Start the render thread
int StartRenderThread();

// Start the audio thread
int StartAudioThread();

// Stop the render thread
void StopRenderThread();

// Stop the audio thread
void StopAudioThread();

// Check if render thread is running
bool IsRenderThreadRunning();

// Check if audio thread is running
bool IsAudioThreadRunning();

#ifdef __cplusplus
}
#endif 