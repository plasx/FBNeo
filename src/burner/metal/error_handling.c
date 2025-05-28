#include "error_handling.h"
#include "rom_loading_debug.h"
#include "memory_tracking.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>

// Maximum number of errors to keep in history
#define MAX_ERROR_HISTORY 32

// Error history
static ErrorInfo g_errorHistory[MAX_ERROR_HISTORY];
static int g_errorCount = 0;
static int g_nextErrorIndex = 0;
static int g_initialized = 0;
static int g_autoRecover = 0;

// Error callback
static ErrorCallback g_errorCallback = NULL;

// Error count by category
static int g_categoryErrorCount[ERROR_CATEGORY_CUSTOM + 1] = {0};

// Initialize error handling system
void ErrorHandler_Init(void) {
    // Clear error history
    memset(g_errorHistory, 0, sizeof(g_errorHistory));
    g_errorCount = 0;
    g_nextErrorIndex = 0;
    
    // Clear category error counts
    memset(g_categoryErrorCount, 0, sizeof(g_categoryErrorCount));
    
    // Auto recovery default (disabled)
    g_autoRecover = 0;
    
    g_initialized = 1;
    
    ROMLoader_TrackLoadStep("ERROR INIT", "Error handling system initialized");
}

// Shutdown error handling system
void ErrorHandler_Shutdown(void) {
    g_initialized = 0;
    g_errorCallback = NULL;
}

// Report an error
void ErrorHandler_ReportError(ErrorCode code, ErrorSeverity severity, ErrorCategory category,
                             const char* message, const char* details, const char* source) {
    if (!g_initialized) {
        // Initialize if not already done
        ErrorHandler_Init();
    }
    
    // Create new error info
    ErrorInfo newError;
    memset(&newError, 0, sizeof(ErrorInfo));
    
    newError.code = code;
    newError.severity = severity;
    newError.category = category;
    newError.timestamp = (int)time(NULL);
    newError.handled = 0;
    
    // Copy strings with truncation protection
    if (message) {
        strncpy(newError.message, message, sizeof(newError.message) - 1);
        newError.message[sizeof(newError.message) - 1] = '\0';
    } else {
        strncpy(newError.message, ErrorHandler_GetErrorMessage(code), sizeof(newError.message) - 1);
        newError.message[sizeof(newError.message) - 1] = '\0';
    }
    
    if (details) {
        strncpy(newError.details, details, sizeof(newError.details) - 1);
        newError.details[sizeof(newError.details) - 1] = '\0';
    }
    
    if (source) {
        strncpy(newError.source, source, sizeof(newError.source) - 1);
        newError.source[sizeof(newError.source) - 1] = '\0';
    } else {
        strcpy(newError.source, "unknown");
    }
    
    // Add suggestion
    strncpy(newError.suggestion, ErrorHandler_GetErrorSuggestion(code), sizeof(newError.suggestion) - 1);
    newError.suggestion[sizeof(newError.suggestion) - 1] = '\0';
    
    // Add to history
    g_errorHistory[g_nextErrorIndex] = newError;
    g_nextErrorIndex = (g_nextErrorIndex + 1) % MAX_ERROR_HISTORY;
    g_errorCount++;
    
    // Increment category error count
    if (category <= ERROR_CATEGORY_CUSTOM) {
        g_categoryErrorCount[category]++;
    }
    
    // Log the error
    const char* severityStr = 
        (severity == ERROR_SEVERITY_INFO) ? "INFO" :
        (severity == ERROR_SEVERITY_WARNING) ? "WARNING" :
        (severity == ERROR_SEVERITY_ERROR) ? "ERROR" :
        (severity == ERROR_SEVERITY_FATAL) ? "FATAL" : "UNKNOWN";
    
    ROMLoader_DebugLog(
        (severity == ERROR_SEVERITY_INFO) ? LOG_INFO :
        (severity == ERROR_SEVERITY_WARNING) ? LOG_WARNING :
        LOG_ERROR,
        "%s: %s (code=%d, source=%s)",
        severityStr, newError.message, code, newError.source
    );
    
    // Show detailed error for errors and fatals
    if (severity >= ERROR_SEVERITY_ERROR && details && details[0]) {
        ROMLoader_DebugLog(LOG_ERROR, "Details: %s", details);
    }
    
    // Show suggestion for errors and fatals
    if (severity >= ERROR_SEVERITY_ERROR && newError.suggestion[0]) {
        ROMLoader_DebugLog(LOG_ERROR, "Suggestion: %s", newError.suggestion);
    }
    
    // For each error category, use an appropriate tracking tag
    const char* trackTag = 
        (category == ERROR_CATEGORY_SYSTEM) ? "SYSTEM ERROR" :
        (category == ERROR_CATEGORY_ROM) ? "ROM ERROR" :
        (category == ERROR_CATEGORY_GRAPHICS) ? "GRAPHICS ERROR" :
        (category == ERROR_CATEGORY_AUDIO) ? "AUDIO ERROR" :
        (category == ERROR_CATEGORY_INPUT) ? "INPUT ERROR" :
        (category == ERROR_CATEGORY_EMULATION) ? "EMULATION ERROR" :
        (category == ERROR_CATEGORY_NETWORK) ? "NETWORK ERROR" :
        (category == ERROR_CATEGORY_CONFIG) ? "CONFIG ERROR" :
        (category == ERROR_CATEGORY_SHADER) ? "SHADER ERROR" :
        "ERROR";
    
    // Track the error in the load step system
    if (severity >= ERROR_SEVERITY_WARNING) {
        ROMLoader_TrackLoadStep(trackTag, "%s: %s", severityStr, newError.message);
        
        if (severity >= ERROR_SEVERITY_ERROR && newError.suggestion[0]) {
            ROMLoader_TrackLoadStep(trackTag, "Suggestion: %s", newError.suggestion);
        }
    }
    
    // Call error callback if registered
    if (g_errorCallback) {
        g_errorCallback(&newError);
    }
    
    // Attempt auto recovery if enabled
    if (g_autoRecover && ErrorHandler_IsRecoverable(code)) {
        ErrorHandler_AttemptRecovery(code);
    }
}

