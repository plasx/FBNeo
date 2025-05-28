#ifndef ERROR_HANDLING_H
#define ERROR_HANDLING_H

#include "metal_declarations.h"

#ifdef __cplusplus
extern "C" {
#endif

// Error severity levels
typedef enum {
    ERROR_SEVERITY_INFO,      // Informational message, not an error
    ERROR_SEVERITY_WARNING,   // Warning, operation can continue
    ERROR_SEVERITY_ERROR,     // Error, operation may need to be aborted
    ERROR_SEVERITY_FATAL      // Fatal error, application needs to terminate
} ErrorSeverity;

// Error categories
typedef enum {
    ERROR_CATEGORY_SYSTEM,    // System-level errors (file I/O, memory, etc.)
    ERROR_CATEGORY_ROM,       // ROM loading/validation errors
    ERROR_CATEGORY_GRAPHICS,  // Graphics/rendering errors
    ERROR_CATEGORY_AUDIO,     // Audio errors
    ERROR_CATEGORY_INPUT,     // Input/controller errors
    ERROR_CATEGORY_EMULATION, // Emulation errors
    ERROR_CATEGORY_NETWORK,   // Network errors
    ERROR_CATEGORY_CONFIG,    // Configuration errors
    ERROR_CATEGORY_SHADER,    // Shader errors
    ERROR_CATEGORY_CUSTOM     // Custom error category
} ErrorCategory;

// Error code type
typedef int ErrorCode;

// Common error codes
#define ERROR_SUCCESS                   0
#define ERROR_GENERAL_FAILURE          -1
#define ERROR_FILE_NOT_FOUND          -10
#define ERROR_FILE_READ_ERROR         -11
#define ERROR_FILE_WRITE_ERROR        -12
#define ERROR_OUT_OF_MEMORY           -20
#define ERROR_INVALID_PARAMETER       -30
#define ERROR_INVALID_STATE           -31
#define ERROR_NOT_IMPLEMENTED         -40
#define ERROR_NOT_SUPPORTED           -41
#define ERROR_ROM_NOT_FOUND           -50
#define ERROR_ROM_INVALID             -51
#define ERROR_ROM_UNSUPPORTED         -52
#define ERROR_ROM_MISSING_FILES       -53
#define ERROR_GRAPHICS_INIT_FAILED    -60
#define ERROR_GRAPHICS_DEVICE_LOST    -61
#define ERROR_AUDIO_INIT_FAILED       -70
#define ERROR_AUDIO_DEVICE_LOST       -71
#define ERROR_INPUT_INIT_FAILED       -80
#define ERROR_SHADER_COMPILATION      -90
#define ERROR_SHADER_MISSING          -91
#define ERROR_NETWORK_FAILURE        -100
#define ERROR_CONFIG_INVALID         -110

// Error info structure
typedef struct {
    ErrorCode code;                 // Error code
    ErrorSeverity severity;         // Error severity
    ErrorCategory category;         // Error category
    char message[256];              // Error message
    char details[1024];             // Detailed error information
    char source[64];                // Source of the error (file:line or function)
    char suggestion[256];           // Suggestion for resolving the error
    int timestamp;                  // Timestamp when the error occurred
    int handled;                    // Whether the error has been handled
} ErrorInfo;

// Error callback function type
typedef void (*ErrorCallback)(const ErrorInfo* error);

// Initialize error handling system
void ErrorHandler_Init(void);

// Shutdown error handling system
void ErrorHandler_Shutdown(void);

// Report an error
void ErrorHandler_ReportError(ErrorCode code, ErrorSeverity severity, ErrorCategory category,
                             const char* message, const char* details, const char* source);

// Register error callback
void ErrorHandler_RegisterCallback(ErrorCallback callback);

// Get the last error
const ErrorInfo* ErrorHandler_GetLastError(void);

// Clear the last error
void ErrorHandler_ClearLastError(void);

// Get error message for a specific error code
const char* ErrorHandler_GetErrorMessage(ErrorCode code);

// Get error suggestion for a specific error code
const char* ErrorHandler_GetErrorSuggestion(ErrorCode code);

// Log all errors
void ErrorHandler_LogErrors(void);

// Get the number of errors since initialization
int ErrorHandler_GetErrorCount(void);

// Get the number of errors in a specific category
int ErrorHandler_GetCategoryErrorCount(ErrorCategory category);

// Try to recover from an error
int ErrorHandler_AttemptRecovery(ErrorCode code);

// Set error recovery policy
void ErrorHandler_SetRecoveryPolicy(int autoRecover);

// Check if error is recoverable
int ErrorHandler_IsRecoverable(ErrorCode code);

// Format error message with details
void ErrorHandler_FormatErrorMessage(char* buffer, size_t bufferSize, const ErrorInfo* error);

#ifdef __cplusplus
}
#endif

#endif // ERROR_HANDLING_H 