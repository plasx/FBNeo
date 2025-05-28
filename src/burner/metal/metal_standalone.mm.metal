#include <metal_stdlib>
using namespace metal;

// Vertex input/output structure
struct VertexIn {
    float4 position [[attribute(0)]];
    float2 texCoord [[attribute(1)]];
};

struct VertexOut {
    float4 position [[position]];
    float2 texCoord;
};

// Vertex shader
vertex VertexOut vertex_main(uint vertexID [[vertex_id]],
                           constant float* vertices [[buffer(0)]]) {
    VertexOut out;
    // Each vertex has 6 floats: position (4) + texcoord (2)
    out.position = float4(vertices[vertexID * 6], vertices[vertexID * 6 + 1], vertices[vertexID * 6 + 2], vertices[vertexID * 6 + 3]);
    out.texCoord = float2(vertices[vertexID * 6 + 4], vertices[vertexID * 6 + 5]);
    return out;
}

// Fragment shader - basic texture sampling
fragment float4 fragment_main(VertexOut in [[stage_in]],
                            texture2d<float> texture [[texture(0)]],
                            sampler textureSampler [[sampler(0)]]) {
    return texture.sample(textureSampler, in.texCoord);
}

// Post-processing parameters
struct PostProcessUniforms {
    float scanlineIntensity;  // Intensity of scanlines (0.0 - 1.0)
    float scanlineCount;      // Number of scanlines
    float curvature;          // Screen curvature factor
    float vignetteStrength;   // Vignette edge darkening
    float2 screenSize;        // Screen dimensions
    float2 textureSize;       // Original texture dimensions
};

// Scanline post-processing effect
fragment float4 scanline_postprocess(VertexOut in [[stage_in]],
                                 texture2d<float> texture [[texture(0)]],
                                 sampler textureSampler [[sampler(0)]],
                                 constant PostProcessUniforms &uniforms [[buffer(0)]]) {
    // Sample base texture
    float4 baseColor = texture.sample(textureSampler, in.texCoord);
    
    // Apply scanlines
    float scanlinePosition = in.texCoord.y * uniforms.screenSize.y;
    float scanlineFactor = 1.0 - abs(sin(scanlinePosition * uniforms.scanlineCount * 3.14159)) * uniforms.scanlineIntensity;
    
    // Apply vignette (darken corners)
    float2 centeredCoord = in.texCoord * 2.0 - 1.0;
    float vignette = 1.0 - length(centeredCoord) * uniforms.vignetteStrength;
    vignette = clamp(vignette, 0.0, 1.0);
    
    // Composite effects
    float4 outputColor = baseColor * scanlineFactor * vignette;
    
    return outputColor;
}

// CRT curvature effect
fragment float4 crt_postprocess(VertexOut in [[stage_in]],
                             texture2d<float> texture [[texture(0)]],
                             sampler textureSampler [[sampler(0)]],
                             constant PostProcessUniforms &uniforms [[buffer(0)]]) {
    // Apply CRT curvature distortion
    float2 centeredCoord = in.texCoord * 2.0 - 1.0;
    float distort = dot(centeredCoord, centeredCoord) * uniforms.curvature;
    centeredCoord = centeredCoord * (1.0 + distort);
    float2 finalCoord = (centeredCoord * 0.5 + 0.5);
    
    // Check if we're outside the texture bounds after distortion
    if (finalCoord.x < 0.0 || finalCoord.x > 1.0 || finalCoord.y < 0.0 || finalCoord.y > 1.0) {
        return float4(0.0, 0.0, 0.0, 1.0); // Black color for out-of-bounds
    }
    
    // Sample with the distorted coordinates
    float4 baseColor = texture.sample(textureSampler, finalCoord);
    
    // Apply scanlines
    float scanlinePosition = finalCoord.y * uniforms.screenSize.y;
    float scanlineFactor = 1.0 - abs(sin(scanlinePosition * uniforms.scanlineCount * 3.14159)) * uniforms.scanlineIntensity;
    
    // Apply vignette (darken corners)
    float vignette = 1.0 - length(centeredCoord * 0.5) * uniforms.vignetteStrength;
    vignette = clamp(vignette, 0.0, 1.0);
    
    // Composite effects
    float4 outputColor = baseColor * scanlineFactor * vignette;
    
    return outputColor;
} 