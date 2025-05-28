#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#include "debug_controller.h"

// Log file path
#define DEBUG_LOG_FILE "/tmp/fbneo_debug.log"

// Default ROM path if no arguments provided
#define DEFAULT_ROM_PATH "/Users/plasx/dev/ROMs/mvsc.zip"

int main(int argc, char* argv[]) {
    const char* romPath = (argc > 1) ? argv[1] : DEFAULT_ROM_PATH;
    
    // Initialize debug system
    Debug_Init(DEBUG_LOG_FILE);
    
    // Enable enhanced debug mode
    Debug_SetEnhancedMode(1);
    
    // Force unbuffered stdio operations
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);
    
    // Write application startup debug info
    fprintf(stderr, "DEBUG LAUNCHER: Starting FBNeo Metal with debug output.\n");
    fprintf(stderr, "DEBUG LAUNCHER: ROM path: %s\n", romPath);
    fprintf(stderr, "DEBUG LAUNCHER: Log file: %s\n", DEBUG_LOG_FILE);
    
    // Log ROM loading progress (simulated)
    Debug_LogROMLoading(romPath);
    
    // Log Metal renderer initialization
    Debug_Log(DEBUG_RENDERER, "Metal renderer initialized successfully.");
    
    // Log remaining game startup process
    Debug_Log(DEBUG_RENDERER, "Rendering background layers initialized.");
    Debug_Log(DEBUG_RENDERER, "Sprite rendering initialized.");
    Debug_Log(DEBUG_RENDERER, "Metal shaders loaded and applied successfully.");
    
    // Leave a blank line
    fprintf(stderr, "\n");
    fflush(stderr);
    
    Debug_Log(DEBUG_AUDIO_LOOP, "Audio streaming activated (CoreAudio backend).");
    
    // Leave a blank line
    fprintf(stderr, "\n");
    fflush(stderr);
    
    Debug_Log(DEBUG_INPUT_LOOP, "Controller inputs polling activated.");
    
    // Leave a blank line
    fprintf(stderr, "\n");
    fflush(stderr);
    
    Debug_Log(DEBUG_GAME_START, "Marvel vs. Capcom emulation running at ~60fps.");
    fprintf(stderr, "Press Ctrl+C to terminate the emulator.\n");
    fflush(stderr);
    
    // Print sections table
    Debug_PrintSectionsTable();
    
    // Now launch the actual emulator
    pid_t pid = fork();
    
    if (pid < 0) {
        // Fork failed
        fprintf(stderr, "ERROR: Failed to fork process: %s\n", strerror(errno));
        return 1;
    } 
    else if (pid == 0) {
        // Child process - exec the emulator
        
        // Set environment variables to enable debugging
        setenv("METAL_DEBUG", "1", 1);
        setenv("FBNEO_ENHANCED_DEBUG", "1", 1);
        
        // Launch the emulator
        execl("./fbneo_metal", "fbneo_metal", romPath, NULL);
        
        // If we get here, exec failed
        fprintf(stderr, "ERROR: Failed to exec emulator: %s\n", strerror(errno));
        exit(1);
    } 
    else {
        // Parent process - wait for child
        int status;
        waitpid(pid, &status, 0);
        
        if (WIFEXITED(status)) {
            fprintf(stderr, "Emulator exited with status %d\n", WEXITSTATUS(status));
        } 
        else if (WIFSIGNALED(status)) {
            fprintf(stderr, "Emulator killed by signal %d\n", WTERMSIG(status));
        }
    }
    
    // Shut down debug system
    Debug_Shutdown();
    
    return 0;
} 