#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "hardware_tracking.h"

// Simple hardware tracking for CPS2 emulation

// CPU state
static int g_cpuInitialized = 0;
static int g_cpuFrequency = 0;

// Frame counter
static int g_frameCounter = 0;

// Initialize CPU tracking
void CPU_Init(int frequency) {
    g_cpuInitialized = 1;
    g_cpuFrequency = frequency;
    g_frameCounter = 0;
    
    printf("[CPU] CPU initialized at %d Hz\n", frequency);
}

// CPU Reset
void CPU_Reset() {
    if (g_cpuInitialized) {
        printf("[CPU] CPU reset\n");
    }
}

// Running a CPU frame
void CPU_RunFrame() {
    if (g_cpuInitialized) {
        g_frameCounter++;
        if (g_frameCounter % 60 == 0) {
            printf("[CPU] Frame %d\n", g_frameCounter);
        }
    }
}

// Get CPU frequency
int CPU_GetFrequency() {
    return g_cpuFrequency;
}

// Cleanup CPU tracking
void CPU_Exit() {
    g_cpuInitialized = 0;
    printf("[CPU] CPU shutdown\n");
}

// Audio state
static int g_audioInitialized = 0;
static int g_audioSampleRate = 0;
static int g_audioChannels = 0;

// Initialize audio tracking
void Audio_Init(int sampleRate, int channels) {
    g_audioInitialized = 1;
    g_audioSampleRate = sampleRate;
    g_audioChannels = channels;
    
    printf("[AUDIO] Audio initialized at %d Hz, %d channels\n", 
           sampleRate, channels);
}

// Audio reset
void Audio_Reset() {
    if (g_audioInitialized) {
        printf("[AUDIO] Audio reset\n");
    }
}

// Get audio sample rate
int Audio_GetSampleRate() {
    return g_audioSampleRate;
}

// Get audio channels
int Audio_GetChannels() {
    return g_audioChannels;
}

// Cleanup audio tracking
void Audio_Exit() {
    g_audioInitialized = 0;
    printf("[AUDIO] Audio shutdown\n");
}

// CPS2 state tracking
static int g_cpsInitialized = 0;
static int g_cps2Mode = 0;
static int g_cpsNumSprites = 0;
static int g_cpsNumLayers = 0;

// Initialize CPS2 tracking
void CPS2_Init() {
    g_cpsInitialized = 1;
    g_cps2Mode = 1;        // 1 = CPS2, 0 = CPS1
    g_cpsNumSprites = 0;
    g_cpsNumLayers = 0;
    
    printf("[CPS2] CPS2 hardware initialized\n");
}

// Track CPS2 sprites
void CPS2_TrackSprite(int spriteIndex, int x, int y, int width, int height) {
    if (g_cpsInitialized) {
        g_cpsNumSprites++;
    }
}

// Track CPS2 layers
void CPS2_TrackLayer(int layerIndex, int enabled) {
    if (g_cpsInitialized) {
        g_cpsNumLayers = (layerIndex > g_cpsNumLayers) ? layerIndex : g_cpsNumLayers;
    }
}

// Cleanup CPS2 tracking
void CPS2_Exit() {
    g_cpsInitialized = 0;
    printf("[CPS2] CPS2 hardware shutdown\n");
}

// Print CPS2 status
void CPS2_PrintStatus() {
    if (g_cpsInitialized) {
        printf("[CPS2] Status: %d sprites, %d layers\n", 
               g_cpsNumSprites, g_cpsNumLayers);
    }
} 