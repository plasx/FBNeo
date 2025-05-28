#include "mesh_shader_utils.h"

// Static member initialization
bool MeshShaderUtils::s_initialized = false;
bool MeshShaderUtils::s_meshShadersSupported = false;

// Mesh shader metal code - will be compiled at runtime
// In a production environment, this should be in a separate .metal file
// and compiled using offline compilation for better performance
static NSString* kMeshShaderSource = @R"(
#include <metal_stdlib>
#include <metal_meshlet>

using namespace metal;

// Object shader payload structure
struct ObjectPayload {
    uint vertexCount;
    uint primitiveCount;
    float4 color;
};

// Mesh vertex structure
struct MeshVertex {
    float4 position [[position]];
    float4 color;
};

// Triangle indices
struct TriangleIndices {
    uint index0;
    uint index1;
    uint index2;
};

// Object data structure (passed from CPU)
struct ObjectData {
    float3 position;
    float scale;
    float4 color;
    uint type; // 0 = box, 1 = sphere, 2 = arrow, etc.
};

// Object shader (first stage of mesh shader pipeline)
// Determines how many meshes to emit and their properties
[[object]]
void objectMain(
    object_data objectIndexData,
    constant ObjectData* objectData [[buffer(0)]],
    device ObjectPayload& payload [[payload]]
) {
    uint objectIndex = objectIndexData.object_id;
    ObjectData data = objectData[objectIndex];
    
    // Determine vertex and primitive counts based on object type
    uint vertexCount = 0;
    uint primitiveCount = 0;
    
    switch (data.type) {
        case 0: // Box
            vertexCount = 8;
            primitiveCount = 12; // 12 triangles (6 faces * 2 triangles)
            break;
        case 1: // Sphere approximation
            vertexCount = 26;
            primitiveCount = 48;
            break;
        case 2: // Arrow
            vertexCount = 7;
            primitiveCount = 8;
            break;
        default:
            vertexCount = 0;
            primitiveCount = 0;
            break;
    }
    
    // Set output payload values
    payload.vertexCount = vertexCount;
    payload.primitiveCount = primitiveCount;
    payload.color = data.color;
}

// Mesh shader (second stage of mesh shader pipeline)
// Generates actual vertices and triangles
[[mesh]]
void meshMain(
    mesh_grid_properties mgp,
    constant ObjectData* objectData [[buffer(0)]],
    device ObjectPayload& payload [[payload]],
    uint gid [[thread_position_in_grid]],
    meshlet_array<MeshVertex, 64> vertices,
    meshlet_array<TriangleIndices, 126> primitives
) {
    // Access object data
    uint objectIndex = mgp.object_id;
    ObjectData data = objectData[objectIndex];
    
    // Process based on object type
    switch (data.type) {
        case 0: { // Box
            // Generate box vertices (simplified for example)
            float s = data.scale * 0.5f;
            float3 p = data.position;
            
            // Vertices for a box centered at position
            float3 positions[8] = {
                float3(p.x - s, p.y - s, p.z - s), // 0: left bottom back
                float3(p.x + s, p.y - s, p.z - s), // 1: right bottom back
                float3(p.x + s, p.y + s, p.z - s), // 2: right top back
                float3(p.x - s, p.y + s, p.z - s), // 3: left top back
                float3(p.x - s, p.y - s, p.z + s), // 4: left bottom front
                float3(p.x + s, p.y - s, p.z + s), // 5: right bottom front
                float3(p.x + s, p.y + s, p.z + s), // 6: right top front
                float3(p.x - s, p.y + s, p.z + s)  // 7: left top front
            };
            
            // Add vertices to mesh output
            for (uint i = 0; i < 8; ++i) {
                vertices[i].position = float4(positions[i], 1.0);
                vertices[i].color = payload.color;
            }
            
            // Define triangle indices (12 triangles for a box)
            // Front face
            primitives[0].index0 = 4; primitives[0].index1 = 5; primitives[0].index2 = 6;
            primitives[1].index0 = 4; primitives[1].index1 = 6; primitives[1].index2 = 7;
            // Back face
            primitives[2].index0 = 0; primitives[2].index1 = 3; primitives[2].index2 = 2;
            primitives[3].index0 = 0; primitives[3].index1 = 2; primitives[3].index2 = 1;
            // Left face
            primitives[4].index0 = 0; primitives[4].index1 = 4; primitives[4].index2 = 7;
            primitives[5].index0 = 0; primitives[5].index1 = 7; primitives[5].index2 = 3;
            // Right face
            primitives[6].index0 = 1; primitives[6].index1 = 2; primitives[6].index2 = 6;
            primitives[7].index0 = 1; primitives[7].index1 = 6; primitives[7].index2 = 5;
            // Top face
            primitives[8].index0 = 3; primitives[8].index1 = 7; primitives[8].index2 = 6;
            primitives[9].index0 = 3; primitives[9].index1 = 6; primitives[9].index2 = 2;
            // Bottom face
            primitives[10].index0 = 0; primitives[10].index1 = 1; primitives[10].index2 = 5;
            primitives[11].index0 = 0; primitives[11].index1 = 5; primitives[11].index2 = 4;
            
            // Set output counts
            vertices.set_vertex_count(8);
            primitives.set_primitive_count(12);
            break;
        }
        
        case 1: { // Sphere approximation (simplified icosphere)
            // This would be a simple sphere representation
            // Simplified for example - actual implementation would have more vertices
            
            float r = data.scale * 0.5f;
            float3 p = data.position;
            
            // Set output counts for placeholder
            vertices.set_vertex_count(payload.vertexCount);
            primitives.set_primitive_count(payload.primitiveCount);
            break;
        }
        
        case 2: { // Arrow
            // Simple arrow pointing in a direction
            float s = data.scale;
            float3 p = data.position;
            
            // Set output counts for placeholder
            vertices.set_vertex_count(payload.vertexCount);
            primitives.set_primitive_count(payload.primitiveCount);
            break;
        }
        
        default:
            // No output
            vertices.set_vertex_count(0);
            primitives.set_primitive_count(0);
            break;
    }
}

