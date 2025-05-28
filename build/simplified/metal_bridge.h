#ifndef METAL_BRIDGE_H
#define METAL_BRIDGE_H

#ifdef __cplusplus
extern "C" {
#endif

// Frame buffer structure
typedef struct {
    uint32_t* data;
    int width;
    int height;
    int pitch;
    bool updated;
} FrameBuffer;

// Frame buffer functions
bool InitFrameBuffer(int width, int height);
int Metal_GenerateTestPattern(int patternType);

// Game status functions
const char* Metal_GetGameTitle(void);
void Metal_SetGameTitle(const char* title);
bool Metal_IsGameRunning(void);
void Metal_SetGameRunning(bool running);
float Metal_GetFrameRate(void);
int Metal_GetTotalFrames(void);

// Error handling
void Metal_SetError(int code, const char* message);
bool Metal_HasError(void);
const char* Metal_GetLastErrorMessage(void);
void Metal_SetDebugMode(bool enable);
int Metal_EnableFallbackAudio(void);

// ROM loading
bool Metal_LoadAndInitROM(const char* path);
void Metal_UnloadROM(void);

#ifdef __cplusplus
}
#endif

#endif // METAL_BRIDGE_H