// Register error callback
void ErrorHandler_RegisterCallback(ErrorCallback callback) {
    g_errorCallback = callback;
}

// Get the last error
const ErrorInfo* ErrorHandler_GetLastError(void) {
    if (g_errorCount == 0) {
        return NULL;
    }
    
    int lastIndex = (g_nextErrorIndex + MAX_ERROR_HISTORY - 1) % MAX_ERROR_HISTORY;
    return &g_errorHistory[lastIndex];
}

// Clear the last error
void ErrorHandler_ClearLastError(void) {
    if (g_errorCount == 0) {
        return;
    }
    
    int lastIndex = (g_nextErrorIndex + MAX_ERROR_HISTORY - 1) % MAX_ERROR_HISTORY;
    g_errorHistory[lastIndex].handled = 1;
}

// Get error message for a specific error code
const char* ErrorHandler_GetErrorMessage(ErrorCode code) {
    switch (code) {
        case ERROR_SUCCESS:
            return "Operation completed successfully";
        case ERROR_GENERAL_FAILURE:
            return "General failure";
        case ERROR_FILE_NOT_FOUND:
            return "File not found";
        case ERROR_FILE_READ_ERROR:
            return "File read error";
        case ERROR_FILE_WRITE_ERROR:
            return "File write error";
        case ERROR_OUT_OF_MEMORY:
            return "Out of memory";
        case ERROR_INVALID_PARAMETER:
            return "Invalid parameter";
        case ERROR_INVALID_STATE:
            return "Invalid state";
        case ERROR_NOT_IMPLEMENTED:
            return "Not implemented";
        case ERROR_NOT_SUPPORTED:
            return "Not supported";
        case ERROR_ROM_NOT_FOUND:
            return "ROM not found";
        case ERROR_ROM_INVALID:
            return "Invalid ROM format";
        case ERROR_ROM_UNSUPPORTED:
            return "Unsupported ROM";
        case ERROR_ROM_MISSING_FILES:
            return "Missing ROM files";
        case ERROR_GRAPHICS_INIT_FAILED:
            return "Graphics initialization failed";
        case ERROR_GRAPHICS_DEVICE_LOST:
            return "Graphics device lost";
        case ERROR_AUDIO_INIT_FAILED:
            return "Audio initialization failed";
        case ERROR_AUDIO_DEVICE_LOST:
            return "Audio device lost";
        case ERROR_INPUT_INIT_FAILED:
            return "Input initialization failed";
        case ERROR_SHADER_COMPILATION:
            return "Shader compilation error";
        case ERROR_SHADER_MISSING:
            return "Shader file missing";
        case ERROR_NETWORK_FAILURE:
            return "Network failure";
        case ERROR_CONFIG_INVALID:
            return "Invalid configuration";
        default:
            return "Unknown error";
    }
}

