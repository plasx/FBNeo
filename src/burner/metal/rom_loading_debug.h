#ifndef ROM_LOADING_DEBUG_H
#define ROM_LOADING_DEBUG_H

#include "metal_declarations.h"

#ifdef __cplusplus
extern "C" {
#endif

// Debug logging levels
#define LOG_ERROR 0     // Errors only
#define LOG_WARNING 1   // Warnings and errors
#define LOG_INFO 2      // Basic informational messages
#define LOG_DETAIL 3    // Detailed logging
#define LOG_VERBOSE 4   // Everything

// Debug log functions
void ROMLoader_InitDebugLog(void);
void ROMLoader_CloseDebugLog(void);
void ROMLoader_DebugLog(int level, const char* format, ...);
void ROMLoader_SetDebugLevel(int level);
void ROMLoader_DumpMemory(const void* data, int size, const char* description);
void ROMLoader_LogROMInfo(const char* romPath);
void ROMLoader_TrackLoadStep(const char* step, const char* format, ...);
bool ROMLoader_VerifyROMData(const UINT8* data, INT32 size, const char* romName);

#ifdef __cplusplus
}
#endif

#endif // ROM_LOADING_DEBUG_H
