#ifndef INPUT_TRACKING_H
#define INPUT_TRACKING_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Initialize input tracking system
void Input_Init(void);

// Register a controller
int Input_RegisterController(const char* name, bool hasAnalog);

// Map a button for a controller
bool Input_MapButton(int controllerIdx, const char* buttonName, int keyCode);

// Set a controller's mapping complete status
void Input_SetMappingComplete(int controllerIdx, bool complete);

// Generate report of input controller initialization
void Input_GenerateReport(void);

// Initialize default CPS2 controller setup
void Input_InitDefaultCPS2(void);

// Report input system state for real-time monitoring
void INPUT_ReportInputState(int activeInputs, int inputChanges);

#ifdef __cplusplus
}
#endif

#endif // INPUT_TRACKING_H 