#ifndef _AI_INTERFACE_H_
#define _AI_INTERFACE_H_

/*
 * Public API for FBNeo AI features on Metal backend
 * 
 * This header provides the C interface for integrating AI capabilities
 * into the FBNeo Metal implementation.
 */

#include <stdbool.h>
#include "../fixes/ai_stub_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize the AI system
 * 
 * Must be called before any other AI functions.
 * Sets up CoreML and Metal backends for AI processing.
 * 
 * @return true if initialization succeeded, false otherwise
 */
bool AI_Initialize(void);

/**
 * Load an AI model from the specified path
 * 
 * @param model_path Path to the CoreML model file
 * @return true if model loaded successfully, false otherwise
 */
bool AI_LoadModel(const char* model_path);

/**
 * Get information about the currently loaded model
 * 
 * @param info Pointer to AIModelInfo structure to fill
 * @return true if information was retrieved, false otherwise
 */
bool AI_GetModelInfo(struct AIModelInfo* info);

/**
 * Capture the current frame for AI processing
 * 
 * This function captures the current screen buffer and 
 * prepares it for AI processing.
 * 
 * @return Pointer to AIFrameData structure, or NULL on failure
 */
struct AIFrameData* AI_CaptureFrame(void);

/**
 * Process a frame with AI and generate actions
 * 
 * @param frame_data Pointer to frame data to process
 * @param actions Pointer to AIActions structure to fill with results
 * @return true if prediction succeeded, false otherwise
 */
bool AI_Predict(const struct AIFrameData* frame_data, struct AIActions* actions);

/**
 * Apply AI-generated actions to the emulator
 * 
 * @param actions Pointer to AIActions structure containing actions to apply
 * @return true if actions were applied, false otherwise
 */
bool AI_ApplyActions(const struct AIActions* actions);

/**
 * Process the current frame with AI
 * 
 * This function performs the complete AI processing pipeline:
 * 1. Capture the current frame
 * 2. Process it with AI
 * 3. Apply the resulting actions
 * 
 * It handles frame skipping according to configuration.
 */
void AI_ProcessFrame(void);

/**
 * Enable or disable AI processing
 * 
 * @param enabled Whether AI should be enabled
 */
void AI_SetEnabled(bool enabled);

/**
 * Configure the AI system
 * 
 * @param config Pointer to AIConfig structure with configuration options
 * @return true if configuration was applied, false otherwise
 */
bool AI_Configure(const struct AIConfig* config);

/**
 * Get the current AI configuration
 * 
 * @param config Pointer to AIConfig structure to fill with current configuration
 * @return true if configuration was retrieved, false otherwise
 */
bool AI_GetConfiguration(struct AIConfig* config);

/**
 * Shut down the AI system
 * 
 * Releases all resources used by the AI system.
 * No other AI functions should be called after this.
 */
void AI_Shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* _AI_INTERFACE_H_ */ 