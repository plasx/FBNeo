#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

// Vertex shader types
struct VertexIn {
    float2 position [[attribute(0)]];
    float2 texCoord [[attribute(1)]];
    float4 color [[attribute(2)]];
};

struct VertexOut {
    float4 position [[position]];
    float2 texCoord;
    float4 color;
};

// Uniform buffer for common parameters
struct Uniforms {
    float4x4 projectionMatrix;
    float4 tint;         // RGB + alpha
    float time;          // For animated effects
    float scanlineIntensity;  // Scanline effect intensity
    float crtCurvature;       // Screen curvature amount
    float sharpenAmount;      // Image sharpening
};

// Post processing parameters structure
struct PostProcessParams {
    float scanlineIntensity;
    float scanlineWidth;
    float scanlineOffset;
    float crtCurvature;
    float vignetteStrength;
    float vignetteSmoothness;
    float2 resolution;
    float2 screenSize;
    int dynamicResolution;
};

// Vertex shader function
vertex VertexOut vertexShader(uint vertexID [[vertex_id]],
                             constant VertexIn *vertices [[buffer(0)]]) {
    VertexOut out;
    out.position = float4(vertices[vertexID].position, 0.0, 1.0);
    out.texCoord = vertices[vertexID].texCoord;
    out.color = vertices[vertexID].color;
    return out;
}

// Basic fragment shader - just sample the texture
fragment float4 fragmentShader(VertexOut in [[stage_in]],
                              texture2d<float> tex [[texture(0)]],
                              sampler texSampler [[sampler(0)]]) {
    return tex.sample(texSampler, in.texCoord);
}

// Advanced shader with CRT effects
fragment float4 crtFragmentShader(
    VertexOut in [[stage_in]],
    texture2d<float> colorTexture [[texture(0)]],
    constant Uniforms &uniforms [[buffer(1)]],
    sampler textureSampler [[sampler(0)]]
) {
    float2 texCoord = in.texCoord;
    
    // Apply CRT curvature if enabled
    if (uniforms.crtCurvature > 0.0) {
        // Convert to centered coordinates (-1 to 1)
        float2 cc = texCoord * 2.0 - 1.0;
        
        // Apply quadratic distortion
        float distort = uniforms.crtCurvature;
        float r2 = cc.x * cc.x + cc.y * cc.y;
        float f = 1.0 + r2 * distort;
        
        // Convert back to texture coordinates
        texCoord = (cc * f + 1.0) * 0.5;
        
        // If outside of texture, return black
        if (texCoord.x < 0.0 || texCoord.x > 1.0 || texCoord.y < 0.0 || texCoord.y > 1.0) {
            return float4(0.0, 0.0, 0.0, 1.0);
        }
    }
    
    // Sample main color
    float4 color = colorTexture.sample(textureSampler, texCoord);
    
    // Apply scanline effect
    if (uniforms.scanlineIntensity > 0.0) {
        float scanline = sin(texCoord.y * 250.0) * 0.5 + 0.5;
        scanline = mix(1.0, scanline, uniforms.scanlineIntensity);
        color.rgb *= scanline;
    }
    
    // Simple image sharpening
    if (uniforms.sharpenAmount > 0.0) {
        float2 texelSize = 1.0 / float2(colorTexture.get_width(), colorTexture.get_height());
        float4 n = colorTexture.sample(textureSampler, texCoord + float2(0.0, -texelSize.y));
        float4 e = colorTexture.sample(textureSampler, texCoord + float2(texelSize.x, 0.0));
        float4 s = colorTexture.sample(textureSampler, texCoord + float2(0.0, texelSize.y));
        float4 w = colorTexture.sample(textureSampler, texCoord + float2(-texelSize.x, 0.0));
        float4 sharpen = color * (1.0 + 4.0 * uniforms.sharpenAmount) - (n + e + s + w) * uniforms.sharpenAmount;
        color = mix(color, sharpen, uniforms.sharpenAmount);
    }
    
    // Apply uniform tint
    color *= uniforms.tint;
    
    return color;
}

// Pixel-perfect scaling fragment shader
fragment float4 pixelPerfectShader(
    VertexOut in [[stage_in]],
    texture2d<float> colorTexture [[texture(0)]],
    constant Uniforms &uniforms [[buffer(1)]],
    sampler textureSampler [[sampler(0)]]
) {
    // Use nearest-neighbor sampling for sharp pixels
    float4 color = colorTexture.sample(textureSampler, in.texCoord);
    
    // Apply uniform tint
    color *= uniforms.tint;
    
    return color;
}

// Shader for AI-enhanced upscaling preview
fragment float4 aiEnhancedShader(
    VertexOut in [[stage_in]],
    texture2d<float> colorTexture [[texture(0)]],
    texture2d<float> aiTexture [[texture(1)]],
    constant Uniforms &uniforms [[buffer(1)]],
    sampler textureSampler [[sampler(0)]]
) {
    // Sample from both the original texture and the AI-enhanced texture
    float4 originalColor = colorTexture.sample(textureSampler, in.texCoord);
    float4 aiColor = aiTexture.sample(textureSampler, in.texCoord);
    
    // Mix between original and AI-enhanced based on time for preview effect
    float mixFactor = (sin(uniforms.time) * 0.5 + 0.5);
    float4 color = mix(originalColor, aiColor, mixFactor);
    
    // Apply uniform tint
    color *= uniforms.tint;
    
    return color;
}

