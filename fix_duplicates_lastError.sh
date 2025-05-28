#!/bin/bash
# Fix duplicate g_lastError symbol issues

set -e  # Exit on error

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
YELLOW='\033[0;33m'
RESET='\033[0m'

echo -e "${BLUE}Fixing duplicate g_lastError symbol...${RESET}"

# Create directory for unified implementations if it doesn't exist
mkdir -p build/metal_fixed

# Create a unified implementation for error handling
echo -e "${BLUE}Creating unified error handling implementation...${RESET}"

# We need to update the existing error_functions.c to be more complete
cat > build/metal_fixed/error_functions.c << 'EOL'
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>

// Error info structure
typedef struct {
    int code;
    char message[256];
    char function[64];
    char file[128];
    int line;
} MetalErrorInfo;

// Global error state - this is the ONLY global instance
MetalErrorInfo g_lastError = {0};

// Error handling functions
void Metal_ClearLastError(void) {
    g_lastError.code = 0;
    memset(g_lastError.message, 0, sizeof(g_lastError.message));
    memset(g_lastError.function, 0, sizeof(g_lastError.function));
    memset(g_lastError.file, 0, sizeof(g_lastError.file));
    g_lastError.line = 0;
}

bool Metal_HasError(void) {
    return g_lastError.code != 0;
}

const char* Metal_GetLastErrorMessage(void) {
    return g_lastError.message;
}

void Metal_SetError(int code, const char* message) {
    g_lastError.code = code;
    if (message) {
        strncpy(g_lastError.message, message, sizeof(g_lastError.message) - 1);
        g_lastError.message[sizeof(g_lastError.message) - 1] = '\0';
    } else {
        g_lastError.message[0] = '\0';
    }
    
    // Print the error to console
    printf("ERROR: %s (code %d)\n", message ? message : "Unknown error", code);
}

// Log functions
int g_logLevel = 2; // Default to warning level

void Metal_LogMessage(int level, const char* format, ...) {
    // Skip if log level is higher than current setting
    if (level > g_logLevel) return;
    
    va_list args;
    char buffer[1024];
    
    // Format the message
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer)-1, format, args);
    va_end(args);
    buffer[sizeof(buffer)-1] = '\0';
    
    // Print to console
    printf("[Metal] %s\n", buffer);
}

void Metal_SetLogLevel(int level) {
    g_logLevel = level;
}

int Metal_EnableFallbackAudio(void) {
    printf("Enabling fallback audio\n");
    return 0;
}

// Debug mode tracking
static bool g_debugMode = false;

void Metal_SetDebugMode(bool enabled) {
    g_debugMode = enabled;
    printf("Debug mode: %s\n", enabled ? "ON" : "OFF");
}

bool Metal_IsDebugMode(void) {
    return g_debugMode;
}
EOL

# Comment out duplicated g_lastError in build_final_resolved.sh
if [ -f build_final_resolved.sh ]; then
    echo -e "${BLUE}Patching build_final_resolved.sh...${RESET}"
    
    # Create backup
    cp build_final_resolved.sh build_final_resolved.sh.bak
    
    # Comment out the definition of g_lastError
    sed -i '' '/^MetalErrorInfo g_lastError = {0};/s/^/\/\/ REMOVED: Using the implementation from error_functions.c\n\/\/ /' build_final_resolved.sh
fi

# Comment out duplicated g_lastError in metal_error_handling.c
if [ -f src/burner/metal/metal_error_handling.c ]; then
    echo -e "${BLUE}Patching metal_error_handling.c...${RESET}"
    
    # Create backup
    cp src/burner/metal/metal_error_handling.c src/burner/metal/metal_error_handling.c.bak
    
    # Comment out the declaration of g_lastError
    sed -i '' '/^MetalErrorInfo g_lastError = {0};/s/^/\/\/ REMOVED: Using the implementation from error_functions.c\n\/\/ /' src/burner/metal/metal_error_handling.c
fi

# Comment out duplicated g_lastError in build/metal_unified_error.c
if [ -f build/metal_unified_error.c ]; then
    echo -e "${BLUE}Patching build/metal_unified_error.c...${RESET}"
    
    # Create backup
    cp build/metal_unified_error.c build/metal_unified_error.c.bak
    
    # Comment out the declaration of g_lastError
    sed -i '' '/^MetalErrorInfo g_lastError = {0};/s/^/\/\/ REMOVED: Using the implementation from error_functions.c\n\/\/ /' build/metal_unified_error.c
fi

# Comment out duplicated g_lastError in build/simplified/error_handling.c
if [ -f build/simplified/error_handling.c ]; then
    echo -e "${BLUE}Patching build/simplified/error_handling.c...${RESET}"
    
    # Create backup
    cp build/simplified/error_handling.c build/simplified/error_handling.c.bak
    
    # Comment out the declaration of g_lastError
    sed -i '' '/^static ErrorInfo g_lastError = {0};/s/^/\/\/ REMOVED: Using the implementation from error_functions.c\n\/\/ /' build/simplified/error_handling.c
fi

echo -e "${GREEN}g_lastError duplicate fixed successfully!${RESET}" 