#ifndef METAL_APP_INTEGRATION_H
#define METAL_APP_INTEGRATION_H

#ifdef __cplusplus
extern "C" {
#endif

// Function to setup input handling for a Metal window and view
void MetalApp_SetupInputForWindowAndView(void* windowPtr, void* viewPtr);

// Function to clean up input handling
void MetalApp_CleanupInput();

#ifdef __cplusplus
}
#endif

#endif // METAL_APP_INTEGRATION_H 