#import <Cocoa/Cocoa.h>
#import <Foundation/Foundation.h>
#import <MetalKit/MetalKit.h>
#include "burnint.h"
#include "metal_bridge.h"
#include "game_renderer.h"
#include "game_state.h"
#include <sys/stat.h>

// Game state management for FBNeo Metal implementation

// State variables
static GameState gameState = {
    .isInitialized = false,
    .isRunning = false,
    .isPaused = false,
    .frameCount = 0,
    .fps = 0,
    .lastFrameTime = 0,
    .romPath = {0},
    .driverIndex = -1,
    .volume = 100
};

// Notification delegate
@interface GameStateDelegate : NSObject
- (void)gameStarted;
- (void)gamePaused:(BOOL)paused;
- (void)gameReset;
- (void)gameStopped;
@end

// Implementation of delegate
@implementation GameStateDelegate
- (void)gameStarted {
    // Post notification that game started
    [[NSNotificationCenter defaultCenter] postNotificationName:@"GameStarted" object:nil];
}

- (void)gamePaused:(BOOL)paused {
    // Post notification that game was paused/unpaused
    NSDictionary *info = @{@"paused": @(paused)};
    [[NSNotificationCenter defaultCenter] postNotificationName:@"GamePaused" object:nil userInfo:info];
}

- (void)gameReset {
    // Post notification that game was reset
    [[NSNotificationCenter defaultCenter] postNotificationName:@"GameReset" object:nil];
}

- (void)gameStopped {
    // Post notification that game stopped
    [[NSNotificationCenter defaultCenter] postNotificationName:@"GameStopped" object:nil];
}
@end

// Static delegate instance
static GameStateDelegate *delegate = nil;

// Initialize the game state system
int GameState_Init() {
    // Create the delegate
    delegate = [[GameStateDelegate alloc] init];
    
    // Reset state
    gameState.isInitialized = true;
    gameState.isRunning = false;
    gameState.isPaused = false;
    gameState.frameCount = 0;
    gameState.fps = 0;
    gameState.lastFrameTime = [[NSDate date] timeIntervalSince1970];
    gameState.driverIndex = -1;
    gameState.volume = 100;
    
    printf("Game state system initialized\n");
    
    return 0;
}

// Shutdown the game state system
int GameState_Exit() {
    // Stop the game if it's running
    if (gameState.isRunning) {
        GameState_StopGame();
    }
    
    // Release the delegate
    delegate = nil;
    
    // Reset state
    gameState.isInitialized = false;
    
    printf("Game state system shutdown\n");
    
    return 0;
}

// Load a ROM and prepare to run the game
int GameState_LoadROM(const char *romPath) {
    printf("GameState_LoadROM(%s)\n", romPath);
    
    if (!gameState.isInitialized) {
        printf("Error: Game state system not initialized\n");
        return 1;
    }
    
    // If a game is already running, stop it first
    if (gameState.isRunning) {
        GameState_StopGame();
    }
    
    // Store the ROM path
    strncpy(gameState.romPath, romPath, sizeof(gameState.romPath) - 1);
    gameState.romPath[sizeof(gameState.romPath) - 1] = '\0';
    
    // Parse the ROM filename to get the base name
    const char* filename = strrchr(romPath, '/');
    if (filename) {
        filename++; // Skip the slash
    } else {
        filename = romPath;
    }
    
    // Extract ROM name (remove extension)
    char romName[256] = {0};
    strncpy(romName, filename, sizeof(romName) - 1);
    char* ext = strrchr(romName, '.');
    if (ext) {
        *ext = '\0'; // Remove extension
    }
    
    printf("ROM name: %s\n", romName);
    
    // Try to load the ROM using the Metal bridge
    int result = Metal_LoadROM(romPath);
    if (result != 0) {
        printf("Error: Failed to load ROM: %d\n", result);
        
        // Try to provide more information about why the load failed
        struct stat sb;
        if (stat(romPath, &sb) != 0) {
            printf("ROM file does not exist: %s\n", romPath);
        } else if (!S_ISREG(sb.st_mode)) {
            printf("Path exists but is not a regular file: %s\n", romPath);
        } else {
            printf("ROM file exists (%lld bytes) but could not be loaded. May be missing files, corrupted, or unsupported.\n", 
                (long long)sb.st_size);
        }
        
        return result;
    }
    
    // Get the driver index for the loaded ROM
    extern UINT32 nBurnDrvActive;
    gameState.driverIndex = nBurnDrvActive;
    
    // Get ROM dimensions from the driver
    INT32 width, height;
    BurnDrvGetVisibleSize(&width, &height);
    
    // Initialize the game renderer with proper resolution
    result = GameRenderer_Init(width, height, 32);
    if (result != 0) {
        printf("Error: Failed to initialize game renderer: %d\n", result);
        return result;
    }
    
    printf("ROM loaded successfully: %s (driver index: %d, dimensions: %dx%d)\n", 
           gameState.romPath, gameState.driverIndex, width, height);
    
    return 0;
}

