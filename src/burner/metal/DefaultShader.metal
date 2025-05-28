#include <metal_stdlib>
using namespace metal;

// Vertex shader input structure
struct VertexIn {
    float3 position [[attribute(0)]];
    float2 texCoord [[attribute(1)]];
};

// Vertex shader output structure
struct VertexOut {
    float4 position [[position]];
    float2 texCoord;
};

// Constants structure
struct Constants {
    float2 scale;
    float2 offset;
};

// Vertex shader
vertex VertexOut vertexShader(uint vertexID [[vertex_id]],
                             constant float* vertices [[buffer(0)]],
                             constant Constants& constants [[buffer(1)]]) {
    VertexOut out;
    
    // Get vertex position and texture coordinates
    float3 position = float3(vertices[vertexID * 5],
                           vertices[vertexID * 5 + 1],
                           vertices[vertexID * 5 + 2]);
    float2 texCoord = float2(vertices[vertexID * 5 + 3],
                           vertices[vertexID * 5 + 4]);
    
    // Apply scaling and offset
    position.xy = position.xy * constants.scale + constants.offset;
    
    // Set output
    out.position = float4(position, 1.0);
    out.texCoord = texCoord;
    
    return out;
}

// Fragment shader
fragment float4 fragmentShader(VertexOut in [[stage_in]],
                             texture2d<float> gameTexture [[texture(0)]],
                             sampler textureSampler [[sampler(0)]]) {
    // Sample texture
    float4 color = gameTexture.sample(textureSampler, in.texCoord);
    
    // Apply gamma correction
    color.rgb = pow(color.rgb, float3(1.0/2.2));
    
    return color;
}
