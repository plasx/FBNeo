#import <Cocoa/Cocoa.h>
#import <MetalKit/MetalKit.h>
#import <QuartzCore/QuartzCore.h>
#import <Foundation/Foundation.h>
#import <CoreVideo/CoreVideo.h>
#include "burnint.h"
#include "metal_bridge.h"
#include "game_renderer.h"
#include "game_state.h"
#include "game_audio.h"
#include "game_input.h"
#include "app/metal_app.h"
#include "game_main.h"

// External C function declarations
extern "C" void* GetMetalDelegate();

// Game main loop integration for FBNeo Metal implementation

// Game state - used to track the overall application state
static BOOL g_localGameInitialized = NO;  // Renamed to avoid conflict
static BOOL g_gameRunning = NO;
static BOOL g_gamePaused = NO;
static NSString *g_currentROMPath = nil;
static NSTimer *g_gameTimer = nil;
static NSTimeInterval g_lastFrameTime = 0;
static int g_frameCounter = 0;
static float g_fps = 0;
static BOOL g_exitRequested = NO;

// FPS calculation
static NSTimeInterval g_fpsLastTime = 0;
static int g_fpsFrameCount = 0;

// Forward declaration
void UpdateGameFPS();
void FrameTick(NSTimer *timer);

// External variables needed by the game timer
extern bool g_gameInitialized;

// Main game timer - used for syncing the emulation to the display refresh rate
static CVDisplayLinkRef displayLink = NULL;
static bool isTimerRunning = false;

// Display link callback
static CVReturn GameTimerCallback(CVDisplayLinkRef displayLink, 
                                 const CVTimeStamp* now, 
                                 const CVTimeStamp* outputTime, 
                                 CVOptionFlags flagsIn, 
                                 CVOptionFlags* flagsOut, 
                                 void* displayLinkContext) {
    // Run a frame of the emulation
    Metal_RunFrame(true);
    
    // Always return success
    return kCVReturnSuccess;
}

// Display link callback for the modern CADisplayLink
@interface DisplayLinkTarget : NSObject
+ (instancetype)sharedInstance;
- (void)displayLinkTick:(CADisplayLink *)sender;
@end

@implementation DisplayLinkTarget
+ (instancetype)sharedInstance {
    static DisplayLinkTarget *instance = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        instance = [[self alloc] init];
    });
    return instance;
}

- (void)displayLinkTick:(CADisplayLink *)sender {
    @autoreleasepool {
        // Call our game update/render
        GameMain_RunFrame();
    }
}
@end

// Initialize the game
int GameMain_Init() {
    NSLog(@"GameMain_Init()");
    
    if (g_localGameInitialized) {
        NSLog(@"Game already initialized");
        return 0;
    }
    
    // Initialize Metal
    int result = Metal_Init();
    if (result != 0) {
        NSLog(@"Error: Failed to initialize Metal: %d", result);
        return result;
    }
    
    // Initialize the game state system
    result = GameState_Init();
    if (result != 0) {
        NSLog(@"Error: Failed to initialize game state: %d", result);
        return result;
    }
    
    // Initialize the input system
    result = GameInput_Init();
    if (result != 0) {
        NSLog(@"Error: Failed to initialize input system: %d", result);
        return result;
    }
    
    // Initialize the audio system (44.1kHz, 2048 samples, stereo)
    result = GameAudio_Init(44100, 2048, 2);
    if (result != 0) {
        NSLog(@"Error: Failed to initialize audio system: %d", result);
        return result;
    }
    
    g_localGameInitialized = YES;
    g_gameRunning = NO;
    g_gamePaused = NO;
    g_exitRequested = NO;
    g_lastFrameTime = [[NSDate date] timeIntervalSince1970];
    g_frameCounter = 0;
    g_fps = 0;
    g_fpsLastTime = g_lastFrameTime;
    g_fpsFrameCount = 0;
    
    NSLog(@"Game initialized");
    
    return 0;
}