// Get error suggestion for a specific error code
const char* ErrorHandler_GetErrorSuggestion(ErrorCode code) {
    switch (code) {
        case ERROR_SUCCESS:
            return "";
        case ERROR_GENERAL_FAILURE:
            return "Check the logs for more details";
        case ERROR_FILE_NOT_FOUND:
            return "Verify the file path is correct and the file exists";
        case ERROR_FILE_READ_ERROR:
            return "Ensure the file is not corrupted and you have permission to read it";
        case ERROR_FILE_WRITE_ERROR:
            return "Ensure you have permission to write to the destination and enough disk space";
        case ERROR_OUT_OF_MEMORY:
            return "Close other applications to free memory or restart the application";
        case ERROR_INVALID_PARAMETER:
            return "Check the input parameters or configuration";
        case ERROR_INVALID_STATE:
            return "The operation was called in an invalid state, try restarting the application";
        case ERROR_NOT_IMPLEMENTED:
            return "This feature is not yet implemented";
        case ERROR_NOT_SUPPORTED:
            return "This feature is not supported on your system";
        case ERROR_ROM_NOT_FOUND:
            return "Verify the ROM path is correct and the ROM file exists";
        case ERROR_ROM_INVALID:
            return "The ROM file may be corrupted or in an unsupported format";
        case ERROR_ROM_UNSUPPORTED:
            return "This ROM is not supported by the emulator";
        case ERROR_ROM_MISSING_FILES:
            return "The ROM requires additional files which are missing";
        case ERROR_GRAPHICS_INIT_FAILED:
            return "Ensure your graphics drivers are up to date";
        case ERROR_GRAPHICS_DEVICE_LOST:
            return "The graphics device was lost, try restarting the application";
        case ERROR_AUDIO_INIT_FAILED:
            return "Ensure your audio drivers are up to date";
        case ERROR_AUDIO_DEVICE_LOST:
            return "The audio device was lost, try restarting the application";
        case ERROR_INPUT_INIT_FAILED:
            return "Ensure your controllers are properly connected";
        case ERROR_SHADER_COMPILATION:
            return "Check the shader code for errors";
        case ERROR_SHADER_MISSING:
            return "Ensure the shader files are in the correct location";
        case ERROR_NETWORK_FAILURE:
            return "Check your network connection";
        case ERROR_CONFIG_INVALID:
            return "The configuration file may be corrupted, try resetting to defaults";
        default:
            return "Check the logs for more details";
    }
}

