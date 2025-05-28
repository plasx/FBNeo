#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "ai_stub_types.h"
#include "../metal_declarations.h"
#include <stdlib.h>
#include "c_cpp_compatibility.h"

// Forward declaration - we only need this one, remove others
struct AIModelInfo;  // Forward declaration

// Forward declarations for internal functions
struct AIFrameData* AI_CaptureFrame(void);
bool AI_Predict(const struct AIFrameData* frame_data, struct AIActions* actions);
bool AI_LoadModelFile(const char* model_path);

/*
 * This file contains stub implementations for AI functionality
 * to be used during the Metal build process until the full
 * implementation is complete.
 */

// Global variables
static bool g_bAIInitialized = false;
static bool g_bAIActive = false;
static bool g_bAITraining = false;
static int g_nControlledPlayer = 0;
static int g_nDifficulty = 2;
static bool g_bDebugOverlay = false;

// AI configuration
static struct AIConfig ai_config = {
    .enabled = false,
    .frame_skip = 0,
    .confidence_threshold = 0.5f,
    .model_path = "",
    .visualization = false,
    .debug_mode = false
};

// AI frame data
static struct AIFrameData frame_data = {
    .data = NULL,
    .width = 0,
    .height = 0,
    .channels = 4, // RGBA
    .size = 0
};

// AI actions
static struct AIActions actions = {
    .action_count = 0
};

// AI model info
static struct AIModelInfo model_info = {
    .name = "None",
    .version = "0.0",
    .game_id = "unknown",
    .is_game_specific = false,
    .input_width = 0,
    .input_height = 0
};

// Function to convert AIActions to C_AIOutputAction
static C_AIOutputAction ConvertActionsToOutputAction(struct AIActions* actions) {
    C_AIOutputAction output = {0};
    output.player = g_nControlledPlayer;
    
    // In a real implementation, this would map AI actions to game inputs
    // For now, just return a dummy action
    output.button_press = 0;
    output.button_release = 0;
    output.joystick = 0;
    output.confidence = 0.0f;
    
    return output;
}

// Initialize AI subsystem
int AI_Init(const char* configPath) {
    if (g_bAIInitialized) {
        printf("AI is already initialized\n");
        return 1;
    }
    
    printf("AI_Init: Initializing AI subsystem with config: %s\n", 
           configPath ? configPath : "default");
    
    // Set default configuration
    ai_config.enabled = false;
    ai_config.frame_skip = 0;
    ai_config.confidence_threshold = 0.5f;
    ai_config.visualization = false;
    ai_config.debug_mode = false;
    
    // In a real implementation, this would:
    // 1. Parse the configuration file
    // 2. Initialize AI framework
    // 3. Set up data pipelines
    
    g_bAIInitialized = true;
    return 0;
}

// Shutdown AI subsystem
void AI_Exit() {
    if (!g_bAIInitialized) {
        return;
    }
    
    printf("AI_Exit: Shutting down AI subsystem\n");
    
    // Free any resources
    if (frame_data.data) {
        free(frame_data.data);
        frame_data.data = NULL;
    }
    
    g_bAIInitialized = false;
    g_bAIActive = false;
}

// Enable/disable AI
void AI_SetActive(int enable) {
    g_bAIActive = (enable != 0);
    printf("AI_SetActive: AI is now %s\n", g_bAIActive ? "active" : "inactive");
}

// Check if AI is active
int AI_IsActive() {
    return g_bAIActive ? 1 : 0;
}

// Enable/disable training mode
void AI_SetTraining(int enable) {
    g_bAITraining = (enable != 0);
    printf("AI_SetTraining: Training mode is now %s\n", 
           g_bAITraining ? "enabled" : "disabled");
}

// Check if training mode is active
int AI_IsTraining() {
    return g_bAITraining ? 1 : 0;
}

// Legacy AI initialization (kept for compatibility)
void AI_Initialize() {
    AI_Init(NULL);
}

// Legacy AI shutdown (kept for compatibility)
void AI_Shutdown() {
    AI_Exit();
}

// Load an AI model
void AI_LoadModel(const char* modelPath) {
    if (!g_bAIInitialized) {
        printf("AI_LoadModel: AI is not initialized\n");
        return;
    }
    
    printf("AI_LoadModel: Loading model from %s\n", 
           modelPath ? modelPath : "default");
    
    // In a real implementation, this would:
    // 1. Load the model file
    // 2. Initialize the model
    // 3. Validate model compatibility
    
    if (modelPath) {
        strncpy(ai_config.model_path, modelPath, sizeof(ai_config.model_path) - 1);
    }
    
    // Pretend we loaded the model successfully
    g_bAIActive = true;
}

// Process a game state frame
C_AIOutputAction AI_ProcessFrame(void* gameState, int frameNumber) {
    // This stub implementation just returns a dummy action
    C_AIOutputAction output = {0};
    
    if (!g_bAIInitialized || !g_bAIActive) {
        return output;
    }
    
    // In a real implementation, this would:
    // 1. Convert game state to AI input format
    // 2. Run the AI model
    // 3. Convert AI output to game actions
    
    // For now, just return a dummy action
    output.player = g_nControlledPlayer;
    output.button_press = frameNumber % 60 == 0 ? 1 : 0; // Press button every 60 frames
    output.confidence = 0.9f;
    
    return output;
}

