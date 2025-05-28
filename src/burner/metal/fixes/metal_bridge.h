#ifndef _METAL_BRIDGE_H_
#define _METAL_BRIDGE_H_

// =============================================================================
// FBNeo Metal - Universal Bridge Header
// =============================================================================
// This is the main header that includes all the necessary bridges for proper
// C/C++ interoperability in the FBNeo Metal port.
// Include this single header in your Metal implementation files to get access
// to all the properly bridged interfaces.
// =============================================================================

// Base interoperability definitions
#include "metal_interop.h"

// Include all the bridge interfaces
#include "metal_core_bridge.h"     // Core FBNeo functions
#include "metal_audio_bridge.h"    // Audio subsystem
#include "cps_input_bridge.h"      // CPS input system

// Include any additional platform-specific headers needed
#ifdef __OBJC__
    // Objective-C/C++ specific includes
    #import <Foundation/Foundation.h>
    #import <Cocoa/Cocoa.h>
    #import <Metal/Metal.h>
    #import <MetalKit/MetalKit.h>
#endif

// =============================================================================
// Universal Macros and Utility Functions
// =============================================================================

METAL_BEGIN_C_DECL

// -----------------------------------------------------------------------------
// Global control functions
// -----------------------------------------------------------------------------

// Initialize all systems
INT32 Metal_Initialize();

// Shut down all systems
void Metal_Shutdown();

// Process a single frame
INT32 Metal_ProcessFrame();

// Pause/resume emulation
void Metal_Pause(INT32 pauseState);

// Check if Metal backend is active/initialized
INT32 Metal_IsActive();

// -----------------------------------------------------------------------------
// Error reporting
// -----------------------------------------------------------------------------

// Report an error from C code
void Metal_ReportError(const char* message);

// Log a debug message from C code
void Metal_LogDebug(const char* format, ...);

METAL_END_C_DECL

#endif // _METAL_BRIDGE_H_ 