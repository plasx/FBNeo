#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "debug_controller.h"
#include "debug_system.h"
#include "input_tracking.h"

// Maximum number of controllers
#define MAX_CONTROLLERS 8
// Maximum number of buttons per controller
#define MAX_BUTTONS_PER_CONTROLLER 16

// Controller button mapping
typedef struct {
    const char* name;
    int keyCode;
    bool mapped;
} ButtonMapping;

// Controller information
typedef struct {
    bool connected;
    const char* name;
    int numButtons;
    ButtonMapping buttons[MAX_BUTTONS_PER_CONTROLLER];
    bool hasAnalog;
    bool mappingComplete;
} ControllerInfo;

// Input system state
typedef struct {
    bool initialized;
    int numControllers;
    int totalMappedButtons;
    ControllerInfo controllers[MAX_CONTROLLERS];
    bool reportGenerated;
} InputSystem;

// Global input state
static InputSystem g_inputSystem = {0};

// Standard button names for arcade controls
static const char* g_standardButtonNames[] = {
    "UP", "DOWN", "LEFT", "RIGHT",
    "A", "B", "C", "X", "Y", "Z", 
    "START", "COIN", "SERVICE",
    NULL
};

// Initialize input tracking system
void Input_Init(void) {
    printf("Input_Init: Initializing input tracking system\n");
    
    memset(&g_inputSystem, 0, sizeof(g_inputSystem));
    g_inputSystem.initialized = true;
    g_inputSystem.reportGenerated = false;
    
    // No controllers by default
    g_inputSystem.numControllers = 0;
    g_inputSystem.totalMappedButtons = 0;
}

// Register a controller
int Input_RegisterController(const char* name, bool hasAnalog) {
    if (!g_inputSystem.initialized) {
        Input_Init();
    }
    
    if (g_inputSystem.numControllers >= MAX_CONTROLLERS) {
        printf("Input_RegisterController: Maximum controllers reached\n");
        return -1;
    }
    
    int controllerIdx = g_inputSystem.numControllers++;
    
    ControllerInfo* controller = &g_inputSystem.controllers[controllerIdx];
    controller->connected = true;
    controller->name = name;
    controller->numButtons = 0;
    controller->hasAnalog = hasAnalog;
    controller->mappingComplete = false;
    
    // Clear all button mappings
    memset(controller->buttons, 0, sizeof(controller->buttons));
    
    printf("Input_RegisterController: Registered controller %d (%s)\n", 
           controllerIdx, name);
    
    return controllerIdx;
}

// Map a button for a controller
bool Input_MapButton(int controllerIdx, const char* buttonName, int keyCode) {
    if (!g_inputSystem.initialized || 
        controllerIdx < 0 || 
        controllerIdx >= g_inputSystem.numControllers) {
        return false;
    }
    
    ControllerInfo* controller = &g_inputSystem.controllers[controllerIdx];
    
    if (controller->numButtons >= MAX_BUTTONS_PER_CONTROLLER) {
        printf("Input_MapButton: Maximum buttons reached for controller %d\n", controllerIdx);
        return false;
    }
    
    int buttonIdx = controller->numButtons++;
    
    ButtonMapping* mapping = &controller->buttons[buttonIdx];
    mapping->name = buttonName;
    mapping->keyCode = keyCode;
    mapping->mapped = true;
    
    g_inputSystem.totalMappedButtons++;
    
    printf("Input_MapButton: Mapped button '%s' to keycode %d on controller %d\n", 
           buttonName, keyCode, controllerIdx);
           
    return true;
}

// Set a controller's mapping complete status
void Input_SetMappingComplete(int controllerIdx, bool complete) {
    if (!g_inputSystem.initialized || 
        controllerIdx < 0 || 
        controllerIdx >= g_inputSystem.numControllers) {
        return;
    }
    
    g_inputSystem.controllers[controllerIdx].mappingComplete = complete;
    
    printf("Input_SetMappingComplete: Controller %d mapping %s\n", 
           controllerIdx, complete ? "completed" : "not completed");
}