// Helper function for CRT curvature effect
float2 curveRemapUV(float2 uv, float curvature) {
    // Convert UV from 0..1 to -1..1
    uv = uv * 2.0 - 1.0;
    
    // Apply distortion
    float r2 = uv.x * uv.x + uv.y * uv.y;
    uv *= 1.0 + r2 * curvature;
    
    // Convert back to 0..1
    return (uv * 0.5 + 0.5);
}

// Helper function for scanline effect
float scanlineIntensity(float2 uv, float intensity, float width, float offset) {
    float scanline = smoothstep(width, 0.0, abs(fract(uv.y * 240.0 + offset) - 0.5));
    return mix(1.0, scanline, intensity);
}

// Helper function for vignette effect
float vignette(float2 uv, float strength, float smoothness) {
    float dist = length(uv - 0.5) * 2.0;
    return smoothstep(strength, strength - smoothness, dist);
}

// Advanced post-processing fragment shader
fragment float4 postProcessingShader(VertexOut in [[stage_in]],
                                    texture2d<float> tex [[texture(0)]],
                                    sampler texSampler [[sampler(0)]],
                                    constant PostProcessParams &params [[buffer(0)]]) {
    // Apply CRT curvature
    float2 uv = in.texCoord;
    if (params.crtCurvature > 0.0) {
        uv = curveRemapUV(uv, params.crtCurvature);
    }
    
    // Check if UV is out of bounds after curvature
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
        return float4(0.0, 0.0, 0.0, 1.0); // Black borders
    }
    
    // Sample the texture
    float4 color = tex.sample(texSampler, uv);
    
    // Apply scanlines
    if (params.scanlineIntensity > 0.0) {
        float scanline = scanlineIntensity(uv, params.scanlineIntensity, params.scanlineWidth, params.scanlineOffset);
        color.rgb *= scanline;
    }
    
    // Apply vignette effect
    if (params.vignetteStrength > 0.0) {
        float vig = vignette(uv, params.vignetteStrength, params.vignetteSmoothness);
        color.rgb *= vig;
    }
    
    // Apply slight contrast enhancement
    float avgLum = (color.r + color.g + color.b) / 3.0;
    color.rgb = mix(color.rgb, color.rgb * (1.05 - avgLum * 0.1), 0.5);
    
    // Enhance colors slightly
    color.rgb = pow(color.rgb, 0.95);
    
    return color;
}

// Dynamic resolution shader for adaptive performance
fragment float4 dynamicResolutionShader(VertexOut in [[stage_in]],
                                       texture2d<float> tex [[texture(0)]],
                                       sampler texSampler [[sampler(0)]],
                                       constant PostProcessParams &params [[buffer(0)]]) {
    // In dynamic resolution mode, we might be rendering at a lower resolution
    // and need to upscale with a sharper filter for retro look
    
    float2 uv = in.texCoord;
    
    // For nearest-neighbor upscaling, uncomment these lines:
    // float2 texSize = float2(tex.get_width(), tex.get_height());
    // float2 uvPixel = uv * texSize;
    // float2 uvPixelFloored = floor(uvPixel);
    // uv = uvPixelFloored / texSize;
    
    float4 color = tex.sample(texSampler, uv);
    
    // Apply sharpening filter
    float offset = 1.0 / params.resolution.y;
    float3 blur = tex.sample(texSampler, uv + float2(0, offset)).rgb +
                 tex.sample(texSampler, uv - float2(0, offset)).rgb +
                 tex.sample(texSampler, uv + float2(offset, 0)).rgb +
                 tex.sample(texSampler, uv - float2(offset, 0)).rgb;
    blur /= 4.0;
    
    float sharpness = 0.5;
    color.rgb = mix(color.rgb, color.rgb + (color.rgb - blur), sharpness);
    
    return color;
}

// Bloom effect shader
fragment float4 bloomShader(VertexOut in [[stage_in]],
                           texture2d<float> tex [[texture(0)]],
                           texture2d<float> bloomTex [[texture(1)]],
                           sampler texSampler [[sampler(0)]]) {
    float4 baseColor = tex.sample(texSampler, in.texCoord);
    float4 bloomColor = bloomTex.sample(texSampler, in.texCoord);
    
    // Add bloom to base color
    return baseColor + bloomColor * 0.5;
}

// Pixelation effect shader
fragment float4 pixelateShader(VertexOut in [[stage_in]],
                              texture2d<float> tex [[texture(0)]],
                              sampler texSampler [[sampler(0)]],
                              constant float2 &pixelSize [[buffer(0)]]) {
    float2 uv = in.texCoord;
    
    // Pixelate by snapping to a coarser grid
    uv = floor(uv * pixelSize) / pixelSize;
    
    return tex.sample(texSampler, uv);
}