// Set which player the AI controls
void AI_SetControlledPlayer(int playerIndex) {
    g_nControlledPlayer = playerIndex;
    printf("AI_SetControlledPlayer: AI now controls player %d\n", playerIndex);
}

// Set AI difficulty level
void AI_SetDifficulty(int level) {
    g_nDifficulty = level;
    printf("AI_SetDifficulty: Difficulty set to %d\n", level);
}

// Enable/disable training mode (alternative function)
void AI_EnableTrainingMode(int enable) {
    AI_SetTraining(enable);
}

// Enable/disable debug overlay
void AI_EnableDebugOverlay(int enable) {
    g_bDebugOverlay = (enable != 0);
    printf("AI_EnableDebugOverlay: Debug overlay is now %s\n", 
           g_bDebugOverlay ? "enabled" : "disabled");
}

// Save frame data for external analysis
void AI_SaveFrameData(const char* filename) {
    if (!frame_data.data || frame_data.size == 0) {
        printf("AI_SaveFrameData: No frame data available\n");
        return;
    }
    
    printf("AI_SaveFrameData: Saving frame data to %s\n", filename);
    
    // In a real implementation, this would save the frame data to a file
    FILE* file = fopen(filename, "wb");
    if (file) {
        fwrite(frame_data.data, 1, frame_data.size, file);
        fclose(file);
        printf("Saved %u bytes of frame data\n", frame_data.size);
    } else {
        printf("Failed to open file for writing\n");
    }
}

// Configure game memory mapping for AI
void AI_ConfigureGameMemoryMapping(int gameType, const char* gameId) {
    printf("AI_ConfigureGameMemoryMapping: Game type %d, ID %s\n", 
           gameType, gameId ? gameId : "unknown");
    
    // In a real implementation, this would set up memory mapping for the AI
}

// Get game observation for AI
void* AI_GetGameObservation() {
    // In a real implementation, this would return a pointer to the game state
    return NULL;
}

// Process frame buffer data
void AI_ProcessFrameBuffer(const void* data, int width, int height, int pitch) {
    if (!g_bAIInitialized || !g_bAIActive || !data) {
        return;
    }
    
    // Reallocate frame buffer if dimensions changed
    size_t new_size = width * height * 4; // Assume RGBA
    if (frame_data.width != width || frame_data.height != height || frame_data.size != new_size) {
        if (frame_data.data) {
            free(frame_data.data);
        }
        
        frame_data.data = malloc(new_size);
        if (!frame_data.data) {
            printf("AI_ProcessFrameBuffer: Failed to allocate memory\n");
            frame_data.size = 0;
            return;
        }
        
        frame_data.width = width;
        frame_data.height = height;
        frame_data.size = new_size;
        printf("AI_ProcessFrameBuffer: Resized frame buffer to %dx%d (%u bytes)\n", 
               width, height, (unsigned int)new_size);
    }
    
    // Copy frame data
    // In a real implementation, this would handle different pixel formats
    // For simplicity, assume source is RGBA with pitch = width * 4
    if (frame_data.data) {
        memcpy(frame_data.data, data, new_size);
    }
}

// Start an AI play session
void AI_StartSession() {
    printf("AI_StartSession: Starting new AI play session\n");
}

// End an AI play session
float AI_EndSession(int success) {
    printf("AI_EndSession: Ending session (success: %d)\n", success);
    return success ? 1.0f : 0.0f;
}

// Save AI state
int AI_SaveState(const char* path) {
    printf("AI_SaveState: Saving state to %s\n", path);
    return 1; // Success
}

// Load AI state
int AI_LoadState(const char* path) {
    printf("AI_LoadState: Loading state from %s\n", path);
    return 1; // Success
}

// Get confidence level of current action
float AI_GetCurrentActionConfidence() {
    // In a real implementation, this would return the confidence of the current action
    return 0.9f;
}

// Get current state value estimation
float AI_GetStateValue() {
    // In a real implementation, this would return the estimated value of the current state
    return 0.5f;
}

// Get number of top actions
int AI_GetTopActionCount() {
    // In a real implementation, this would return the number of actions with high confidence
    return 1;
}

// Get information about a top action
void AI_GetTopActionInfo(int index, char* actionName, float* confidence) {
    if (actionName) {
        strcpy(actionName, "Button1");
    }
    
    if (confidence) {
        *confidence = 0.9f;
    }
}

// CoreML stub functions
bool CoreML_Initialize() {
    printf("CoreML_Initialize: Initializing CoreML\n");
    return true;
}

void CoreML_Shutdown() {
    printf("CoreML_Shutdown: Shutting down CoreML\n");
}

bool CoreML_LoadModel(const char* path) {
    printf("CoreML_LoadModel: Loading model from %s\n", path ? path : "default");
    return true;
}

bool CoreML_FindDefaultModels() {
    printf("CoreML_FindDefaultModels: Looking for default models\n");
    return true;
}