// Shutdown the game
int GameMain_Exit() {
    NSLog(@"GameMain_Exit()");
    
    if (!g_localGameInitialized) {
        NSLog(@"Game not initialized");
        return 0;
    }
    
    // Stop the display link if it's running
    GameMain_StopGameLoop();
    
    // Shutdown systems in reverse order of initialization
    GameAudio_Exit();
    GameInput_Exit();
    GameState_Exit();
    Metal_Exit();
    
    g_localGameInitialized = NO;
    
    NSLog(@"Game shutdown");
    
    return 0;
}

// Load a ROM
int GameMain_LoadROM(const char* romPath) {
    NSLog(@"GameMain_LoadROM(%s)", romPath);
    
    if (!g_localGameInitialized) {
        NSLog(@"Error: Game not initialized");
        return 1;
    }
    
    // Load the ROM using the game state system
    int result = GameState_LoadROM(romPath);
    if (result != 0) {
        NSLog(@"Error: Failed to load ROM: %d", result);
        return result;
    }
    
    // Initialize renderer for the loaded game
    INT32 nWidth = 0;
    INT32 nHeight = 0;
    
    // Use the proper BurnDrvGetVisibleSize function
    BurnDrvGetVisibleSize(&nWidth, &nHeight);
    
    // For BurnDrvGetDepth, we'll use a default depth for now
    INT32 nDepth = 32;
    
    NSLog(@"Game dimensions: %dx%d, %d BPP", nWidth, nHeight, nDepth);
    
    result = GameRenderer_Init(nWidth, nHeight, nDepth);
    if (result != 0) {
        NSLog(@"Error: Failed to initialize renderer: %d", result);
        return result;
    }
    
    // Remember the ROM path
    g_currentROMPath = [NSString stringWithUTF8String:romPath];
    
    NSLog(@"ROM loaded successfully: %s (driver index: %d)", romPath, nBurnDrvActive);
    
    return 0;
}

// Start the game
int GameMain_StartGame() {
    NSLog(@"GameMain_StartGame()");
    
    if (!g_localGameInitialized) {
        NSLog(@"Error: Game not initialized");
        return 1;
    }
    
    if (g_gameRunning) {
        NSLog(@"Game already running");
        return 0;
    }
    
    // Start the game using the game state system
    int result = GameState_StartGame();
    if (result != 0) {
        NSLog(@"Error: Failed to start game: %d", result);
        return result;
    }
    
    // Start the game loop
    result = GameMain_StartGameLoop();
    if (result != 0) {
        NSLog(@"Error: Failed to start game loop: %d", result);
        return result;
    }
    
    // Reset audio
    GameAudio_Reset();
    
    // Reset input
    GameInput_Reset();
    
    NSLog(@"Game started");
    
    return 0;
}

// Pause/unpause the game
int GameMain_PauseGame(BOOL pause) {
    NSLog(@"GameMain_PauseGame(%d)", pause);
    
    if (!g_localGameInitialized || !g_gameRunning) {
        NSLog(@"Error: Game not running");
        return 1;
    }
    
    if (g_gamePaused == pause) {
        return 0;
    }
    
    // Pause/unpause the game using the game state system
    int result = GameState_PauseGame(pause);
    if (result != 0) {
        NSLog(@"Error: Failed to %s game: %d", pause ? "pause" : "unpause", result);
        return result;
    }
    
    // Also pause/unpause the display link on macOS
    // Instead of setting paused, we need to remove/add the display link
    if (g_gameTimer) {
        if (pause) {
            [g_gameTimer invalidate];
            g_gameTimer = nil;
        } else {
            [g_gameTimer fire];
        }
    }
    
    // Also enable/disable audio
    GameAudio_SetEnabled(!pause);
    
    NSLog(@"Game %s", pause ? "paused" : "unpaused");
    
    return 0;
}

