#ifndef HARDWARE_TRACKING_H
#define HARDWARE_TRACKING_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Initialize hardware tracking system
void Hardware_Init(void);

// Register a hardware component to track
int Hardware_RegisterComponent(const char* name, const char* details);

// Set initialization status for a component
void Hardware_SetInitialized(int componentIndex, bool success);

// Generate a report of hardware initialization
void Hardware_GenerateReport(void);

// Initialize hardware components for emulation
void Hardware_InitComponents(void);

#ifdef __cplusplus
}
#endif

#endif // HARDWARE_TRACKING_H 