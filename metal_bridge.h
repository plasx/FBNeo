#pragma once

// Basic types and definitions for Metal-FBNeo integration
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Define common types
typedef uint8_t UINT8;
typedef int8_t INT8;
typedef uint16_t UINT16;
typedef int16_t INT16;
typedef uint32_t UINT32;
typedef int32_t INT32;
typedef uint64_t UINT64;
typedef int64_t INT64;

// Constants
#define MAX_PATH 260
#define DIRS_MAX 10
#define DRV_NAME (0)
#define DRV_FULLNAME (2)

// Game driver metadata
struct BurnDrvMeta {
    char* szShortName;
    char* szFullNameA;
    INT32 nWidth;
    INT32 nHeight;
    INT32 nAspectX;
    INT32 nAspectY;
};

// External references to core variables - these will be linked to the actual core
extern BurnDrvMeta BurnDrvInfo;
extern UINT32 nBurnDrvCount;
extern UINT32 nBurnDrvActive;
extern INT32 nBurnBpp;
extern INT32 nBurnPitch;
extern UINT8* pBurnDraw;
extern INT32 nBurnSoundRate;
extern INT32 nBurnSoundLen;
extern INT16* pBurnSoundOut;

// External references to Metal-specific variables
extern UINT8* pBurnDraw_Metal;
extern INT32 nBurnPitch_Metal;
extern INT32 nBurnBpp_Metal;
extern char szAppRomPaths[DIRS_MAX][MAX_PATH];
extern char szAppDirPath[MAX_PATH];
extern bool g_gameInitialized;

// Forward declarations for C++
#ifdef __cplusplus
namespace fbneo {
namespace ai {
class AITorchPolicy;
class RLAlgorithm;
struct AIInputFrame;
struct AIOutputAction;
struct GameState;
}
}
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Core bridge functions
INT32 BurnLibInit_Metal();
INT32 BurnLibExit_Metal();
INT32 BurnDrvInit_Metal(INT32 nDrvNum);
INT32 BurnDrvExit_Metal();
char* BurnDrvGetTextA_Metal(UINT32 i);
INT32 SetBurnHighCol(INT32 nDepth);
INT32 BurnDrvGetIndexByName(const char* szName);
INT32 BurnDrvFrame();

// Metal integration functions
int Metal_Init();
int Metal_Exit();
int Metal_LoadROM(const char* romPath);
int Metal_RunFrame(bool bDraw);
int Metal_RenderFrame(void* frameData, int width, int height);
int Metal_ShowTestPattern(int width, int height);
void UpdateMetalFrameTexture(const void* frameData, unsigned int width, unsigned int height);

// Input functions
int Metal_HandleKeyDown(int keyCode);
int Metal_HandleKeyUp(int keyCode);
int Metal_InitInput();

// Path management functions
int GetCurrentROMPath(char* szPath, size_t len);
int SetCurrentROMPath(const char* szPath);
int ValidateROMPath(const char* path);

// Audio functions
int Metal_InitAudio(int sampleRate);
int Metal_SetAudioEnabled(int enabled);
int Metal_SetVolume(int volume);
int Metal_GetAudioBufferSize();
bool Metal_IsAudioEnabled();
int Metal_GetVolume();
short* Metal_GetAudioBuffer();
int Metal_InitAudioSystem(int sampleRate);
int Metal_PauseAudio(int pause);
int Metal_ShutdownAudio();
int Metal_ResetAudio();

// AI functions
int Metal_SetAIEnabled(int enabled);
int Metal_SetAIDifficulty(int level);
int Metal_SetAIControlledPlayer(int playerIndex);
int Metal_SetAITrainingMode(int enabled);
int Metal_SetAIDebugOverlay(int enabled);
int Metal_LoadAIModel(const char* modelPath);
const char* Metal_GetCurrentModelPath();
int Metal_IsAIEnabled();
int Metal_GetAIDifficulty();
int Metal_GetAIControlledPlayer();
int Metal_IsAITrainingMode();
int Metal_IsAIDebugOverlayEnabled();
int AI_ProcessFrame(const void* frameData, int width, int height);

// Debug visualization
int Metal_ToggleHitboxViewer(int enabled);
int Metal_ToggleFrameData(int enabled);
int Metal_ToggleInputDisplay(int enabled);
int Metal_ToggleGameStateDisplay(int enabled);

// CPS2 support
void Cps2_SetupMetalLinkage();

// Advanced AI system (C++ side only)
#ifdef __cplusplus
// AI system initialization and shutdown
int Metal_InitializeAI(const char* modelPath, const char* algorithmType);
int Metal_ShutdownAI();

// AI model access
fbneo::ai::AITorchPolicy* Metal_GetAIModel();
fbneo::ai::RLAlgorithm* Metal_GetRLAlgorithm();

// AI training control
int Metal_SetAITrainingEnabled(bool enabled);
bool Metal_IsAITrainingEnabled();
int Metal_SetAutoResetEnabled(bool enabled);
bool Metal_IsAutoResetEnabled();

// Game state and reward functions
int Metal_ExtractGameState(fbneo::ai::GameState& state);
bool Metal_IsEpisodeOver(const fbneo::ai::GameState& state);
float Metal_CalculateReward(const fbneo::ai::AIInputFrame& prevFrame, 
                         const fbneo::ai::GameState& currentState);
#endif

#ifdef __cplusplus
}
#endif 