// Start running the game
int GameState_StartGame() {
    printf("GameState_StartGame()\n");
    
    if (!gameState.isInitialized) {
        printf("Error: Game state system not initialized\n");
        return 1;
    }
    
    if (gameState.driverIndex < 0) {
        printf("Error: No ROM loaded\n");
        return 1;
    }
    
    if (gameState.isRunning) {
        printf("Game is already running\n");
        return 0;
    }
    
    // Enable core rendering to use the actual game output
    GameRenderer_SetUseCoreRendering(true);
    printf("Core rendering enabled\n");
    
    // Start the game renderer
    int result = GameRenderer_Start();
    if (result != 0) {
        printf("Error: Failed to start game renderer\n");
        return result;
    }
    
    // Set state
    gameState.isRunning = true;
    gameState.isPaused = false;
    gameState.frameCount = 0;
    gameState.lastFrameTime = [[NSDate date] timeIntervalSince1970];
    
    // Notify that game started
    [delegate gameStarted];
    
    printf("Game started\n");
    
    return 0;
}

// Pause/unpause the game
int GameState_PauseGame(bool pause) {
    printf("GameState_PauseGame(%d)\n", pause);
    
    if (!gameState.isInitialized || !gameState.isRunning) {
        printf("Error: Game not running\n");
        return 1;
    }
    
    // Only change state if it's different
    if (gameState.isPaused != pause) {
        gameState.isPaused = pause;
        
        // Update Metal state
        Metal_PauseGame(pause ? 1 : 0);
        
        // Notify that game was paused/unpaused
        [delegate gamePaused:pause];
        
        printf("Game %s\n", pause ? "paused" : "resumed");
    }
    
    return 0;
}

// Reset the game
int GameState_ResetGame() {
    printf("GameState_ResetGame()\n");
    
    if (!gameState.isInitialized || !gameState.isRunning) {
        printf("Error: Game not running\n");
        return 1;
    }
    
    // Reset the Metal game
    int result = Metal_ResetGame();
    if (result != 0) {
        printf("Error: Failed to reset game\n");
        return result;
    }
    
    // Reset state
    gameState.frameCount = 0;
    gameState.lastFrameTime = [[NSDate date] timeIntervalSince1970];
    
    // If the game was paused, unpause it
    if (gameState.isPaused) {
        GameState_PauseGame(false);
    }
    
    // Notify that game was reset
    [delegate gameReset];
    
    printf("Game reset\n");
    
    return 0;
}

// Stop the game
int GameState_StopGame() {
    printf("GameState_StopGame()\n");
    
    if (!gameState.isInitialized || !gameState.isRunning) {
        printf("Game not running\n");
        return 0;
    }
    
    // Stop the game renderer
    int result = GameRenderer_Stop();
    if (result != 0) {
        printf("Error: Failed to stop game renderer\n");
        return result;
    }
    
    // Reset state
    gameState.isRunning = false;
    gameState.isPaused = false;
    
    // Notify that game stopped
    [delegate gameStopped];
    
    printf("Game stopped\n");
    
    return 0;
}

// Render a frame of the game
int GameState_RenderFrame() {
    if (!gameState.isInitialized || !gameState.isRunning || gameState.isPaused) {
        // Don't render if not running or paused
        return 0;
    }
    
    // Increment frame counter
    gameState.frameCount++;
    
    // Calculate FPS every second
    NSTimeInterval currentTime = [[NSDate date] timeIntervalSince1970];
    NSTimeInterval elapsed = currentTime - gameState.lastFrameTime;
    
    if (elapsed >= 1.0) {
        gameState.fps = (int)(gameState.frameCount / elapsed);
        gameState.frameCount = 0;
        gameState.lastFrameTime = currentTime;
    }
    
    // Render the frame
    return GameRenderer_RenderFrame();
}

// Get the current game state
const GameState* GameState_GetState() {
    return &gameState;
}

// Set the game volume
int GameState_SetVolume(int volume) {
    // Clamp volume to 0-100
    if (volume < 0) volume = 0;
    if (volume > 100) volume = 100;
    
    gameState.volume = volume;
    
    // Update Metal volume
    Metal_SetVolume(volume);
    
    return 0;
}

// Get the current frame rate
int GameState_GetFPS() {
    return gameState.fps;
}

// Check if a game is loaded
bool GameState_IsGameLoaded() {
    return gameState.driverIndex >= 0;
}

// Check if the game is running
bool GameState_IsGameRunning() {
    return gameState.isRunning;
}

// Check if the game is paused
bool GameState_IsGamePaused() {
    return gameState.isPaused;
} 