bool CoreML_GetModelInfo(struct AIModelInfo* info) {
    if (!info) {
        return false;
    }
    
    // Fill with dummy info
    strcpy(info->name, "StubModel");
    strcpy(info->version, "1.0");
    strcpy(info->game_id, "generic");
    info->is_game_specific = false;
    info->input_width = 320;
    info->input_height = 240;
    
    return true;
}

bool CoreML_Predict(const void* frameData, struct AIActions* actions) {
    if (!actions) {
        return false;
    }
    
    // Fill with dummy actions
    actions->action_count = 1;
    
    // Set up a dummy action
    for (int i = 0; i < actions->action_count && i < MAX_ACTION_COUNT; i++) {
        actions->actions[i].type = AI_ACTION_BUTTON;
        actions->actions[i].input_id = i;
        actions->actions[i].value = 1.0f;
    }
    
    return true;
}

// Rename this function to avoid conflict with metal_declarations.h
void AI_ProcessFrame_Internal(void) {
    if (!g_bAIInitialized || !g_bAIActive) {
        return;
    }
    
    static int frame_count = 0;
    frame_count++;
    
    // Skip frames if configured
    if (ai_config.frame_skip > 0 && frame_count % (ai_config.frame_skip + 1) != 0) {
        return;
    }
    
    // In a real implementation, this would:
    // 1. Get the current frame data
    // 2. Process it through the AI model
    // 3. Apply the resulting actions to the game
    
    // For debugging only
    if (frame_count % 60 == 0) {
        printf("AI frame %d processed\n", frame_count);
    }
    
    // Get frame dimensions
    int width = Metal_GetFrameWidth();
    int height = Metal_GetFrameHeight();
    
    if (width <= 0 || height <= 0) {
        // Use defaults if dimensions are invalid
        width = 320;
        height = 240;
    }
    
    // Predict actions based on frame data
    struct AIActions predicted_actions = {0};
    if (CoreML_Predict(frame_data.data, &predicted_actions)) {
        // Apply confidence threshold
        for (unsigned int i = 0; i < predicted_actions.action_count && i < MAX_ACTION_COUNT; i++) {
            if (predicted_actions.actions[i].value < ai_config.confidence_threshold) {
                predicted_actions.actions[i].value = 0;
            }
        }
        
        // Apply the actions to the game
        if (ai_config.debug_mode) {
            for (unsigned int i = 0; i < predicted_actions.action_count && i < MAX_ACTION_COUNT; i++) {
                if (predicted_actions.actions[i].value > 0) {
                    printf("Action %u: type=%d, input=%u, value=%.2f\n", 
                           i, predicted_actions.actions[i].type, 
                           predicted_actions.actions[i].input_id,
                           predicted_actions.actions[i].value);
                }
            }
        }
    }
}

// Main AI processing function - called each frame (rename to avoid conflict)
void AI_Process_Frame_Main(void) {
    static int frame_skip_counter = 0;
    
    // If AI is not enabled, do nothing
    if (!ai_config.enabled || !g_bAIInitialized) {
        return;
    }
    
    // Skip frames if needed
    frame_skip_counter++;
    if (frame_skip_counter < ai_config.frame_skip) {
        return;
    }
    frame_skip_counter = 0;
    
    // Capture the current frame
    struct AIFrameData* frame = AI_CaptureFrame();
    if (!frame) {
        printf("AI_ProcessFrame: Failed to capture frame\n");
        return;
    }
    
    // Get AI predictions
    struct AIActions predicted_actions;
    if (!AI_Predict(frame, &predicted_actions)) {
        printf("AI_ProcessFrame: Prediction failed\n");
        return;
    }
    
    // Apply actions to the game if AI control is active
    if (AI_IsActive()) {
        if (!AI_ApplyActions(&predicted_actions)) {
            printf("AI_ProcessFrame: Failed to apply actions\n");
        }
    }
    
    // If in training mode, record the frame and actions
    if (AI_IsTraining()) {
        // This would save the frame and actions for training in a real implementation
        printf("AI training mode - would save frame and actions\n");
    }
}

// Enable or disable AI system
void AI_SetEnabled(bool enabled) {
    printf("AI_SetEnabled: %s AI system\n", enabled ? "Enabling" : "Disabling");
    ai_config.enabled = enabled;
    AI_SetActive(enabled ? 1 : 0);
}

// Configure AI system with new settings
bool AI_Configure(const struct AIConfig* config) {
    if (!config) {
        return false;
    }
    
    printf("AI_Configure: Updating AI configuration\n");
    memcpy(&ai_config, config, sizeof(struct AIConfig));
    
    return true;
}

// Get current AI configuration
bool AI_GetConfiguration(struct AIConfig* config) {
    if (!config) {
        return false;
    }
    
    memcpy(config, &ai_config, sizeof(struct AIConfig));
    return true;
}

// Shutdown AI system
void AI_ShutdownSystem(void) {
    printf("AI_ShutdownSystem: Shutting down AI system\n");
    
    if (g_bAIInitialized) {
        AI_Exit();
        g_bAIInitialized = false;
    }
}

