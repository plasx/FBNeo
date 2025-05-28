#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Game renderer interface
// This connects the FBNeo core's rendering to our Metal implementation

// Initialize the game renderer
int GameRenderer_Init(int width, int height, int bpp);

// Shutdown the game renderer
int GameRenderer_Exit();

// Render a frame of the game
int GameRenderer_RenderFrame();

// Set whether to use the core's rendering or our test pattern
void GameRenderer_SetUseCoreRendering(bool useCoreRendering);

// Resize the game rendering surface
int GameRenderer_Resize(int width, int height);

// Start the game rendering
int GameRenderer_Start();

// Stop the game rendering
int GameRenderer_Stop();

// Is the game currently rendering
bool GameRenderer_IsRunning();

#ifdef __cplusplus
}
#endif 