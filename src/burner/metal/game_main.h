#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Main game integration for FBNeo Metal implementation

// Initialize the game
int GameMain_Init();

// Shutdown the game
int GameMain_Exit();

// Load a ROM
int GameMain_LoadROM(const char* romPath);

// Start the game
int GameMain_StartGame();

// Pause/unpause the game
int GameMain_PauseGame(bool pause);

// Reset the game
int GameMain_ResetGame();

// Stop the game
int GameMain_StopGame();

// Start the game loop
int GameMain_StartGameLoop();

// Stop the game loop
int GameMain_StopGameLoop();

// Run a single frame of the game
int GameMain_RunFrame();

// Get the current FPS
int GameMain_GetFPS();

// Should the game exit
bool GameMain_ShouldExit();

// Set the game's volume
int GameMain_SetVolume(int volume);

// Configure the game's controls
int GameMain_ConfigureControls();

// Is a specific input active
bool GameMain_IsInputActive(int player, int input);

// Update the audio buffer with new data
int GameMain_UpdateAudio(const short* data, int size);

#ifdef __cplusplus
}
#endif 