// Connection to Metal rendering for visualization
void AI_UpdateVisualization(uint8_t* overlay_buffer, int width, int height, int pitch) {
    if (!overlay_buffer || !ai_config.visualization) {
        return;
    }
    
    // In a real implementation, this would draw visualization data onto the overlay buffer
    // For now, just draw a simple pattern to indicate the overlay is active
    
    // Draw a border around the edges
    const uint32_t border_color = 0xFF00FF00; // Green
    const int border_width = 4;
    
    // Top and bottom borders
    for (int y = 0; y < border_width; y++) {
        for (int x = 0; x < width; x++) {
            int top_pos = y * pitch;
            int bottom_pos = (height - 1 - y) * pitch;
            
            // Write RGBA values (assuming 4 bytes per pixel)
            *((uint32_t*)(overlay_buffer + top_pos + x * 4)) = border_color;
            *((uint32_t*)(overlay_buffer + bottom_pos + x * 4)) = border_color;
        }
    }
    
    // Left and right borders
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < border_width; x++) {
            int left_pos = y * pitch;
            int right_pos = y * pitch + (width - 1 - x) * 4;
            
            *((uint32_t*)(overlay_buffer + left_pos + x * 4)) = border_color;
            *((uint32_t*)(overlay_buffer + right_pos)) = border_color;
        }
    }
    
    // Add text showing AI status at the top
    const char* text = "AI ACTIVE";
    const int text_x = 20;
    const int text_y = 20;
    const uint32_t text_color = 0xFFFFFFFF; // White
    
    // Simple font rendering - just squares for now
    for (int i = 0; i < strlen(text); i++) {
        int x = text_x + i * 10;
        
        for (int j = 0; j < 8; j++) {
            for (int k = 0; k < 8; k++) {
                int pos = (text_y + j) * pitch + (x + k) * 4;
                *((uint32_t*)(overlay_buffer + pos)) = text_color;
            }
        }
    }
}

// Reward function for reinforcement learning
float AI_CalculateReward(void* gameState, int gameType) {
    static float last_score = 0.0f;
    float reward = 0.0f;
    
    // If no game state, return zero reward
    if (!gameState) {
        return 0.0f;
    }
    
    // Calculate reward based on game type
    switch (gameType) {
        case 1: // Fighting games
            {
                // Get player health and opponent health from memory
                uint8_t p1_health = ((uint8_t*)gameState)[0x100];
                uint8_t p2_health = ((uint8_t*)gameState)[0x200];
                
                // Reward = gain in health difference
                float health_diff = (float)(p1_health - p2_health);
                static float last_health_diff = 0.0f;
                reward = health_diff - last_health_diff;
                last_health_diff = health_diff;
            }
            break;
            
        case 2: // Shmups
            {
                // Get score from memory
                uint32_t score = *(uint32_t*)(((uint8_t*)gameState) + 0x108);
                
                // Reward = score change
                reward = (float)(score - last_score) * 0.01f;
                last_score = (float)score;
                
                // Additional reward for not dying
                uint8_t lives = ((uint8_t*)gameState)[0x104];
                static uint8_t last_lives = 0;
                if (lives < last_lives) {
                    reward -= 10.0f; // Penalty for dying
                }
                last_lives = lives;
            }
            break;
            
        case 3: // Platformers
            {
                // Get score and position
                uint32_t score = *(uint32_t*)(((uint8_t*)gameState) + 0x108);
                uint16_t x_pos = *(uint16_t*)(((uint8_t*)gameState) + 0x100);
                static uint16_t last_x_pos = 0;
                
                // Reward = score change + moving forward
                reward = (float)(score - last_score) * 0.01f;
                reward += (x_pos > last_x_pos) ? 0.1f : -0.05f;
                
                last_score = (float)score;
                last_x_pos = x_pos;
            }
            break;
            
        default:
            reward = 0.0f;
            break;
    }
    
    return reward;
}

// Function to check if a specific game has AI support
bool AI_IsGameSupported(const char* gameId) {
    // List of games with specific AI support
    static const char* supported_games[] = {
        "mshvsf",    // Marvel Super Heroes vs. Street Fighter
        "sfa3",      // Street Fighter Alpha 3
        "dstlk",     // Darkstalkers
        "ssf2t",     // Super Street Fighter II Turbo
        "1944",      // 1944: The Loop Master
        "progear",   // Progear
        "mslug",     // Metal Slug
        "mslug2",    // Metal Slug 2
        "mslug3",    // Metal Slug 3
        NULL         // End of list
    };
    
    // Check if game is in supported list
    for (int i = 0; supported_games[i] != NULL; i++) {
        if (strcmp(gameId, supported_games[i]) == 0) {
            return true;
        }
    }
    
    return false;
}

// Metal integration for the AI renderer
void AI_UpdateRenderer(void* mtlTexture, int width, int height) {
    // This would update the Metal texture with AI visualization data
    // In a real implementation, this would use Metal specific APIs
    
    if (!mtlTexture || !ai_config.visualization) {
        return;
    }
    
    printf("AI_UpdateRenderer: Would update Metal texture %p (%dx%d) with visualization data\n", 
           mtlTexture, width, height);
}

