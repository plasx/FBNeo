#include <metal_stdlib>
using namespace metal;

// Input vertex data for our blit
struct VertexIn {
    float2 position  [[attribute(0)]];
    float2 texCoords [[attribute(1)]];
};

// Output from vertex shader
struct VertexOut {
    float4 position  [[position]];
    float2 texCoords;
};

// Simple vertex shader that passes along position and texture coords
vertex VertexOut vertex_main(VertexIn in [[stage_in]])
{
    VertexOut out;
    out.position = float4(in.position, 0.0, 1.0);
    out.texCoords = in.texCoords;
    return out;
}

// Simple fragment shader to sample our texture
fragment float4 fragment_main(VertexOut in [[stage_in]],
                              texture2d<float> inTexture [[texture(0)]])
{
    constexpr sampler s(address::clamp_to_edge, filter::linear);
    return inTexture.sample(s, in.texCoords);
}
