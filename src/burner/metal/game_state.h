#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Game state structure for FBNeo Metal implementation
typedef struct {
    bool isInitialized;     // Is the state system initialized
    bool isRunning;         // Is a game currently running
    bool isPaused;          // Is the game paused
    int frameCount;         // Frame counter for FPS calculation
    int fps;                // Current FPS
    double lastFrameTime;   // Last time FPS was calculated
    char romPath[1024];     // Path to the loaded ROM
    int driverIndex;        // Driver index for the loaded ROM
    int volume;             // Audio volume (0-100)
} GameState;

// Initialize the game state system
int GameState_Init();

// Shutdown the game state system
int GameState_Exit();

// Load a ROM and prepare to run the game
int GameState_LoadROM(const char *romPath);

// Start running the game
int GameState_StartGame();

// Pause/unpause the game
int GameState_PauseGame(bool pause);

// Reset the game
int GameState_ResetGame();

// Stop the game
int GameState_StopGame();

// Render a frame of the game
int GameState_RenderFrame();

// Get the current game state
const GameState* GameState_GetState();

// Set the game volume
int GameState_SetVolume(int volume);

// Get the current frame rate
int GameState_GetFPS();

// Check if a game is loaded
bool GameState_IsGameLoaded();

// Check if the game is running
bool GameState_IsGameRunning();

// Check if the game is paused
bool GameState_IsGamePaused();

#ifdef __cplusplus
}
#endif 