// Perform intrinsic curiosity module calculation (ICM)
float AI_CalculateIntrinsicReward(const struct AIFrameData* current_frame, 
                                 const struct AIFrameData* next_frame,
                                 const struct AIActions* actions) {
    // This would calculate intrinsic reward based on prediction error
    // For now, return a dummy value
    return 0.1f;
}

// Get the AI's explanation for its current decision
bool AI_GetDecisionExplanation(char* explanation, int maxLength) {
    if (!explanation || maxLength <= 0 || !g_bAIInitialized) {
        return false;
    }
    
    // Get top action information
    int action_count = AI_GetTopActionCount();
    if (action_count <= 0) {
        strcpy(explanation, "No action decision available");
        return false;
    }
    
    // Get the top action
    char action_name[64];
    float confidence;
    AI_GetTopActionInfo(0, action_name, &confidence);
    
    // Create explanation
    snprintf(explanation, maxLength, "Selected action: %s (confidence: %.2f)", 
             action_name, confidence);
    
    return true;
}

// Neural network inference functions
/**
 * Performs neural network inference using CoreML
 * @param frameData Pointer to the frame data
 * @param modelPath Path to the CoreML model
 * @param results Pointer to store the inference results
 * @return true if inference was successful, false otherwise
 */
bool AI_PerformCoreMLInference(const struct AIFrameData* frameData, const char* modelPath, float* results) {
    if (!g_bAIInitialized || !frameData || !results) {
        return false;
    }
    
    printf("AI_PerformCoreMLInference: Running inference on frame %dx%d with model %s\n", 
           frameData->width, frameData->height, modelPath);
    
    // This would call into the CoreML subsystem
    // For now, generate some random values for testing
    for (int i = 0; i < 16; i++) { // Assume 16 output values
        results[i] = ((float)rand() / RAND_MAX);
    }
    
    return true;
}

/**
 * Performs batch processing on multiple frames
 * @param frames Array of frame data pointers
 * @param frameCount Number of frames in the batch
 * @param results Pointer to store the batch inference results
 * @return true if batch processing was successful, false otherwise
 */
bool AI_ProcessFrameBatch(const struct AIFrameData** frames, int frameCount, float** results) {
    if (!g_bAIInitialized || !frames || !results || frameCount <= 0) {
        return false;
    }
    
    printf("AI_ProcessFrameBatch: Processing batch of %d frames\n", frameCount);
    
    // Process each frame individually
    for (int i = 0; i < frameCount; i++) {
        if (frames[i]) {
            // Generate random results for testing
            for (int j = 0; j < 16; j++) {
                results[i][j] = ((float)rand() / RAND_MAX);
            }
        }
    }
    
    return true;
}

/**
 * Normalizes image frame data to prepare for neural network processing
 * @param frameData Pointer to the source frame data
 * @param normalizedData Pointer to the destination for normalized data
 * @param mean Array of mean values for normalization (one per channel)
 * @param stdDev Array of standard deviation values (one per channel)
 * @return true if normalization was successful, false otherwise
 */
bool AI_NormalizeFrameData(const struct AIFrameData* frameData, float* normalizedData, 
                          const float* mean, const float* stdDev) {
    if (!g_bAIInitialized || !frameData || !normalizedData || !mean || !stdDev) {
        return false;
    }
    
    // Calculate total pixels
    int totalPixels = frameData->width * frameData->height;
    
    // Normalize each pixel (assuming RGBA format)
    for (int i = 0; i < totalPixels; i++) {
        for (int c = 0; c < frameData->channels; c++) {
            int idx = i * frameData->channels + c;
            if (c < 3) { // Only normalize RGB channels, not alpha
                float pixelValue = frameData->data[idx] / 255.0f; // Convert to 0-1 range
                normalizedData[i * 3 + c] = (pixelValue - mean[c]) / stdDev[c];
            }
        }
    }
    
    return true;
}

/**
 * Registers a callback function to be called when new AI predictions are available
 * @param callback The callback function
 * @param userData User data to be passed to the callback
 * @return true if registration was successful, false otherwise
 */
bool AI_RegisterPredictionCallback(void (*callback)(const struct AIActions*, void*), void* userData) {
    if (!g_bAIInitialized || !callback) {
        return false;
    }
    
    // In a real implementation, we would store the callback and userData
    printf("AI_RegisterPredictionCallback: Registered prediction callback %p with user data %p\n", 
           (void*)callback, userData);
    
    return true;
}

/**
 * Configures the AI system for a specific game
 * @param gameId Unique identifier for the game
 * @param configPath Path to the game-specific configuration file
 * @return true if configuration was successful, false otherwise
 */