// Reset the game
int GameMain_ResetGame() {
    NSLog(@"GameMain_ResetGame()");
    
    if (!g_localGameInitialized || !g_gameRunning) {
        NSLog(@"Error: Game not running");
        return 1;
    }
    
    // Reset the game using the game state system
    int result = GameState_ResetGame();
    if (result != 0) {
        NSLog(@"Error: Failed to reset game: %d", result);
        return result;
    }
    
    // Reset audio
    GameAudio_Reset();
    
    // Reset input
    GameInput_Reset();
    
    NSLog(@"Game reset");
    
    return 0;
}

// Stop the game
int GameMain_StopGame() {
    NSLog(@"GameMain_StopGame()");
    
    if (!g_localGameInitialized || !g_gameRunning) {
        NSLog(@"Game not running");
        return 0;
    }
    
    // Stop the game loop
    GameMain_StopGameLoop();
    
    // Stop the game using the game state system
    int result = GameState_StopGame();
    if (result != 0) {
        NSLog(@"Error: Failed to stop game: %d", result);
        return result;
    }
    
    NSLog(@"Game stopped");
    
    return 0;
}

// Start the game loop
int GameMain_StartGameLoop() {
    NSLog(@"GameMain_StartGameLoop()");
    
    if (g_gameTimer) {
        NSLog(@"Game loop already running");
        return 0;
    }
    
    // Calculate frame interval based on driver's refresh rate
    float refreshRate = 60.0f; // Default to 60Hz
    
    // If the driver reports a refresh rate, use it
    if (BurnDrvGetHardwareCode() & HARDWARE_PREFIX_KONAMI) {
        // Konami games typically run at 59.17Hz
        refreshRate = 59.17f;
    } else if (BurnDrvGetHardwareCode() & HARDWARE_PREFIX_CAPCOM) {
        // CPS1/CPS2 games run at 59.61Hz
        refreshRate = 59.61f;
    }
    
    NSTimeInterval frameInterval = 1.0 / refreshRate;
    NSLog(@"Starting game loop with refresh rate: %.2f Hz (%.4f s/frame)", refreshRate, frameInterval);
    
    // Create and start the timer
    g_gameTimer = [NSTimer scheduledTimerWithTimeInterval:frameInterval
                                                   target:[DisplayLinkTarget sharedInstance]
                                                 selector:@selector(displayLinkTick:)
                                                 userInfo:nil
                                                  repeats:YES];
    
    // Add the timer to the common run loop modes to ensure it fires during user interaction
    [[NSRunLoop currentRunLoop] addTimer:g_gameTimer forMode:NSRunLoopCommonModes];
    
    return 0;
}

// Stop the game loop
int GameMain_StopGameLoop() {
    NSLog(@"GameMain_StopGameLoop()");
    
    if (g_gameTimer) {
        [g_gameTimer invalidate];
        g_gameTimer = nil;
    }
    
    return 0;
}

// Run a single frame of the game
int GameMain_RunFrame() {
    if (!g_localGameInitialized || !g_gameRunning || g_gamePaused) {
        return 0;
    }
    
    // Update input state
    GameInput_Update();
    
    // Run a frame of the game
    Metal_RunFrame(TRUE);
    
    // Count frames
    g_frameCounter++;
    g_fpsFrameCount++;
    
    // Update FPS counter every second
    NSTimeInterval currentTime = [NSDate timeIntervalSinceReferenceDate];
    if (currentTime - g_fpsLastTime >= 1.0) {
        UpdateGameFPS();
    }
    
    return 0;
}

// Get the current FPS
int GameMain_GetFPS() {
    return (int)g_fps;
}

// Should the game exit
BOOL GameMain_IsExitRequested() {
    return g_exitRequested;
}

// Set the game's volume
int GameMain_SetVolume(int volume) {
    NSLog(@"GameMain_SetVolume(%d)", volume);
    
    if (!g_localGameInitialized) {
        NSLog(@"Error: Game not initialized");
        return 1;
    }
    
    // Set the volume using the game state system
    int result = GameState_SetVolume(volume);
    if (result != 0) {
        NSLog(@"Error: Failed to set volume: %d", result);
        return result;
    }
    
    // Also set the audio system volume
    GameAudio_SetVolume(volume);
    
    NSLog(@"Volume set to %d", volume);
    
    return 0;
}