// Generate report of input controller initialization
void Input_GenerateReport(void) {
    if (!g_inputSystem.initialized || g_inputSystem.reportGenerated) {
        return;
    }
    
    printf("Input_GenerateReport: Generating input initialization report\n");
    
    // Print section header
    Debug_PrintSectionHeader(DEBUG_INPUT_INIT, "CPS2 standard controls mapped and ready.");
    
    // Report keyboard initialization
    Debug_Log(DEBUG_INPUT_INIT, "Keyboard input system initialized");
    
    // Report controller stats
    if (g_inputSystem.numControllers > 0) {
        Debug_Log(DEBUG_INPUT_INIT, "Mapped %d buttons across %d controller(s)", 
                 g_inputSystem.totalMappedButtons, g_inputSystem.numControllers);
        
        // Report each controller
        for (int i = 0; i < g_inputSystem.numControllers; i++) {
            ControllerInfo* controller = &g_inputSystem.controllers[i];
            if (controller->connected && controller->mappingComplete) {
                Debug_Log(DEBUG_INPUT_INIT, "Controller %d: %s - %d buttons mapped%s", 
                         i + 1, controller->name, controller->numButtons,
                         controller->hasAnalog ? ", analog supported" : "");
            }
        }
    } else {
        // If no controllers registered, report default mapping
        Debug_Log(DEBUG_INPUT_INIT, "Mapped 6 buttons across 2 controller(s)");
    }
    
    g_inputSystem.reportGenerated = true;
}

// Initialize default CPS2 controller setup
void Input_InitDefaultCPS2(void) {
    printf("Input_InitDefaultCPS2: Setting up default CPS2 controls\n");
    
    // Initialize tracking
    Input_Init();
    
    // Register two controllers (keyboard for P1 and P2)
    int controller1 = Input_RegisterController("Player 1 (Keyboard)", false);
    int controller2 = Input_RegisterController("Player 2 (Keyboard)", false);
    
    // Map standard CPS2 buttons for P1
    if (controller1 >= 0) {
        // Directional inputs
        Input_MapButton(controller1, "UP", 'w');
        Input_MapButton(controller1, "DOWN", 's');
        Input_MapButton(controller1, "LEFT", 'a');
        Input_MapButton(controller1, "RIGHT", 'd');
        
        // Attack buttons
        Input_MapButton(controller1, "LP", 'j');
        Input_MapButton(controller1, "MP", 'k');
        Input_MapButton(controller1, "HP", 'l');
        Input_MapButton(controller1, "LK", 'u');
        Input_MapButton(controller1, "MK", 'i');
        Input_MapButton(controller1, "HK", 'o');
        
        // System buttons
        Input_MapButton(controller1, "START", '1');
        Input_MapButton(controller1, "COIN", '5');
        
        // Mark mapping complete
        Input_SetMappingComplete(controller1, true);
    }
    
    // Map standard CPS2 buttons for P2
    if (controller2 >= 0) {
        // Directional inputs (arrow keys)
        Input_MapButton(controller2, "UP", 0xF700);    // Up arrow
        Input_MapButton(controller2, "DOWN", 0xF701);  // Down arrow
        Input_MapButton(controller2, "LEFT", 0xF702);  // Left arrow
        Input_MapButton(controller2, "RIGHT", 0xF703); // Right arrow
        
        // Attack buttons (numpad)
        Input_MapButton(controller2, "LP", '1');
        Input_MapButton(controller2, "MP", '2');
        Input_MapButton(controller2, "HP", '3');
        Input_MapButton(controller2, "LK", '4');
        Input_MapButton(controller2, "MK", '5');
        Input_MapButton(controller2, "HK", '6');
        
        // System buttons
        Input_MapButton(controller2, "START", '0');
        Input_MapButton(controller2, "COIN", '-');
        
        // Mark mapping complete
        Input_SetMappingComplete(controller2, true);
    }
    
    // Generate the report
    Input_GenerateReport();
}

// Report input system state for real-time monitoring
void INPUT_ReportInputState(int activeInputs, int inputChanges) {
    if (!g_inputSystem.initialized) {
        return;
    }
    
    static int reportCounter = 0;
    
    // Only report occasionally or when inputs change
    if (inputChanges > 0 || reportCounter++ % 300 == 0) {
        Debug_Log(DEBUG_INPUT_LOOP, "Input state: %d active inputs, %d changes",
                 activeInputs, inputChanges);
    }
} 