bool AI_ConfigureForGame(const char* gameId, const char* configPath) {
    if (!g_bAIInitialized || !gameId) {
        return false;
    }
    
    printf("AI_ConfigureForGame: Configuring AI for game %s with config %s\n", 
           gameId, configPath ? configPath : "default");
    
    // Check if game is supported
    if (!AI_IsGameSupported(gameId)) {
        printf("Game %s is not supported by the AI system\n", gameId);
        return false;
    }
    
    // Get game genre for memory mapping
    int gameGenre = 0; // Default: unknown
    
    // Determine game genre based on game ID
    if (strstr(gameId, "sf") || strstr(gameId, "fighter") || strstr(gameId, "vs")) {
        gameGenre = 1; // Fighting game
    } else if (strstr(gameId, "shoot") || strstr(gameId, "gun") || strstr(gameId, "19") || 
               strstr(gameId, "darius") || strstr(gameId, "gradius")) {
        gameGenre = 2; // Shoot-em-up
    } else if (strstr(gameId, "mario") || strstr(gameId, "sonic") || 
               strstr(gameId, "ghost") || strstr(gameId, "platform")) {
        gameGenre = 3; // Platformer
    } else if (strstr(gameId, "puzzle") || strstr(gameId, "tetris") || 
               strstr(gameId, "columns") || strstr(gameId, "puyo")) {
        gameGenre = 4; // Puzzle
    }
    
    // Configure memory mapping for game
    AI_ConfigureGameMemoryMapping(gameGenre, gameId);
    
    // Try to load game-specific model if available
    char modelPath[256];
    snprintf(modelPath, sizeof(modelPath), "models/%s/model.mlmodel", gameId);
    if (AI_LoadModelFile(modelPath)) {
        printf("Loaded game-specific model for %s\n", gameId);
    } else {
        printf("No game-specific model found for %s, using default model\n", gameId);
        // Try to load default model for the genre
        snprintf(modelPath, sizeof(modelPath), "models/genre_%d/model.mlmodel", gameGenre);
        if (AI_LoadModelFile(modelPath)) {
            printf("Loaded genre-specific model for genre %d\n", gameGenre);
        } else {
            printf("No genre-specific model found, using base model\n");
            // Load base model
            AI_LoadModelFile("models/base_model.mlmodel");
        }
    }
    
    return true;
}

/**
 * Initializes the tensor computation system
 * @return true if initialization was successful, false otherwise
 */
bool AI_InitializeTensorSystem(void) {
    if (!g_bAIInitialized) {
        return false;
    }
    
    printf("AI_InitializeTensorSystem: Initializing tensor computation system\n");
    
    // In a real implementation, this would initialize the tensor computation backend
    // (Metal Performance Shaders, etc.)
    
    return true;
}

/**
 * Performs a tensor operation on the given input tensors
 * @param operation The operation to perform (0=add, 1=multiply, 2=convolution, etc.)
 * @param inputs Array of input tensors
 * @param inputCount Number of input tensors
 * @param output Output tensor
 * @return true if the operation was successful, false otherwise
 */
bool AI_PerformTensorOperation(int operation, const float** inputs, int inputCount, float* output, 
                             const int* dimensions, int dimensionCount) {
    if (!g_bAIInitialized || !inputs || inputCount <= 0 || !output || !dimensions || dimensionCount <= 0) {
        return false;
    }
    
    printf("AI_PerformTensorOperation: Performing tensor operation %d with %d inputs\n", 
           operation, inputCount);
    
    // Calculate total elements in the tensor
    int totalElements = 1;
    for (int i = 0; i < dimensionCount; i++) {
        totalElements *= dimensions[i];
    }
    
    // Perform operation based on type
    switch (operation) {
        case 0: // Addition
            if (inputCount >= 2) {
                for (int i = 0; i < totalElements; i++) {
                    output[i] = inputs[0][i] + inputs[1][i];
                    for (int j = 2; j < inputCount; j++) {
                        output[i] += inputs[j][i];
                    }
                }
            }
            break;
            
        case 1: // Multiplication
            if (inputCount >= 2) {
                for (int i = 0; i < totalElements; i++) {
                    output[i] = inputs[0][i] * inputs[1][i];
                    for (int j = 2; j < inputCount; j++) {
                        output[i] *= inputs[j][i];
                    }
                }
            }
            break;
            
        case 2: // Convolution (simplified)
            // A real implementation would perform proper convolution
            // This is just a placeholder
            memset(output, 0, totalElements * sizeof(float));
            break;
            
        default:
            printf("Unknown tensor operation: %d\n", operation);
            return false;
    }
    
    return true;
}

/**
 * Enables or disables on-screen visualization of AI processing
 * @param visualizationType Type of visualization (0=none, 1=heatmap, 2=attention, etc.)
 * @param opacity Opacity of the visualization (0.0-1.0)
 * @return true if visualization settings were updated successfully, false otherwise
 */
bool AI_SetVisualization(int visualizationType, float opacity) {
    if (!g_bAIInitialized) {
        return false;
    }
    
    printf("AI_SetVisualization: Setting visualization type %d with opacity %.2f\n", 
           visualizationType, opacity);
    
    // Update config
    ai_config.visualization = (visualizationType > 0);
    
    // Additional visualization parameters would be stored in a real implementation
    
    return true;
}

/**
 * Sets up reinforcement learning for the AI system
 * @param algorithm Reinforcement learning algorithm (0=PPO, 1=A3C, 2=DQN, etc.)
 * @param paramPath Path to algorithm hyperparameters
 * @return true if reinforcement learning was set up successfully, false otherwise
 */
