#pragma once

/**
 * Print usage information for CLI modes
 */
void printUsage();

/**
 * Parse command line arguments
 * 
 * @param argc Number of arguments
 * @param argv Array of argument strings
 * @return true if parsing successful, false otherwise
 */
bool parseCLIArgs(int argc, char* argv[]);

/**
 * Run collect mode - headless data collection
 * 
 * @return Exit code (0 for success, non-zero for error)
 */
int runCollectMode();

/**
 * Run play mode - play with AI
 * 
 * @return Exit code (0 for success, non-zero for error)
 */
int runPlayMode();

/**
 * Run train mode - model training
 * 
 * @return Exit code (0 for success, non-zero for error)
 */
int runTrainMode();

/**
 * Run replay mode - replay file playback
 * 
 * @return Exit code (0 for success, non-zero for error)
 */
int runReplayMode();

/**
 * Main function to handle CLI modes based on command line arguments
 * 
 * @param argc Number of arguments
 * @param argv Array of argument strings
 * @return Exit code (0 for success, non-zero for error)
 */
int handleCLIModes(int argc, char* argv[]);

/**
 * Set AI controller for a specific player
 * 
 * @param playerIndex Player index (1 or 2)
 * @param controller Pointer to AI controller or nullptr to disable
 */
void setAIControllerForPlayer(int playerIndex, void* controller);

/**
 * Load a ROM file
 * 
 * @param romPath Path to ROM file
 * @return true if successfully loaded, false otherwise
 */
bool loadRom(const char* romPath); 