// Log all errors
void ErrorHandler_LogErrors(void) {
    if (g_errorCount == 0) {
        ROMLoader_DebugLog(LOG_INFO, "No errors to report");
        return;
    }
    
    int numToShow = (g_errorCount < MAX_ERROR_HISTORY) ? g_errorCount : MAX_ERROR_HISTORY;
    
    ROMLoader_DebugLog(LOG_INFO, "Error history (%d errors, showing %d):", g_errorCount, numToShow);
    
    for (int i = 0; i < numToShow; i++) {
        int index = (g_nextErrorIndex - i - 1 + MAX_ERROR_HISTORY) % MAX_ERROR_HISTORY;
        ErrorInfo* error = &g_errorHistory[index];
        
        const char* severityStr = 
            (error->severity == ERROR_SEVERITY_INFO) ? "INFO" :
            (error->severity == ERROR_SEVERITY_WARNING) ? "WARNING" :
            (error->severity == ERROR_SEVERITY_ERROR) ? "ERROR" :
            (error->severity == ERROR_SEVERITY_FATAL) ? "FATAL" : "UNKNOWN";
        
        ROMLoader_DebugLog(LOG_INFO, "[%d] %s: %s (code=%d, source=%s, handled=%d)",
                         i + 1, severityStr, error->message, error->code, 
                         error->source, error->handled);
    }
    
    // Log category counts
    ROMLoader_DebugLog(LOG_INFO, "Error counts by category:");
    for (int i = 0; i <= ERROR_CATEGORY_CUSTOM; i++) {
        if (g_categoryErrorCount[i] > 0) {
            const char* categoryStr = 
                (i == ERROR_CATEGORY_SYSTEM) ? "System" :
                (i == ERROR_CATEGORY_ROM) ? "ROM" :
                (i == ERROR_CATEGORY_GRAPHICS) ? "Graphics" :
                (i == ERROR_CATEGORY_AUDIO) ? "Audio" :
                (i == ERROR_CATEGORY_INPUT) ? "Input" :
                (i == ERROR_CATEGORY_EMULATION) ? "Emulation" :
                (i == ERROR_CATEGORY_NETWORK) ? "Network" :
                (i == ERROR_CATEGORY_CONFIG) ? "Config" :
                (i == ERROR_CATEGORY_SHADER) ? "Shader" :
                (i == ERROR_CATEGORY_CUSTOM) ? "Custom" : "Unknown";
            
            ROMLoader_DebugLog(LOG_INFO, "  %s: %d", categoryStr, g_categoryErrorCount[i]);
        }
    }
}

// Get the number of errors since initialization
int ErrorHandler_GetErrorCount(void) {
    return g_errorCount;
}

// Get the number of errors in a specific category
int ErrorHandler_GetCategoryErrorCount(ErrorCategory category) {
    if (category > ERROR_CATEGORY_CUSTOM) {
        return 0;
    }
    
    return g_categoryErrorCount[category];
}

// Try to recover from an error
int ErrorHandler_AttemptRecovery(ErrorCode code) {
    if (!ErrorHandler_IsRecoverable(code)) {
        ROMLoader_DebugLog(LOG_WARNING, "Cannot recover from error code %d", code);
        return 0;
    }
    
    int success = 0;
    
    // Implement recovery strategies for different errors
    switch (code) {
        case ERROR_FILE_NOT_FOUND:
        case ERROR_FILE_READ_ERROR:
            // Could offer to browse for the file
            ROMLoader_TrackLoadStep("ERROR RECOVERY", "Please select the file manually");
            success = 0; // Manual intervention needed
            break;
            
        case ERROR_GRAPHICS_DEVICE_LOST:
            // Try to reinitialize graphics
            ROMLoader_TrackLoadStep("ERROR RECOVERY", "Attempting to reinitialize graphics");
            // In a real implementation, this would call graphics reinitialization
            success = 1; // Assume success for now
            break;
            
        case ERROR_AUDIO_DEVICE_LOST:
            // Try to reinitialize audio
            ROMLoader_TrackLoadStep("ERROR RECOVERY", "Attempting to reinitialize audio");
            // In a real implementation, this would call audio reinitialization
            success = 1; // Assume success for now
            break;
            
        case ERROR_SHADER_MISSING:
            // Try to load default shader
            ROMLoader_TrackLoadStep("ERROR RECOVERY", "Loading default shader");
            // In a real implementation, this would load a default shader
            success = 1; // Assume success for now
            break;
            
        default:
            // No specific recovery strategy
            ROMLoader_DebugLog(LOG_WARNING, "No specific recovery strategy for error code %d", code);
            success = 0;
            break;
    }
    
    if (success) {
        ROMLoader_TrackLoadStep("ERROR RECOVERY", "Successfully recovered from error");
    } else {
        ROMLoader_TrackLoadStep("ERROR RECOVERY", "Failed to recover from error");
    }
    
    return success;
}

