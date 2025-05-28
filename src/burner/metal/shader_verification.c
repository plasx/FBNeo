#include "shader_verification.h"
#include "rom_loading_debug.h"
#include "memory_tracking.h"
#include <string.h>
#include <stdlib.h>

// Maximum number of shaders
#define MAX_SHADERS 64

// Registered shaders
static ShaderDescriptor g_shaders[MAX_SHADERS];
static int g_shaderCount = 0;
static int g_initialized = 0;

// Default metallib path
static char g_metallibPath[256] = "fbneo_shaders.metallib";

// Initialize shader verification system
void ShaderVerifier_Init(void) {
    if (g_initialized) {
        return;
    }
    
    // Clear shader array
    memset(g_shaders, 0, sizeof(g_shaders));
    g_shaderCount = 0;
    
    // Register default shaders
    // Vertex shader
    ShaderVerifier_RegisterShader(
        "default_vertexShader",
        SHADER_TYPE_VERTEX,
        SHADER_SOURCE_METALLIB,
        g_metallibPath,
        "default_vertexShader"
    );
    
    // Fragment shader
    ShaderVerifier_RegisterShader(
        "default_fragmentShader",
        SHADER_TYPE_FRAGMENT,
        SHADER_SOURCE_METALLIB,
        g_metallibPath,
        "default_fragmentShader"
    );
    
    g_initialized = 1;
    
    ROMLoader_TrackLoadStep("RENDERER INIT", "Shader verification system initialized");
}

// Shutdown shader verification system
void ShaderVerifier_Shutdown(void) {
    g_initialized = 0;
}

// Register a shader for verification
int ShaderVerifier_RegisterShader(const char* name, ShaderType type, 
                                ShaderSourceType sourceType, const char* source,
                                const char* entryPoint) {
    if (!g_initialized || !name || !source || !entryPoint || g_shaderCount >= MAX_SHADERS) {
        return -1;
    }
    
    // Check if shader already exists
    for (int i = 0; i < g_shaderCount; i++) {
        if (strcmp(g_shaders[i].name, name) == 0) {
            // Update existing shader
            g_shaders[i].type = type;
            g_shaders[i].sourceType = sourceType;
            g_shaders[i].source = source;
            g_shaders[i].entryPoint = entryPoint;
            g_shaders[i].status = SHADER_STATUS_UNVERIFIED;
            g_shaders[i].errorMessage[0] = '\0';
            
            ROMLoader_DebugLog(LOG_INFO, "Updated shader %s", name);
            return i;
        }
    }
    
    // Add new shader
    int id = g_shaderCount++;
    g_shaders[id].name = strdup(name);
    g_shaders[id].type = type;
    g_shaders[id].sourceType = sourceType;
    g_shaders[id].source = strdup(source);
    g_shaders[id].entryPoint = strdup(entryPoint);
    g_shaders[id].status = SHADER_STATUS_UNVERIFIED;
    g_shaders[id].errorMessage[0] = '\0';
    
    ROMLoader_DebugLog(LOG_INFO, "Registered shader %s (id=%d)", name, id);
    return id;
}

// In a real implementation, this would use Metal API to verify the shader
// Here we'll simulate verification
static int SimulateShaderVerification(ShaderDescriptor* shader) {
    // Assume all metallib shaders are valid
    if (shader->sourceType == SHADER_SOURCE_METALLIB) {
        // Check if the metallib file exists
        FILE* file = fopen(shader->source, "rb");
        if (file) {
            fclose(file);
            return 1;
        } else {
            strcpy(shader->errorMessage, "Metallib file not found");
            return 0;
        }
    }
    
    // For other source types, just assume they're valid for this simulation
    return 1;
}

// Verify all registered shaders
int ShaderVerifier_VerifyAll(void) {
    if (!g_initialized) {
        return 0;
    }
    
    int validCount = 0;
    
    for (int i = 0; i < g_shaderCount; i++) {
        if (ShaderVerifier_VerifyShader(i)) {
            validCount++;
        }
    }
    
    ROMLoader_TrackLoadStep("RENDERER INIT", "Verified %d/%d shaders successfully", 
                         validCount, g_shaderCount);
    
    return (validCount == g_shaderCount);
}

// Verify a specific shader
int ShaderVerifier_VerifyShader(int shaderId) {
    if (!g_initialized || shaderId < 0 || shaderId >= g_shaderCount) {
        return 0;
    }
    
    ShaderDescriptor* shader = &g_shaders[shaderId];
    
    // Simulate verification
    int isValid = SimulateShaderVerification(shader);
    
    // Update status
    if (isValid) {
        shader->status = SHADER_STATUS_VALID;
        ROMLoader_DebugLog(LOG_INFO, "Shader %s verified successfully", shader->name);
    } else {
        shader->status = SHADER_STATUS_INVALID;
        ROMLoader_DebugLog(LOG_ERROR, "Shader %s verification failed: %s", 
                         shader->name, shader->errorMessage);
    }
    
    return isValid;
}

// Get shader descriptor
ShaderDescriptor* ShaderVerifier_GetShader(int shaderId) {
    if (!g_initialized || shaderId < 0 || shaderId >= g_shaderCount) {
        return NULL;
    }
    
    return &g_shaders[shaderId];
}

