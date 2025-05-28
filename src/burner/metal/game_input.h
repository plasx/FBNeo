#ifndef _METAL_GAME_INPUT_H_
#define _METAL_GAME_INPUT_H_

#ifdef __cplusplus
extern "C" {
#endif

// Define constants for input system
#define MAX_PLAYERS 2
#define MAX_BUTTONS 8

// Player structure
typedef struct {
    // Directional inputs
    bool up;
    bool down;
    bool left;
    bool right;
    
    // Buttons (up to 8 per player)
    bool buttons[MAX_BUTTONS];
    
    // System buttons
    bool start;
    bool coin;
} PlayerInputState;

// Overall input state
typedef struct {
    // Player inputs
    PlayerInputState players[MAX_PLAYERS];
    
    // System controls
    bool exit;
    bool menu;
    bool pause;
} InputState;

// Initialize input system
int GameInput_Init();

// Shutdown input system
int GameInput_Exit();

// Handle key presses
int GameInput_HandleKeyDown(int keyCode);
int GameInput_HandleKeyUp(int keyCode);

// Get the current input state
const InputState* GameInput_GetState();

// Check if a specific button is pressed
bool GameInput_IsButtonPressed(int player, int button);

// Configure input controls
int GameInput_ConfigureControls();

// Check if exit is requested
bool GameInput_IsExitRequested();

// Check if menu is requested
bool GameInput_IsMenuRequested();

// Get gamepad information
int GameInput_GetGamepadCount();
bool GameInput_HasActiveGamepad();
const char* GameInput_GetActiveGamepadName();

// Update input state
void GameInput_Update();

// Reset input state
void GameInput_Reset();

#ifdef __cplusplus
}
#endif 

#endif // _METAL_GAME_INPUT_H_ 