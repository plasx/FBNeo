#ifndef SHADER_VERIFICATION_H
#define SHADER_VERIFICATION_H

#include "metal_declarations.h"

#ifdef __cplusplus
extern "C" {
#endif

// Shader types
typedef enum {
    SHADER_TYPE_VERTEX,
    SHADER_TYPE_FRAGMENT,
    SHADER_TYPE_COMPUTE
} ShaderType;

// Shader verification status
typedef enum {
    SHADER_STATUS_UNVERIFIED,
    SHADER_STATUS_VALID,
    SHADER_STATUS_INVALID,
    SHADER_STATUS_MISSING
} ShaderVerificationStatus;

// Shader source types
typedef enum {
    SHADER_SOURCE_METALLIB,
    SHADER_SOURCE_STRING,
    SHADER_SOURCE_FILE
} ShaderSourceType;

// Shader descriptor
typedef struct {
    const char* name;                      // Shader function name
    ShaderType type;                       // Shader type
    ShaderSourceType sourceType;           // Source type
    const char* source;                    // Source code or file path
    const char* entryPoint;                // Entry point function
    ShaderVerificationStatus status;       // Verification status
    char errorMessage[1024];               // Error message if invalid
} ShaderDescriptor;

// Initialize shader verification system
void ShaderVerifier_Init(void);

// Shutdown shader verification system
void ShaderVerifier_Shutdown(void);

// Register a shader for verification
int ShaderVerifier_RegisterShader(const char* name, ShaderType type, 
                                ShaderSourceType sourceType, const char* source,
                                const char* entryPoint);

// Verify all registered shaders
int ShaderVerifier_VerifyAll(void);

// Verify a specific shader
int ShaderVerifier_VerifyShader(int shaderId);

// Get shader descriptor
ShaderDescriptor* ShaderVerifier_GetShader(int shaderId);

// Get shader descriptor by name
ShaderDescriptor* ShaderVerifier_GetShaderByName(const char* name);

// Get verification status
ShaderVerificationStatus ShaderVerifier_GetStatus(int shaderId);

// Get error message
const char* ShaderVerifier_GetErrorMessage(int shaderId);

// Set default metallib path
void ShaderVerifier_SetMetallibPath(const char* path);

// Get default metallib path
const char* ShaderVerifier_GetMetallibPath(void);

// Check if all shaders are valid
int ShaderVerifier_AllShadersValid(void);

// Log shader verification results
void ShaderVerifier_LogResults(void);

// Get the number of registered shaders
int ShaderVerifier_GetShaderCount(void);

// Get the number of valid shaders
int ShaderVerifier_GetValidShaderCount(void);

// Get the number of invalid shaders
int ShaderVerifier_GetInvalidShaderCount(void);

#ifdef __cplusplus
}
#endif

#endif // SHADER_VERIFICATION_H 