bool AI_SetupReinforcementLearning(int algorithm, const char* paramPath) {
    if (!g_bAIInitialized) {
        return false;
    }
    
    printf("AI_SetupReinforcementLearning: Setting up RL algorithm %d with params %s\n", 
           algorithm, paramPath ? paramPath : "default");
    
    // This would set up the reinforcement learning algorithm in a real implementation
    
    return true;
}

/**
 * Creates a custom reward function for reinforcement learning
 * @param gameId Game identifier
 * @param rewardFunction Custom reward function (0=score-based, 1=progress-based, 2=survival-based, etc.)
 * @return true if the reward function was created successfully, false otherwise
 */
bool AI_CreateRewardFunction(const char* gameId, int rewardFunction) {
    if (!g_bAIInitialized || !gameId) {
        return false;
    }
    
    printf("AI_CreateRewardFunction: Creating reward function %d for game %s\n", 
           rewardFunction, gameId);
    
    // This would create a custom reward function in a real implementation
    
    return true;
}

/**
 * Gets metrics about AI performance
 * @param metrics Pointer to store metrics data
 * @return true if metrics were retrieved successfully, false otherwise
 */
bool AI_GetPerformanceMetrics(struct AIPerformanceMetrics* metrics) {
    if (!g_bAIInitialized || !metrics) {
        return false;
    }
    
    // Fill in some sample metrics
    metrics->inferenceTimeMs = 5.3f;
    metrics->modelSizeBytes = 1024 * 1024 * 8; // 8MB
    metrics->memoryUsageKb = 1024 * 8; // 8MB
    metrics->framesPerSecond = 60;
    metrics->predictionAccuracy = 0.75f;
    
    return true;
}

// Helper function to convert AI predictions to FBNeo input format
int AI_ConvertPredictionToInput(const struct AIActions* actions) {
    if (!g_bAIInitialized || !actions) {
        return 0;
    }
    
    // Default input (no buttons pressed)
    int input = 0;
    
    // Convert AI actions to input bitmask
    for (int i = 0; i < MAX_ACTION_COUNT; i++) {
        if (actions->actions[i].active && actions->actions[i].confidence > ai_config.confidence_threshold) {
            // Map action to input based on action name
            if (strcmp(actions->actions[i].name, "UP") == 0) {
                input |= 0x01;
            } else if (strcmp(actions->actions[i].name, "DOWN") == 0) {
                input |= 0x02;
            } else if (strcmp(actions->actions[i].name, "LEFT") == 0) {
                input |= 0x04;
            } else if (strcmp(actions->actions[i].name, "RIGHT") == 0) {
                input |= 0x08;
            } else if (strcmp(actions->actions[i].name, "BUTTON1") == 0) {
                input |= 0x10;
            } else if (strcmp(actions->actions[i].name, "BUTTON2") == 0) {
                input |= 0x20;
            } else if (strcmp(actions->actions[i].name, "BUTTON3") == 0) {
                input |= 0x40;
            } else if (strcmp(actions->actions[i].name, "BUTTON4") == 0) {
                input |= 0x80;
            } else if (strcmp(actions->actions[i].name, "BUTTON5") == 0) {
                input |= 0x100;
            } else if (strcmp(actions->actions[i].name, "BUTTON6") == 0) {
                input |= 0x200;
            }
        }
    }
    
    return input;
}

// Metal integration - these functions would be implemented in metal_ai_module.cpp
// We're providing stubs here for the C interface
extern int Metal_AI_Initialize();
extern void Metal_AI_Shutdown();
extern int Metal_AI_ProcessFrame(void* frameData, int width, int height, int pitch);
extern int Metal_AI_RenderOverlay(void* overlayData, int width, int height, int pitch);

/**
 * Initializes the Metal AI integration
 * @return true if initialization was successful, false otherwise
 */
bool AI_InitializeMetal(void) {
    if (!g_bAIInitialized) {
        return false;
    }
    
    printf("AI_InitializeMetal: Initializing Metal AI integration\n");
    
    // Call Metal-specific initialization
    return (Metal_AI_Initialize() == 0);
}

/**
 * Shuts down the Metal AI integration
 */
void AI_ShutdownMetal(void) {
    if (!g_bAIInitialized) {
        return;
    }
    
    printf("AI_ShutdownMetal: Shutting down Metal AI integration\n");
    
    // Call Metal-specific shutdown
    Metal_AI_Shutdown();
}

/**
 * Processes a frame using Metal AI module
 * @param frameData Pointer to the frame data
 * @param width Frame width
 * @param height Frame height
 * @param pitch Frame pitch (bytes per row)
 * @return true if processing was successful, false otherwise
 */
bool AI_ProcessFrameMetal(void* frameData, int width, int height, int pitch) {
    if (!g_bAIInitialized || !frameData || width <= 0 || height <= 0 || pitch <= 0) {
        return false;
    }
    
    // Call Metal-specific frame processing
    return (Metal_AI_ProcessFrame(frameData, width, height, pitch) == 0);
}

