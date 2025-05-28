#pragma once

// Include standard headers
#include <stddef.h>  // For size_t

// Only include in non-METAL builds
#ifndef BUILD_METAL
#include "burner_metal.h"

// Platform-independent path helpers
#ifdef __cplusplus
extern "C" {
#endif

int GetROMFilePath(char* szPath, size_t len);
int SetROMFilePath(const char* szPath);

// Bridge functions between Metal UI and FBNeo core
int ExecuteGame(const char* gameName);
int PauseGame(int isPaused);
void ResetGame();
int GetGameInfo(char* buffer, size_t len, int infoType);
int RunFrame(int bDraw);
int RenderToTexture(void* textureData, int width, int height);

// Metal-specific UI functions
int RunInGameMenu();
int ShowGameOptionsMenu();
int IsGameLoaded();
int IsGameActive();
int IsGamePaused();
int GetGameWidth();
int GetGameHeight();
int GetGameRefresh();
const char* GetGameTitle();
const char* GetGameManufacturer();
const char* GetGameYear();
int GetCurrentFPS();

// Input state
int GetPlayerInputState(int player, int inputType);
int SimulatePlayerInput(int player, int inputType, int value);

// AI related functions
int EnableAI(int isEnabled);
int SetAIPlayer(int player);
int SetAIDifficulty(int difficulty);
int EnableAITraining(int isEnabled);
int EnableAIDebugView(int isEnabled);

#ifdef __cplusplus
}
#endif
#endif // !BUILD_METAL 