// Simple vertex shader for standard pipeline (for devices without mesh shader support)
vertex MeshVertex vertexMain(
    uint vertexID [[vertex_id]],
    constant float3* positions [[buffer(0)]],
    constant float4* colors [[buffer(1)]]
) {
    MeshVertex out;
    out.position = float4(positions[vertexID], 1.0);
    out.color = colors[vertexID];
    return out;
}

// Fragment shader
fragment float4 fragmentMain(
    MeshVertex in [[stage_in]]
) {
    return in.color;
}
)";

bool MeshShaderUtils::initialize(id<MTLDevice> device) {
    if (s_initialized) {
        return true;
    }
    
    if (!device) {
        return false;
    }
    
    // Check for mesh shader support
    s_meshShadersSupported = [device supportsFamily:MTLGPUFamilyMetal3] && 
                            [device supportsFamily:MTLGPUFamilyApple8];
    
    s_initialized = true;
    return true;
}

bool MeshShaderUtils::areMeshShadersSupported() {
    return s_meshShadersSupported;
}

id<MTLRenderPipelineState> MeshShaderUtils::createMeshPipeline(
    id<MTLDevice> device,
    NSString* vertexFunction,
    NSString* fragmentFunction,
    NSString* objectFunction,
    NSString* meshFunction) {
    
    if (!s_initialized || !device) {
        return nil;
    }
    
    // Create the shader library
    id<MTLLibrary> library = createMeshShaderLibrary(device);
    if (!library) {
        NSLog(@"Failed to create mesh shader library");
        return nil;
    }
    
    // Check if mesh shaders are supported
    if (!s_meshShadersSupported) {
        NSLog(@"Mesh shaders are not supported on this device");
        [library release];
        return nil;
    }
    
    // Get shader functions
    NSError* error = nil;
    id<MTLFunction> objectShaderFunc = [library newFunctionWithName:objectFunction];
    id<MTLFunction> meshShaderFunc = [library newFunctionWithName:meshFunction];
    id<MTLFunction> fragShaderFunc = [library newFunctionWithName:fragmentFunction];
    
    if (!objectShaderFunc || !meshShaderFunc || !fragShaderFunc) {
        NSLog(@"Failed to get shader functions");
        [library release];
        return nil;
    }
    
    // Create render pipeline descriptor
    MTLRenderPipelineDescriptor* pipelineDesc = [[MTLRenderPipelineDescriptor alloc] init];
    pipelineDesc.objectFunction = objectShaderFunc;
    pipelineDesc.meshFunction = meshShaderFunc;
    pipelineDesc.fragmentFunction = fragShaderFunc;
    
    // Configure pixel format for the render target
    pipelineDesc.colorAttachments[0].pixelFormat = MTLPixelFormatRGBA8Unorm;
    
    // Create the pipeline state
    id<MTLRenderPipelineState> pipelineState = [device newRenderPipelineStateWithDescriptor:pipelineDesc error:&error];
    
    // Clean up
    [pipelineDesc release];
    [objectShaderFunc release];
    [meshShaderFunc release];
    [fragShaderFunc release];
    [library release];
    
    if (!pipelineState) {
        NSLog(@"Failed to create mesh pipeline state: %@", error);
        return nil;
    }
    
    return pipelineState;
}

id<MTLLibrary> MeshShaderUtils::createMeshShaderLibrary(id<MTLDevice> device) {
    if (!device) {
        return nil;
    }
    
    // Create library from source
    NSError* error = nil;
    MTLCompileOptions* options = [[MTLCompileOptions alloc] init];
    options.languageVersion = MTLLanguageVersion2_4; // For Metal 3
    
    id<MTLLibrary> library = [device newLibraryWithSource:kMeshShaderSource options:options error:&error];
    [options release];
    
    if (!library) {
        NSLog(@"Failed to create mesh shader library: %@", error);
        return nil;
    }
    
    return library;
}

void MeshShaderUtils::drawMeshShaders(
    id<MTLRenderCommandEncoder> encoder,
    id<MTLRenderPipelineState> pipeline,
    id<MTLBuffer> objectBuffer,
    uint32_t objectCount,
    id<MTLBuffer> meshArgumentBuffer) {
    
    if (!encoder || !pipeline || !objectBuffer || objectCount == 0) {
        return;
    }
    
    [encoder setRenderPipelineState:pipeline];
    [encoder setObjectBuffer:objectBuffer offset:0 atIndex:0];
    
    if (meshArgumentBuffer) {
        [encoder setMeshBuffer:meshArgumentBuffer offset:0 atIndex:0];
    }
    
    [encoder drawMeshThreads:MTLSizeMake(1, 1, 1) threadsPerObjectThreadgroup:MTLSizeMake(1, 1, 1) objectCount:objectCount];
} 