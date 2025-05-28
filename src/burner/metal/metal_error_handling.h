#ifndef _METAL_ERROR_HANDLING_H_
#define _METAL_ERROR_HANDLING_H_

#include "metal_declarations.h"

#ifdef __cplusplus
extern "C" {
#endif

// Error constants - using the ones already defined in metal_declarations.h

// Error handling functions
const char* Metal_GetLastErrorMessage();
MetalErrorCode Metal_GetLastErrorCode();
void Metal_ClearLastError();
bool Metal_HasError();
void Metal_SetError(MetalErrorCode code, const char* message);
void Metal_SetErrorEx(MetalErrorCode code, const char* message, const char* function, const char* file, int line);

// Log level functions
void Metal_SetLogLevel(MetalLogLevel level);
void Metal_LogMessage(MetalLogLevel level, const char* format, ...);
void Metal_LogHexDump(const void* data, size_t size, const char* description);

// Debug fallback functions
int Metal_EnableFallbackRenderer();
int Metal_EnableFallbackAudio();
int Metal_EnableFallbackInput();

// Debug mode tracking
bool Metal_IsDebugMode();
void Metal_SetDebugMode(bool enabled);
void Metal_SetFrameLogInterval(int frames);

// Game state dumping - for debugging
void Metal_DumpGameState();

#ifdef __cplusplus
}
#endif

#endif /* _METAL_ERROR_HANDLING_H_ */ 