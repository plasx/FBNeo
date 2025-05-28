#pragma once

#ifdef __OBJC__
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <Foundation/Foundation.h>
#else
typedef void* MTLDevice;
typedef void* MTLTexture;
typedef void* MTLCommandQueue;
typedef void* MTLRenderPipelineState;
typedef void* MTLSamplerState;
typedef void* MTLCommandBuffer;
typedef void* MTLRenderCommandEncoder;
typedef void* MTLBlitCommandEncoder;
typedef void* MTLBuffer;
typedef void* MTKView;
#endif

#include "metal_declarations.h"

#ifdef __cplusplus
extern "C" {
#endif

// Metal renderer initialization and shutdown
bool Metal_InitRenderer(void* device);
void Metal_ShutdownRenderer();
void* Metal_InitRendererWithView(void* view);

// Metal frame management
int Metal_RenderFrame(void* frameData, int width, int height);
void Metal_UpdateTexture(void* data, int width, int height);
int Metal_RunFrame(int bDraw);
void* Metal_GetFrameBuffer();

// Metal renderer settings
void Metal_SetRenderState(int state, int value);
const char* Metal_GetRendererInfo();

// Metal sampler states
void* Metal_GetLinearSampler(void* device);
void* Metal_GetNearestSampler(void* device);
void Metal_ReleaseSampler(void* sampler);
void Metal_ClearSamplerCache();

// Metal shader compilation
void* Metal_CompileLibraryWithSource(const char* source, void* device);
void* Metal_LoadLibraryFromFile(const char* path, void* device);
void* Metal_CreateRenderPipeline(void* device, const char* vertexFunc, const char* fragmentFunc, int pixelFormat);
void Metal_ReleaseObject(void* obj);

// Metal debug overlay
void* Metal_CreateDebugOverlay(void* device, int width, int height);
void Metal_DebugOverlayBeginFrame(void* overlay);
void Metal_DebugOverlayAddText(void* overlay, const char* text, float x, float y, 
                              float size, float r, float g, float b, float a);
void* Metal_DebugOverlayGetTexture(void* overlay);
void Metal_DebugOverlaySetConfig(void* overlay, bool enabled, bool showFPS, bool showFrameTime,
                               bool showSystemInfo, bool showGameInfo, bool showInputState,
                               int position, float opacity, bool largeText);
void Metal_DebugOverlayAddGameInfo(void* overlay, const char* gameName, int width, int height);
void Metal_DebugOverlayAddSystemInfo(void* overlay);
void Metal_DebugOverlayAddInputState(void* overlay, uint32_t buttonState);
void Metal_DestroyDebugOverlay(void* overlay);

// Metal screen capture
void* Metal_CreateScreenCapture(void* device);
void Metal_ScreenCaptureSetTexture(void* capture, void* texture);
void Metal_ScreenCaptureSetDirectory(void* capture, const char* directory);
bool Metal_ScreenCaptureScreenshot(void* capture);
bool Metal_ScreenCaptureStartRecording(void* capture);
bool Metal_ScreenCaptureStopRecording(void* capture);
void Metal_ScreenCaptureUpdate(void* capture);
void Metal_DestroyScreenCapture(void* capture);

// Metal audio integration
int Metal_InitAudioSystem(int sampleRate);
void Metal_StopAudioSystem();
int Metal_SetAudioCallback(void* callback);
int Metal_AddAudioSamples(const short* samples, int count);
int Metal_PauseAudio(int pause);
int Metal_SetAudioVolume(float volume);
float Metal_GetAudioVolume();
bool Metal_IsAudioInitialized();
int Metal_GetAudioFrameSize();
int Metal_ProcessAudioFrame();
int Metal_ResetAudio();
float Metal_GetAudioBufferFillPercentage();

// Audio system interface
int MetalAudioInit();
int MetalAudioExit();
int MetalAudioPlay();
int MetalAudioStop();
int MetalAudioSetVolume(int vol);
int MetalAudioGetSettings(InterfaceInfo* pInfo);
int MetalAudio_Init(int sampleRate, int channels);
int MetalAudio_Start();
int MetalAudio_Stop();
int MetalAudio_Exit();
int MetalAudio_SetVolume(float volume);
int MetalAudio_Pause(int pause);
int MetalAudio_Reset();
bool MetalAudio_IsRunning();

// Metal input interface
int MetalInput_Init();
int MetalInput_Exit();
int MetalInput_Make(bool bCopy);
void MetalInput_SetKeyState(int key, bool isPressed);
void MetalInput_SetJoypadState(int player, int button, bool isPressed);
void MetalInput_SetAxisState(int player, int axis, int value);
void MetalInput_SetDefaultKeymap();
void MetalInput_LoadKeymap(const char* filename);
void MetalInput_SaveKeymap(const char* filename);
int MetalInput_GetDeviceCount();
const char* MetalInput_GetDeviceName(int index);
void MetalInput_SetActiveDevice(int index);

// FBNeo core integration
void UpdateMetalFrameTexture(const void* frameData, unsigned int width, unsigned int height);
int Metal_RunFrame(int bDraw);

#ifdef __cplusplus
}
#endif 