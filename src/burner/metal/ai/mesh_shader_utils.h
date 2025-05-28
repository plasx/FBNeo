#pragma once

#import <Metal/Metal.h>
#import <simd/simd.h>
#include <vector>
#include <memory>

/**
 * @brief Utility class for Mesh Shader processing in Metal 3
 * 
 * This class provides helpers for using Metal 3's mesh shaders,
 * which allow for more efficient geometry processing directly on the GPU.
 * These are used for complex debug visualizations in the AI system.
 */
class MeshShaderUtils {
public:
    /**
     * Initialize mesh shader utilities
     * @param device Metal device
     * @return True if initialization succeeded
     */
    static bool initialize(id<MTLDevice> device);
    
    /**
     * Check if mesh shaders are supported on the current device
     * @return True if mesh shaders are supported
     */
    static bool areMeshShadersSupported();
    
    /**
     * Create a mesh shader pipeline for debug visualization
     * @param device Metal device
     * @param vertexFunction Name of vertex function
     * @param fragmentFunction Name of fragment function
     * @param objectFunction Name of object function (mesh shader)
     * @param meshFunction Name of mesh function (mesh shader)
     * @return Pipeline state object or nil if creation failed
     */
    static id<MTLRenderPipelineState> createMeshPipeline(
        id<MTLDevice> device,
        NSString* vertexFunction,
        NSString* fragmentFunction, 
        NSString* objectFunction,
        NSString* meshFunction);
    
    /**
     * Create a library with mesh shader functions
     * @param device Metal device
     * @return Metal library or nil if creation failed
     */
    static id<MTLLibrary> createMeshShaderLibrary(id<MTLDevice> device);
    
    /**
     * Draw mesh using object and mesh shaders
     * @param encoder Command encoder
     * @param pipeline Pipeline state
     * @param objectBuffer Buffer containing object data
     * @param objectCount Number of objects to process
     * @param meshArgumentBuffer Buffer containing mesh arguments
     */
    static void drawMeshShaders(
        id<MTLRenderCommandEncoder> encoder,
        id<MTLRenderPipelineState> pipeline,
        id<MTLBuffer> objectBuffer,
        uint32_t objectCount,
        id<MTLBuffer> meshArgumentBuffer);
    
private:
    static bool s_initialized;
    static bool s_meshShadersSupported;
};

/**
 * @brief Common mesh shader structure definitions
 */
namespace MeshShaderStructures {
    // Object shader payload structure (data from object to mesh shader)
    typedef struct {
        uint32_t vertexCount;
        uint32_t primitiveCount;
        simd::float4 color;
    } ObjectPayload;
    
    // Mesh vertex structure (output of mesh shader)
    typedef struct {
        simd::float4 position [[position]];
        simd::float4 color;
    } MeshVertex;
    
    // Triangle indices (for mesh shader output)
    typedef struct {
        uint32_t index0;
        uint32_t index1;
        uint32_t index2;
    } TriangleIndices;
} 