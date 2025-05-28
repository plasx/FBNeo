#include <metal_stdlib>
using namespace metal;

// Common vertex data
struct VertexOutput {
    float4 position [[position]];
    float2 texCoord;
};

struct VertexInput {
    float2 position [[attribute(0)]];
    float2 texCoord [[attribute(1)]];
};

// Uniform buffer for shader parameters
struct ShaderUniforms {
    float2 screenSize;       // Width and height of the screen
    float time;              // Current time for effects
    float scanlineIntensity; // Intensity of scanlines (0.0 - 1.0)
    float curvature;         // CRT curvature amount (0.0 - 1.0)
    float vignetteIntensity; // Intensity of vignette effect (0.0 - 1.0)
    float noiseAmount;       // Amount of CRT noise (0.0 - 1.0)
    int shaderMode;          // 0=normal, 1=CRT, 2=scanlines, 3=sharp
};

// Forward declarations
vertex VertexOutput vertexShader(uint vertexID [[vertex_id]],
                              constant VertexInput* vertices [[buffer(0)]]);
fragment float4 fragmentShader(VertexOutput in [[stage_in]],
                            texture2d<float> texture [[texture(0)]]);

// Standard vertex shader
vertex VertexOutput vertexShader(uint vertexID [[vertex_id]],
                              constant VertexInput* vertices [[buffer(0)]]) {
    VertexOutput out;
    out.position = float4(vertices[vertexID].position, 0.0, 1.0);
    out.texCoord = vertices[vertexID].texCoord;
    return out;
}

// Standard fragment shader with linear filtering
fragment float4 fragmentShader(VertexOutput in [[stage_in]],
                              texture2d<float> texture [[texture(0)]]) {
    constexpr sampler textureSampler(mag_filter::linear, min_filter::linear);
    return texture.sample(textureSampler, in.texCoord);
}

// Pixel perfect fragment shader (nearest neighbor filtering)
fragment float4 pixelPerfectShader(VertexOutput in [[stage_in]],
                                 texture2d<float> texture [[texture(0)]]) {
    constexpr sampler textureSampler(mag_filter::nearest, min_filter::nearest);
    return texture.sample(textureSampler, in.texCoord);
}

// Scanline fragment shader
fragment float4 scanlineShader(VertexOutput in [[stage_in]],
                             texture2d<float> texture [[texture(0)]],
                             constant ShaderUniforms &uniforms [[buffer(1)]]) {
    constexpr sampler textureSampler(mag_filter::linear, min_filter::linear);
    float4 color = texture.sample(textureSampler, in.texCoord);
    
    // Calculate scanline effect
    float scanline = 0.5 + 0.5 * sin(in.texCoord.y * uniforms.screenSize.y * 2.0);
    scanline = pow(scanline, 1.5); // Adjust curve of scanline
    
    // Apply scanline modulation with intensity parameter
    color.rgb *= mix(1.0, scanline, uniforms.scanlineIntensity);
    
    return color;
}

// Generate a random value based on coordinates and time
float random(float2 p, float time) {
    return fract(sin(dot(p, float2(12.9898, 78.233) + time)) * 43758.5453);
}

// CRT distortion shader
fragment float4 crtShader(VertexOutput in [[stage_in]],
                        texture2d<float> texture [[texture(0)]],
                        constant ShaderUniforms &uniforms [[buffer(1)]]) {
    // Screen aspect ratio
    float2 aspectRatio = uniforms.screenSize / max(uniforms.screenSize.x, uniforms.screenSize.y);
    
    // Apply CRT curvature
    float2 texCoord = in.texCoord;
    
    if (uniforms.curvature > 0.0) {
        // Convert to centered coordinates (-1 to 1)
        float2 centered = 2.0 * texCoord - 1.0;
        
        // Apply curve distortion
        float dist = length(centered);
        float2 curved = centered * (1.0 + uniforms.curvature * dist * dist);
        
        // Convert back to texture coordinates (0 to 1)
        texCoord = (curved + 1.0) * 0.5;
        
        // Skip pixels outside the curved area
        if (texCoord.x < 0.0 || texCoord.x > 1.0 || texCoord.y < 0.0 || texCoord.y > 1.0) {
            return float4(0.0, 0.0, 0.0, 1.0);
        }
    }
    
    // Sample the texture
    constexpr sampler textureSampler(mag_filter::linear, min_filter::linear);
    float4 color = texture.sample(textureSampler, texCoord);
    
    // Apply scanline effect with modulated intensity
    float scanlineY = texCoord.y * uniforms.screenSize.y;
    float scanline = 0.5 + 0.5 * sin(scanlineY * 3.14159 * 2.0);
    scanline = pow(scanline, 1.5); // Adjust curve of scanline
    color.rgb *= mix(1.0, scanline, uniforms.scanlineIntensity);
    
    // Add RGB mask pattern for aperture grille
    float2 pixelCoord = texCoord * uniforms.screenSize;
    int pixelX = int(fmod(pixelCoord.x, 3.0));
    float3 mask = float3(1.0, 1.0, 1.0);
    
    // RGB subpixel simulation
    if (pixelX == 0) mask = float3(1.0, 0.7, 0.7); // R stronger
    else if (pixelX == 1) mask = float3(0.7, 1.0, 0.7); // G stronger
    else mask = float3(0.7, 0.7, 1.0); // B stronger
    
    color.rgb *= mix(float3(1.0), mask, 0.3);
    
    // Apply vignette effect
    if (uniforms.vignetteIntensity > 0.0) {
        float2 center = float2(0.5, 0.5);
        float dist = distance(center, texCoord);
        float vignette = 1.0 - dist * uniforms.vignetteIntensity * 2.0;
        vignette = clamp(vignette, 0.0, 1.0);
        vignette = pow(vignette, 0.8); // Soften the vignette
        color.rgb *= vignette;
    }
    
    // Apply CRT noise
    if (uniforms.noiseAmount > 0.0) {
        float noise = random(texCoord, uniforms.time) * 0.1 - 0.05;
        color.rgb += noise * uniforms.noiseAmount;
    }
    
    // Apply slight bloom/glow
    float4 blurredColor = texture.sample(textureSampler, texCoord, level(2.0));
    color.rgb += blurredColor.rgb * 0.1;
    
    // Apply slight gamma correction to make it look more CRT-like
    color.rgb = pow(color.rgb, float3(1.2));
    
    return color;
}

// Alternative entry points with different names for compatibility
vertex VertexOutput basic_vertexShader(uint vertexID [[vertex_id]],
                                     constant VertexInput* vertices [[buffer(0)]]) {
    VertexOutput result;
    result.position = float4(vertices[vertexID].position, 0.0, 1.0);
    result.texCoord = vertices[vertexID].texCoord;
    return result;
}

fragment float4 basic_fragmentShader(VertexOutput in [[stage_in]],
                                   texture2d<float> texture [[texture(0)]]) {
    constexpr sampler textureSampler(mag_filter::linear, min_filter::linear);
    return texture.sample(textureSampler, in.texCoord);
}

fragment float4 default_fragmentShader(VertexOutput in [[stage_in]],
                                     texture2d<float> texture [[texture(0)]]) {
    constexpr sampler textureSampler(mag_filter::linear, min_filter::linear);
    return texture.sample(textureSampler, in.texCoord);
}

vertex VertexOutput default_vertexShader(uint vertexID [[vertex_id]],
                                       constant VertexInput* vertices [[buffer(0)]]) {
    VertexOutput result;
    result.position = float4(vertices[vertexID].position, 0.0, 1.0);
    result.texCoord = vertices[vertexID].texCoord;
    return result;
} 