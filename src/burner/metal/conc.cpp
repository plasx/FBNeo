// conc.cpp - Concurrency utilities for Metal FBNeo
// This file provides threading and concurrency support for the Metal renderer

#include "burnint.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

// Threading state
static pthread_t g_renderThread;
static pthread_t g_audioThread;
static pthread_mutex_t g_frameMutex;
static pthread_cond_t g_frameReadyCond;
static bool g_renderThreadRunning = false;
static bool g_audioThreadRunning = false;
static bool g_frameReady = false;
static bool g_threadsShouldExit = false;

// Initialize concurrency support
int ConcurrencyInit() {
    printf("Initializing concurrency support...\n");
    
    // Initialize mutex and condition variable
    pthread_mutex_init(&g_frameMutex, NULL);
    pthread_cond_init(&g_frameReadyCond, NULL);
    
    g_threadsShouldExit = false;
    g_frameReady = false;
    
    printf("Concurrency initialized successfully\n");
    return 0;
}

// Clean up concurrency resources
void ConcurrencyExit() {
    printf("Shutting down concurrency support...\n");
    
    // Signal threads to exit
    g_threadsShouldExit = true;
    
    // Wake up any waiting threads
    pthread_mutex_lock(&g_frameMutex);
    g_frameReady = true;
    pthread_cond_broadcast(&g_frameReadyCond);
    pthread_mutex_unlock(&g_frameMutex);
    
    // Wait for threads to terminate
    if (g_renderThreadRunning) {
        pthread_join(g_renderThread, NULL);
        g_renderThreadRunning = false;
    }
    
    if (g_audioThreadRunning) {
        pthread_join(g_audioThread, NULL);
        g_audioThreadRunning = false;
    }
    
    // Clean up synchronization primitives
    pthread_mutex_destroy(&g_frameMutex);
    pthread_cond_destroy(&g_frameReadyCond);
    
    printf("Concurrency shut down successfully\n");
}

// Signal that a new frame is ready
void SignalFrameReady() {
    pthread_mutex_lock(&g_frameMutex);
    g_frameReady = true;
    pthread_cond_signal(&g_frameReadyCond);
    pthread_mutex_unlock(&g_frameMutex);
}

// Wait for a frame to be ready
bool WaitForFrameReady(int timeoutMs) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_nsec += (timeoutMs % 1000) * 1000000;
    ts.tv_sec += timeoutMs / 1000;
    
    // Handle nanosecond overflow
    if (ts.tv_nsec >= 1000000000) {
        ts.tv_nsec -= 1000000000;
        ts.tv_sec++;
    }
    
    pthread_mutex_lock(&g_frameMutex);
    
    // If frame is already ready, consume it and return immediately
    if (g_frameReady) {
        g_frameReady = false;
        pthread_mutex_unlock(&g_frameMutex);
        return true;
    }
    
    // Otherwise wait for a frame to become ready
    int result = pthread_cond_timedwait(&g_frameReadyCond, &g_frameMutex, &ts);
    
    bool frameWasReady = g_frameReady;
    g_frameReady = false;
    
    pthread_mutex_unlock(&g_frameMutex);
    
    return (result == 0) && frameWasReady;
}

// Check if threads should exit
bool ShouldThreadsExit() {
    return g_threadsShouldExit;
}

// Render thread function
static void* RenderThreadFunc(void* arg) {
    printf("Render thread started\n");
    
    while (!g_threadsShouldExit) {
        // Wait for a frame to be ready
        if (WaitForFrameReady(16)) {
            // Call the Metal renderer
            extern void MetalRenderer_Draw();
            MetalRenderer_Draw();
        }
    }
    
    printf("Render thread exiting\n");
    return NULL;
}

// Audio thread function
static void* AudioThreadFunc(void* arg) {
    printf("Audio thread started\n");
    
    while (!g_threadsShouldExit) {
        // Check audio buffer
        extern void Metal_UpdateAudio();
        Metal_UpdateAudio();
        
        // Sleep a bit to avoid consuming too much CPU
        usleep(1000); // 1ms
    }
    
    printf("Audio thread exiting\n");
    return NULL;
}

// Start the render thread
int StartRenderThread() {
    if (g_renderThreadRunning) {
        printf("Render thread already running\n");
        return 0;
    }
    
    g_threadsShouldExit = false;
    
    int result = pthread_create(&g_renderThread, NULL, RenderThreadFunc, NULL);
    if (result != 0) {
        printf("Failed to create render thread: %d\n", result);
        return result;
    }
    
    g_renderThreadRunning = true;
    printf("Render thread started successfully\n");
    return 0;
}

// Start the audio thread
int StartAudioThread() {
    if (g_audioThreadRunning) {
        printf("Audio thread already running\n");
        return 0;
    }
    
    g_threadsShouldExit = false;
    
    int result = pthread_create(&g_audioThread, NULL, AudioThreadFunc, NULL);
    if (result != 0) {
        printf("Failed to create audio thread: %d\n", result);
        return result;
    }
    
    g_audioThreadRunning = true;
    printf("Audio thread started successfully\n");
    return 0;
}

// Stop the render thread
void StopRenderThread() {
    if (!g_renderThreadRunning) {
        return;
    }
    
    // Signal thread to exit
    g_threadsShouldExit = true;
    
    // Wake up the thread
    pthread_mutex_lock(&g_frameMutex);
    g_frameReady = true;
    pthread_cond_signal(&g_frameReadyCond);
    pthread_mutex_unlock(&g_frameMutex);
    
    // Wait for thread to terminate
    pthread_join(g_renderThread, NULL);
    g_renderThreadRunning = false;
    
    printf("Render thread stopped\n");
}

// Stop the audio thread
void StopAudioThread() {
    if (!g_audioThreadRunning) {
        return;
    }
    
    // Signal thread to exit
    g_threadsShouldExit = true;
    
    // Wait for thread to terminate
    pthread_join(g_audioThread, NULL);
    g_audioThreadRunning = false;
    
    printf("Audio thread stopped\n");
}

// Check if render thread is running
bool IsRenderThreadRunning() {
    return g_renderThreadRunning;
}

// Check if audio thread is running
bool IsAudioThreadRunning() {
    return g_audioThreadRunning;
} 