// Get shader descriptor by name
ShaderDescriptor* ShaderVerifier_GetShaderByName(const char* name) {
    if (!g_initialized || !name) {
        return NULL;
    }
    
    for (int i = 0; i < g_shaderCount; i++) {
        if (strcmp(g_shaders[i].name, name) == 0) {
            return &g_shaders[i];
        }
    }
    
    return NULL;
}

// Get verification status
ShaderVerificationStatus ShaderVerifier_GetStatus(int shaderId) {
    if (!g_initialized || shaderId < 0 || shaderId >= g_shaderCount) {
        return SHADER_STATUS_MISSING;
    }
    
    return g_shaders[shaderId].status;
}

// Get error message
const char* ShaderVerifier_GetErrorMessage(int shaderId) {
    if (!g_initialized || shaderId < 0 || shaderId >= g_shaderCount) {
        return "Invalid shader ID";
    }
    
    return g_shaders[shaderId].errorMessage;
}

// Set default metallib path
void ShaderVerifier_SetMetallibPath(const char* path) {
    if (!path) {
        return;
    }
    
    strncpy(g_metallibPath, path, sizeof(g_metallibPath) - 1);
    g_metallibPath[sizeof(g_metallibPath) - 1] = '\0';
    
    ROMLoader_DebugLog(LOG_INFO, "Set metallib path to %s", g_metallibPath);
    
    // Update any shaders using the default metallib
    for (int i = 0; i < g_shaderCount; i++) {
        if (g_shaders[i].sourceType == SHADER_SOURCE_METALLIB && 
            strcmp(g_shaders[i].source, "fbneo_shaders.metallib") == 0) {
            g_shaders[i].source = g_metallibPath;
            g_shaders[i].status = SHADER_STATUS_UNVERIFIED;
        }
    }
}

// Get default metallib path
const char* ShaderVerifier_GetMetallibPath(void) {
    return g_metallibPath;
}

// Check if all shaders are valid
int ShaderVerifier_AllShadersValid(void) {
    if (!g_initialized || g_shaderCount == 0) {
        return 0;
    }
    
    for (int i = 0; i < g_shaderCount; i++) {
        if (g_shaders[i].status != SHADER_STATUS_VALID) {
            return 0;
        }
    }
    
    return 1;
}

// Log shader verification results
void ShaderVerifier_LogResults(void) {
    if (!g_initialized) {
        return;
    }
    
    int validCount = 0;
    int invalidCount = 0;
    int unverifiedCount = 0;
    
    for (int i = 0; i < g_shaderCount; i++) {
        switch (g_shaders[i].status) {
            case SHADER_STATUS_VALID:
                validCount++;
                break;
            case SHADER_STATUS_INVALID:
                invalidCount++;
                break;
            case SHADER_STATUS_UNVERIFIED:
                unverifiedCount++;
                break;
            default:
                break;
        }
    }
    
    ROMLoader_TrackLoadStep("RENDERER INIT", "Shader verification results: %d valid, %d invalid, %d unverified",
                         validCount, invalidCount, unverifiedCount);
    
    // Log details of invalid shaders
    if (invalidCount > 0) {
        ROMLoader_DebugLog(LOG_WARNING, "Invalid shaders:");
        for (int i = 0; i < g_shaderCount; i++) {
            if (g_shaders[i].status == SHADER_STATUS_INVALID) {
                ROMLoader_DebugLog(LOG_WARNING, "  %s: %s", 
                                 g_shaders[i].name, g_shaders[i].errorMessage);
            }
        }
    }
    
    // Log all shaders in verbose mode
    ROMLoader_DebugLog(LOG_VERBOSE, "All shaders:");
    for (int i = 0; i < g_shaderCount; i++) {
        const char* statusStr = 
            (g_shaders[i].status == SHADER_STATUS_VALID) ? "Valid" :
            (g_shaders[i].status == SHADER_STATUS_INVALID) ? "Invalid" :
            (g_shaders[i].status == SHADER_STATUS_UNVERIFIED) ? "Unverified" : "Unknown";
        
        ROMLoader_DebugLog(LOG_VERBOSE, "  %s (%s): %s, entry=%s, status=%s", 
                         g_shaders[i].name, 
                         (g_shaders[i].type == SHADER_TYPE_VERTEX) ? "Vertex" :
                         (g_shaders[i].type == SHADER_TYPE_FRAGMENT) ? "Fragment" : "Compute",
                         g_shaders[i].source, g_shaders[i].entryPoint, statusStr);
    }
}

// Get the number of registered shaders
int ShaderVerifier_GetShaderCount(void) {
    return g_shaderCount;
}

// Get the number of valid shaders
int ShaderVerifier_GetValidShaderCount(void) {
    if (!g_initialized) {
        return 0;
    }
    
    int count = 0;
    for (int i = 0; i < g_shaderCount; i++) {
        if (g_shaders[i].status == SHADER_STATUS_VALID) {
            count++;
        }
    }
    
    return count;
}

// Get the number of invalid shaders
int ShaderVerifier_GetInvalidShaderCount(void) {
    if (!g_initialized) {
        return 0;
    }
    
    int count = 0;
    for (int i = 0; i < g_shaderCount; i++) {
        if (g_shaders[i].status == SHADER_STATUS_INVALID) {
            count++;
        }
    }
    
    return count;
} 