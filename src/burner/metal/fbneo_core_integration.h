#pragma once

#include "burnint.h"

// Game state structure for AI integration
typedef struct {
    uint8_t* screenBuffer;
    int screenWidth;
    int screenHeight;
    int playerHealth[2];
    int opponentHealth[2];
    int playerPosition[2][2];
    int opponentPosition[2][2];
    int currentScore;
    int currentLives;
    int currentLevel;
    int frameCounter;
    char gameMode[32];
    void* rawMemoryPtr;
    size_t rawMemorySize;
} GameStateData;

// Save state header structure
typedef struct {
    char gameId[32];
    char description[256];
    char timestamp[32];
    char version[32];
    int dataSize;
    int thumbnailWidth;
    int thumbnailHeight;
    int thumbnailSize;
} SaveStateHeader;

#ifdef __cplusplus
extern "C" {
#endif

// Core initialization and shutdown
int FBNeo_Core_Initialize();
int FBNeo_Core_Shutdown();

// ROM loading and management
int FBNeo_Core_LoadROM(const char* romName);
int LoadROMByName(const char* romName);
int LoadROMByDriverIndex(int driverIndex);
bool DetectROMPaths();
bool InitializeROMList();
int FBNeo_Core_GetAvailableROMs(char** romNames, int maxCount);

// Game execution
int FBNeo_Core_RunFrame(bool render);
int FBNeo_Core_Reset();
int FBNeo_Core_SetPause(bool pause);

// Game state management
int FBNeo_Core_GetDriverInfo(struct BurnDrvMeta* info);
int FBNeo_Core_GetGameState(void* stateBuffer, int bufferSize);
const char* FBNeo_Core_GetLastError();
bool FBNeo_Core_IsInitialized();
bool FBNeo_Core_IsGameRunning();

// ROM path management
const char* FBNeo_Core_GetROMPath();
int FBNeo_Core_SetROMPath(const char* path);

// Save state management
bool FBNeo_SaveState_Initialize();
bool FBNeo_SaveState_Save(int slot, const char* description);
bool FBNeo_SaveState_Load(int slot);
void FBNeo_SaveState_AutoSave();
void FBNeo_SaveState_SetAutoSave(bool enable);
void FBNeo_SaveState_SetAutoSaveInterval(int seconds);
int FBNeo_SaveState_GetStateList(SaveStateHeader** headers, int maxCount);
int FBNeo_SaveState_GetCurrentSlot();
void FBNeo_SaveState_SetCurrentSlot(int slot);
int FBNeo_SaveState_GetMaxSlots();
void FBNeo_SaveState_SetMaxSlots(int maxSlots);
bool FBNeo_SaveState_GetThumbnail(int slot, void** thumbnailData, int* width, int* height, int* size);
bool FBNeo_SaveState_Delete(int slot);
bool FBNeo_SaveState_Exists(int slot);
void FBNeo_SaveState_Reset();
bool FBNeo_SaveState_GetInfo(int slot, SaveStateHeader* header);

// CPS2-specific functionality
bool Cps2_IsValidROM(const char* romPath);
int Cps2_LoadROM(const char* romPath);
int Cps2_OnDriverInit();
int Cps2_OnFrame();
void Cps2_SetupMetalLinkage();
int Cps2_FillGameState(void* stateData);

#ifdef __cplusplus
}
#endif 