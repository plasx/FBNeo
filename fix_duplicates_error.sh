#!/bin/bash
# Fix duplicate symbol issues in error handling-related files

set -e  # Exit on error

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
YELLOW='\033[0;33m'
RESET='\033[0m'

echo -e "${BLUE}Fixing duplicate error handling symbols...${RESET}"

# Comment out duplicated Metal_LogMessage, Metal_SetLogLevel, Metal_SetDebugMode, etc. in metal_bridge_stubs.c
if [ -f src/burner/metal/metal_bridge_stubs.c ]; then
    echo -e "${BLUE}Patching metal_bridge_stubs.c...${RESET}"
    
    # Create backup
    cp src/burner/metal/metal_bridge_stubs.c src/burner/metal/metal_bridge_stubs.c.bak
    
    # Comment out Metal_LogMessage
    sed -i '' '/^void Metal_LogMessage/,/^}/c\
// REMOVED: Duplicate implementation of Metal_LogMessage\
// Using the implementation from metal_error_handling.c\
void Metal_LogMessage_UNUSED(MetalLogLevel level, const char* format, ...) {\
    // This function has been removed to avoid duplicates\
}' src/burner/metal/metal_bridge_stubs.c
    
    # Comment out Metal_SetLogLevel
    sed -i '' '/^void Metal_SetLogLevel/,/^}/c\
// REMOVED: Duplicate implementation of Metal_SetLogLevel\
// Using the implementation from metal_error_handling.c\
void Metal_SetLogLevel_UNUSED(MetalLogLevel level) {\
    // This function has been removed to avoid duplicates\
}' src/burner/metal/metal_bridge_stubs.c
    
    # Comment out Metal_SetDebugMode
    sed -i '' '/^void Metal_SetDebugMode/,/^}/c\
// REMOVED: Duplicate implementation of Metal_SetDebugMode\
// Using the implementation from metal_error_handling.c\
void Metal_SetDebugMode_UNUSED(bool enabled) {\
    // This function has been removed to avoid duplicates\
}' src/burner/metal/metal_bridge_stubs.c
    
    # Comment out Metal_HasError
    sed -i '' '/^bool Metal_HasError/,/^}/c\
// REMOVED: Duplicate implementation of Metal_HasError\
// Using the implementation from metal_error_handling.c\
bool Metal_HasError_UNUSED(void) {\
    // This function has been removed to avoid duplicates\
    return false;\
}' src/burner/metal/metal_bridge_stubs.c
    
    # Comment out Metal_GetLastErrorMessage
    sed -i '' '/^const char\* Metal_GetLastErrorMessage/,/^}/c\
// REMOVED: Duplicate implementation of Metal_GetLastErrorMessage\
// Using the implementation from metal_error_handling.c\
const char* Metal_GetLastErrorMessage_UNUSED(void) {\
    // This function has been removed to avoid duplicates\
    return "Unused";\
}' src/burner/metal/metal_bridge_stubs.c
    
    # Comment out Metal_ClearLastError
    sed -i '' '/^void Metal_ClearLastError/,/^}/c\
// REMOVED: Duplicate implementation of Metal_ClearLastError\
// Using the implementation from metal_error_handling.c\
void Metal_ClearLastError_UNUSED(void) {\
    // This function has been removed to avoid duplicates\
}' src/burner/metal/metal_bridge_stubs.c
    
    # Comment out Metal_EnableFallbackAudio
    sed -i '' '/^int Metal_EnableFallbackAudio/,/^}/c\
// REMOVED: Duplicate implementation of Metal_EnableFallbackAudio\
// Using the implementation from metal_error_handling.c\
int Metal_EnableFallbackAudio_UNUSED(void) {\
    // This function has been removed to avoid duplicates\
    return 0;\
}' src/burner/metal/metal_bridge_stubs.c
fi

echo -e "${GREEN}Error handling duplicates fixed successfully!${RESET}" 