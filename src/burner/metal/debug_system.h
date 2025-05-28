#ifndef DEBUG_SYSTEM_H
#define DEBUG_SYSTEM_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Utility function for consistent logging
void LogDebugSection(int sectionIndex, const char* format, ...);

// Get formatted timestamp
const char* GetFormattedTimestamp(void);

// --- ROM CHECK Section Implementation ---
void ROM_CheckIntegrity(const char* romPath, int numFiles, int validFiles);

// --- MEM INIT Section Implementation ---
void MEM_ReportComponentAllocation(const char* componentName, size_t size, bool success);

// --- HW INIT Section Implementation ---
void HW_ReportComponentInitialization(const char* componentName, bool success);

// --- GRAPHICS INIT Section Implementation ---
void GRAPHICS_ReportAssetLoading(const char* assetType, int count, size_t memoryUsage);

// --- AUDIO INIT Section Implementation ---
void AUDIO_ReportInitialization(int sampleRate, int channels, int bitDepth, int bufferSize);

// --- INPUT INIT Section Implementation ---
void INPUT_ReportControllerMapping(int numButtons, int numControllers);

// --- EMULATOR Section Implementation ---
void EMULATOR_ReportStartup(const char* gameTitle, float targetFPS);

// --- RENDERER Section Implementation ---
void RENDERER_ReportInitialization(int width, int height, const char* apiVersion);

// --- RENDERER LOOP Section Implementation ---
void RENDERER_ReportFrameStats(int frameNumber, int spriteCount, int layerCount, float fps);

// --- AUDIO LOOP Section Implementation ---
void AUDIO_ReportStreamStats(int bufferFill, int bufferSize, int sampleRate, bool underrun);

// --- GAME START Section Implementation ---
void GAME_ReportRunningState(const char* gameTitle, float actualFPS, bool isRunningWell);

#ifdef __cplusplus
}
#endif

#endif // DEBUG_SYSTEM_H 