/**
 * Renders AI visualization overlay using Metal
 * @param overlayData Pointer to the overlay data
 * @param width Overlay width
 * @param height Overlay height
 * @param pitch Overlay pitch (bytes per row)
 * @return true if rendering was successful, false otherwise
 */
bool AI_RenderOverlayMetal(void* overlayData, int width, int height, int pitch) {
    if (!g_bAIInitialized || !overlayData || width <= 0 || height <= 0 || pitch <= 0) {
        return false;
    }
    
    // Call Metal-specific overlay rendering
    return (Metal_AI_RenderOverlay(overlayData, width, height, pitch) == 0);
}

// Define the CoreML interface structure for use with the C stubs
// In a real implementation, this would connect to the Objective-C++ CoreML code
typedef struct {
    bool (*initialize)(void);
    bool (*findDefaultModels)(void);
    bool (*loadModel)(const char* path);
    bool (*getModelInfo)(struct AIModelInfo* info);
    bool (*predict)(const struct AIFrameData* frame, struct AIActions* actions);
    void (*shutdown)(void);
} CoreMLInterface;

// Static instance of the CoreML interface
static CoreMLInterface g_coreml_interface = {
    .initialize = CoreML_Initialize,
    .findDefaultModels = CoreML_FindDefaultModels,
    .loadModel = CoreML_LoadModel,
    .getModelInfo = CoreML_GetModelInfo,
    // Cast the function type to match expected signature
    .predict = (bool (*)(const struct AIFrameData *, struct AIActions *))CoreML_Predict,
    .shutdown = CoreML_Shutdown
};

// Function to get the CoreML interface
CoreMLInterface* AI_GetCoreMLInterface(void) {
    return &g_coreml_interface;
}

// Add back the necessary functions that were removed
// Capture current frame for AI processing
struct AIFrameData* AI_CaptureFrame(void) {
    // Get current frame dimensions
    int width = Metal_GetFrameWidth();
    int height = Metal_GetFrameHeight();
    
    if (width <= 0 || height <= 0 || width > MAX_FRAME_WIDTH || height > MAX_FRAME_HEIGHT) {
        printf("AI_CaptureFrame: Invalid frame dimensions: %dx%d\n", width, height);
        return NULL;
    }
    
    // If dimensions changed, update frame info
    if (width != frame_data.width || height != frame_data.height) {
        frame_data.width = width;
        frame_data.height = height;
        frame_data.size = width * height * frame_data.channels;
    }
    
    // In a real implementation, this would copy the frame buffer from the renderer
    // For now, we use a pre-allocated buffer filled with placeholder data
    static int frame_counter = 0;
    frame_counter++;
    
    // Generate a simple pattern for testing
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int pos = (y * width + x) * 4;
            frame_data.data[pos + 0] = (x + frame_counter) & 0xFF;  // B
            frame_data.data[pos + 1] = y & 0xFF;                    // G
            frame_data.data[pos + 2] = (x + y + frame_counter) & 0xFF; // R
            frame_data.data[pos + 3] = 0xFF;                        // A
        }
    }
    
    return &frame_data;
}

// Run model inference on frame data
bool AI_Predict(const struct AIFrameData* frame_data, struct AIActions* actions) {
    if (!frame_data || !actions || !g_bAIInitialized) {
        return false;
    }
    
    // Initialize actions
    memset(actions, 0, sizeof(struct AIActions));
    
    // Run CoreML prediction
    if (!CoreML_Predict(frame_data, actions)) {
        printf("AI_Predict: Prediction failed\n");
        return false;
    }
    
    // Apply confidence threshold
    for (int i = 0; i < MAX_ACTION_COUNT; i++) {
        if (actions->actions[i].confidence < ai_config.confidence_threshold) {
            actions->actions[i].active = false;
        }
    }
    
    // For debugging - log top actions
    printf("AI Prediction results:\n");
    for (int i = 0; i < MAX_ACTION_COUNT; i++) {
        if (actions->actions[i].active) {
            printf("  - Action %d: %s (conf: %.2f)\n", 
                  i, actions->actions[i].name, actions->actions[i].confidence);
        }
    }
    
    // Debug visualization if enabled
    if (ai_config.visualization) {
        // This would update visualization UI in a real implementation
        printf("AI visualization enabled - would show prediction details\n");
    }
    
    return true;
}

// Load a specific model file
bool AI_LoadModelFile(const char* model_path) {
    if (!g_bAIInitialized) {
        printf("AI_LoadModelFile: AI system not initialized\n");
        return false;
    }
    
    printf("AI_LoadModelFile: Loading model from %s\n", model_path);
    
    // Update config
    strncpy(ai_config.model_path, model_path, sizeof(ai_config.model_path) - 1);
    
    // Try to load the model using CoreML
    if (!CoreML_LoadModel(model_path)) {
        printf("AI_LoadModelFile: Failed to load model\n");
        return false;
    }
    
    // Get model info
    struct AIModelInfo model_info;
    if (CoreML_GetModelInfo(&model_info)) {
        printf("Model loaded: %s\n", model_info.name);
        printf("  - Input dimensions: %ux%u\n", 
               model_info.input_width, model_info.input_height);
        printf("  - Version: %s\n", model_info.version);
    }
    
    return true;
} 