// Configure the game's controls
int GameMain_ConfigureControls() {
    NSLog(@"GameMain_ConfigureControls()");
    
    if (!g_localGameInitialized) {
        NSLog(@"Error: Game not initialized");
        return 1;
    }
    
    // Configure controls using the input system
    int result = GameInput_ConfigureControls();
    if (result != 0) {
        NSLog(@"Error: Failed to configure controls: %d", result);
        return result;
    }
    
    NSLog(@"Controls configured");
    
    return 0;
}

// Is a specific input active
bool GameMain_IsInputActive(int player, int input) {
    if (!g_localGameInitialized) {
        return false;
    }
    
    // Check the input using the input system
    return GameInput_IsButtonPressed(player, input);
}

// Update the audio buffer with new data
int GameMain_UpdateAudio(const short* data, int size) {
    if (!g_localGameInitialized) {
        return 1;
    }
    
    // Update the audio buffer using the audio system
    return GameAudio_UpdateBuffer(data, size);
}

#pragma mark - Frame Timer Callback

// Frame tick callback for the timer
void FrameTick(NSTimer *timer) {
    GameMain_RunFrame();
}

// Update the FPS counter
void UpdateGameFPS() {
    NSTimeInterval currentTime = [NSDate timeIntervalSinceReferenceDate];
    NSTimeInterval elapsed = currentTime - g_fpsLastTime;
    
    if (elapsed >= 1.0) {
        g_fps = g_fpsFrameCount / elapsed;
        g_fpsLastTime = currentTime;
        g_fpsFrameCount = 0;
    }
}

#pragma mark - Status Queries

// Is the game running?
BOOL GameMain_IsGameRunning() {
    return g_gameRunning;
}

// Is the game paused?
BOOL GameMain_IsGamePaused() {
    return g_gamePaused;
}

// Get currently loaded ROM path
const char* GameMain_GetCurrentROMPath() {
    return [g_currentROMPath UTF8String];
}

// Request exit
void GameMain_RequestExit() {
    g_exitRequested = YES;
}

// Start the game timer
extern "C" int StartGameTimer() {
    if (isTimerRunning) {
        printf("Game timer already running\n");
        return 0;
    }
    
    // Create a display link
    CVReturn result = CVDisplayLinkCreateWithActiveCGDisplays(&displayLink);
    if (result != kCVReturnSuccess) {
        printf("Failed to create display link: %d\n", result);
        return 1;
    }
    
    // Set the callback
    result = CVDisplayLinkSetOutputCallback(displayLink, GameTimerCallback, NULL);
    if (result != kCVReturnSuccess) {
        printf("Failed to set display link callback: %d\n", result);
        CVDisplayLinkRelease(displayLink);
        displayLink = NULL;
        return 2;
    }
    
    // Start the display link
    result = CVDisplayLinkStart(displayLink);
    if (result != kCVReturnSuccess) {
        printf("Failed to start display link: %d\n", result);
        CVDisplayLinkRelease(displayLink);
        displayLink = NULL;
        return 3;
    }
    
    isTimerRunning = true;
    printf("Game timer started successfully\n");
    
    return 0;
}

// Stop the game timer
extern "C" int StopGameTimer() {
    if (!isTimerRunning || !displayLink) {
        printf("Game timer not running\n");
        return 0;
    }
    
    // Stop the display link
    CVReturn result = CVDisplayLinkStop(displayLink);
    if (result != kCVReturnSuccess) {
        printf("Failed to stop display link: %d\n", result);
        return 1;
    }
    
    // Release the display link
    CVDisplayLinkRelease(displayLink);
    displayLink = NULL;
    isTimerRunning = false;
    
    printf("Game timer stopped successfully\n");
    
    return 0;
} 