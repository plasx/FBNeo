#include <metal_stdlib>
using namespace metal;

// Vertex shader inputs
struct VertexInput {
    float4 position [[position]];
    float2 texCoord;
};

// Vertex shader outputs and fragment shader inputs
struct RasterizerData {
    float4 position [[position]];
    float2 texCoord;
};

// Vertex shader
vertex RasterizerData vertexShader(uint vertexID [[vertex_id]]) {
    // Define a triangle filling the screen
    const float4 positions[] = {
        float4(-1.0, -1.0, 0.0, 1.0),
        float4( 3.0, -1.0, 0.0, 1.0),
        float4(-1.0,  3.0, 0.0, 1.0)
    };
    
    const float2 texCoords[] = {
        float2(0.0, 1.0),
        float2(2.0, 1.0),
        float2(0.0, -1.0)
    };
    
    RasterizerData out;
    out.position = positions[vertexID];
    out.texCoord = texCoords[vertexID];
    
    return out;
}

// Fragment shader
fragment float4 fragmentShader(RasterizerData in [[stage_in]], 
                               texture2d<float> texture [[texture(0)]]) {
    constexpr sampler textureSampler(mag_filter::linear, min_filter::linear);
    float4 color = texture.sample(textureSampler, in.texCoord);
    return color;
}
