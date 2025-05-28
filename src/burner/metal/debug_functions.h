#ifndef DEBUG_FUNCTIONS_H
#define DEBUG_FUNCTIONS_H

#include <stddef.h>
#include <stdbool.h>

// Debug section indices
#define DEBUG_ROM_CHECK        1
#define DEBUG_MEMORY           2
#define DEBUG_EMULATOR         3
#define DEBUG_GAME             4
#define DEBUG_RENDERER         5
#define DEBUG_AUDIO            6
#define DEBUG_INPUT            7
#define DEBUG_MEM_INIT         8
#define DEBUG_HW_INIT          9
#define DEBUG_GRAPHICS_INIT    10
#define DEBUG_AUDIO_INIT       11
#define DEBUG_INPUT_INIT       12
#define DEBUG_RENDERER_LOOP    13
#define DEBUG_AUDIO_LOOP       14
#define DEBUG_GAME_START       15

// Core debug logging functions
void Debug_Log(int sectionIndex, const char* format, ...);
void Debug_PrintSectionHeader(int sectionIndex, const char* format, ...);
void LogDebugSection(int sectionIndex, const char* format, ...);

// ROM checking functions
void ROM_CheckIntegrity(const char* romPath, int numFiles, int validFiles);

// Memory tracking functions
void MEM_ReportComponentAllocation(const char* componentName, size_t size, bool success);

// Graphics tracking functions
void ReportGraphicsAssetLoading(const char* assetType, int count, size_t memoryUsed);

// Audio tracking functions
void ReportAudioInitialization(int sampleRate, int channels, int bitDepth, int bufferSize);
void ReportAudioStatus(int bufferSize, int bufferUsed, int underruns);
void AUDIO_ReportStreamStats(int bufferSize, int currentFill, int underruns, int overruns);

// Input tracking functions
void ReportInputInitialization(int buttonCount, int controllerCount);

// Rendering tracking functions
void ReportFrameRendering(int frameNumber, int spriteCount, int layerCount, float fps);

// Emulator status functions
void EMULATOR_ReportStartup(const char* gameTitle, float targetFPS);
void GAME_ReportRunningState(const char* gameTitle, float actualFPS, bool isRunningWell);

#endif // DEBUG_FUNCTIONS_H 