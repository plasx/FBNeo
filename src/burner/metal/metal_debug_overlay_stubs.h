#ifndef METAL_DEBUG_OVERLAY_STUBS_H
#define METAL_DEBUG_OVERLAY_STUBS_H

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
@class NSWindow;

// Basic type definitions if needed
typedef int INT32;

// Function declarations
INT32 Metal_InitDebugOverlay(NSWindow* parentWindow);
INT32 Metal_ExitDebugOverlay();
void Metal_UpdateDebugOverlay(int frameCount);
void Debug_Log(int category, const char* message);
void Debug_PrintSectionHeader(int category, const char* header);
void Metal_ShowSaveStateStatus(bool isSave);
void Metal_ShowStatusMessage(const char* message, int seconds);

// Debug categories
#define DEBUG_RENDERER 1
#define DEBUG_METAL 2
#define DEBUG_AUDIO_LOOP 3
#define DEBUG_INPUT_LOOP 4

#ifdef __cplusplus
}
#endif

#endif // METAL_DEBUG_OVERLAY_STUBS_H 