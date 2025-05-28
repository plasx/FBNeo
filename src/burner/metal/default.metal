#include <metal_stdlib>
using namespace metal;

struct VertexOut {
    float4 position [[position]];
    float2 texCoord;
};

// Simple vertex shader that creates a fullscreen quad
vertex VertexOut default_vertexShader(uint vertexID [[vertex_id]]) {
    VertexOut out;
    
    // Generate a quad covering the entire screen
    // Using a triangle strip with 4 vertices
    float2 position;
    
    // Calculate position and texture coordinates for each vertex
    if (vertexID == 0) {
        // Bottom left
        position = float2(-1.0, -1.0);
        out.texCoord = float2(0.0, 1.0);
    } else if (vertexID == 1) {
        // Top left
        position = float2(-1.0, 1.0);
        out.texCoord = float2(0.0, 0.0);
    } else if (vertexID == 2) {
        // Bottom right
        position = float2(1.0, -1.0);
        out.texCoord = float2(1.0, 1.0);
    } else { // vertexID == 3
        // Top right
        position = float2(1.0, 1.0);
        out.texCoord = float2(1.0, 0.0);
    }
    
    out.position = float4(position, 0.0, 1.0);
    return out;
}

// Simple fragment shader that samples from the texture
fragment float4 default_fragmentShader(VertexOut in [[stage_in]],
                                     texture2d<float> texture [[texture(0)]]) {
    constexpr sampler textureSampler(mag_filter::linear, min_filter::linear);
    float4 color = texture.sample(textureSampler, in.texCoord);
    return color;
} 