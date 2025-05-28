#ifndef DEBUG_CONTROLLER_H
#define DEBUG_CONTROLLER_H

#ifdef __cplusplus
extern "C" {
#endif

// Debug section identifiers
typedef enum {
    DEBUG_ROM_CHECK = 0,
    DEBUG_MEM_INIT,
    DEBUG_HW_INIT,
    DEBUG_GRAPHICS_INIT,
    DEBUG_AUDIO_INIT,
    DEBUG_INPUT_INIT,
    DEBUG_EMULATOR,
    DEBUG_RENDERER,
    DEBUG_RENDERER_LOOP,
    DEBUG_AUDIO_LOOP,
    DEBUG_INPUT_LOOP,
    DEBUG_GAME_START,
    DEBUG_METAL,
    DEBUG_INFO,
    DEBUG_MAX_SECTIONS
} DebugSection;

// Global flag to track if we've already displayed the standard debug format
extern int g_DebugFormatDisplayed;

// Initialize the debug controller
void Debug_Init(const char* logFileName);

// Clean up debug resources
void Debug_Exit();

// Direct output forcing function for maximum visibility
void ForceOutput(const char* message);

// Log a debug message to a specific section
void Debug_Log(int sectionIndex, const char* format, ...);

// Print a section header with formatted message
void Debug_PrintSectionHeader(int sectionIndex, const char* format, ...);

// Log ROM loading progress with multiple sections
void Debug_LogROMLoading(const char* romPath);

// Get a section prefix string
const char* Debug_GetSectionPrefix(DebugSection section);

#ifdef __cplusplus
}
#endif

#endif // DEBUG_CONTROLLER_H 