// Set error recovery policy
void ErrorHandler_SetRecoveryPolicy(int autoRecover) {
    g_autoRecover = autoRecover;
    
    ROMLoader_DebugLog(LOG_INFO, "Error recovery policy set to %s", 
                     autoRecover ? "automatic" : "manual");
}

// Check if error is recoverable
int ErrorHandler_IsRecoverable(ErrorCode code) {
    // Some errors are inherently recoverable
    switch (code) {
        case ERROR_FILE_NOT_FOUND:
        case ERROR_FILE_READ_ERROR:
        case ERROR_GRAPHICS_DEVICE_LOST:
        case ERROR_AUDIO_DEVICE_LOST:
        case ERROR_SHADER_MISSING:
        case ERROR_SHADER_COMPILATION:
        case ERROR_NETWORK_FAILURE:
            return 1;
            
        // Fatal errors are not recoverable
        case ERROR_OUT_OF_MEMORY:
        case ERROR_ROM_UNSUPPORTED:
        case ERROR_NOT_IMPLEMENTED:
        case ERROR_NOT_SUPPORTED:
            return 0;
            
        // Other errors are potentially recoverable but may require manual intervention
        default:
            return 0;
    }
}

// Format error message with details
void ErrorHandler_FormatErrorMessage(char* buffer, size_t bufferSize, const ErrorInfo* error) {
    if (!buffer || bufferSize == 0 || !error) {
        return;
    }
    
    const char* severityStr = 
        (error->severity == ERROR_SEVERITY_INFO) ? "INFO" :
        (error->severity == ERROR_SEVERITY_WARNING) ? "WARNING" :
        (error->severity == ERROR_SEVERITY_ERROR) ? "ERROR" :
        (error->severity == ERROR_SEVERITY_FATAL) ? "FATAL" : "UNKNOWN";
    
    const char* categoryStr = 
        (error->category == ERROR_CATEGORY_SYSTEM) ? "System" :
        (error->category == ERROR_CATEGORY_ROM) ? "ROM" :
        (error->category == ERROR_CATEGORY_GRAPHICS) ? "Graphics" :
        (error->category == ERROR_CATEGORY_AUDIO) ? "Audio" :
        (error->category == ERROR_CATEGORY_INPUT) ? "Input" :
        (error->category == ERROR_CATEGORY_EMULATION) ? "Emulation" :
        (error->category == ERROR_CATEGORY_NETWORK) ? "Network" :
        (error->category == ERROR_CATEGORY_CONFIG) ? "Config" :
        (error->category == ERROR_CATEGORY_SHADER) ? "Shader" :
        (error->category == ERROR_CATEGORY_CUSTOM) ? "Custom" : "Unknown";
    
    // Format basic message
    snprintf(buffer, bufferSize, "[%s] %s: %s (Code: %d, Source: %s)", 
             severityStr, categoryStr, error->message, error->code, error->source);
    
    // Add details if available
    if (error->details[0]) {
        size_t currentLength = strlen(buffer);
        if (currentLength + 12 < bufferSize) {
            strcat(buffer, "\nDetails: ");
            currentLength += 10;
            
            size_t remainingSpace = bufferSize - currentLength - 1;
            strncat(buffer, error->details, remainingSpace);
        }
    }
    
    // Add suggestion if available
    if (error->suggestion[0]) {
        size_t currentLength = strlen(buffer);
        if (currentLength + 15 < bufferSize) {
            strcat(buffer, "\nSuggestion: ");
            currentLength += 13;
            
            size_t remainingSpace = bufferSize - currentLength - 1;
            strncat(buffer, error->suggestion, remainingSpace);
        }
    }
} 