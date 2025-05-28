#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

// Uniform buffer structure - must match ShaderUniforms in metal_renderer.mm
struct Uniforms {
    float scanlineIntensity;
    float crtCurvature;
    float textureWidth;
    float textureHeight;
    float time;
    float padding[3];
};

// Input vertex structure
struct VertexInput {
    float2 position [[attribute(0)]];
    float2 texCoord [[attribute(1)]];
};

// Output vertex structure (passed to fragment shader)
struct VertexOutput {
    float4 position [[position]];
    float2 texCoord;
};

// Vertex shader
vertex VertexOutput vertexShader(VertexInput in [[stage_in]]) {
    VertexOutput out;
    out.position = float4(in.position, 0.0, 1.0);
    out.texCoord = in.texCoord;
    return out;
}

// Fragment shader for regular rendering
fragment float4 fragmentShader(VertexOutput in [[stage_in]],
                              texture2d<float> texture [[texture(0)]],
                              constant Uniforms& uniforms [[buffer(0)]]) {
    constexpr sampler textureSampler(mag_filter::linear, min_filter::linear);
    return texture.sample(textureSampler, in.texCoord);
}

// Fragment shader with scanlines effect
fragment float4 fragmentShaderScanlines(VertexOutput in [[stage_in]],
                                       texture2d<float> texture [[texture(0)]],
                                       constant Uniforms& uniforms [[buffer(0)]]) {
    constexpr sampler textureSampler(mag_filter::linear, min_filter::linear);
    
    float4 color = texture.sample(textureSampler, in.texCoord);
    
    // Get parameters from uniforms
    float scanlineIntensity = uniforms.scanlineIntensity;
    float textureHeight = uniforms.textureHeight;
    
    // Calculate scanline effect (darker horizontal lines)
    float scanline = sin(in.texCoord.y * textureHeight * 2.0) * 0.5 + 0.5;
    scanline = (scanline * 0.5 + 0.5) * scanlineIntensity;
    
    // Apply scanline darkening
    color.rgb *= 1.0 - scanline;
    
    return color;
}

// Fragment shader with CRT effect
fragment float4 fragmentShaderCRT(VertexOutput in [[stage_in]],
                                 texture2d<float> texture [[texture(0)]],
                                 constant Uniforms& uniforms [[buffer(0)]]) {
    constexpr sampler textureSampler(mag_filter::linear, min_filter::linear);
    
    // Get parameters from uniforms
    float curvature = uniforms.crtCurvature;
    float textureHeight = uniforms.textureHeight;
    float time = uniforms.time;
    
    // Apply barrel distortion based on curvature
    float2 uv = in.texCoord * 2.0 - 1.0; // Convert to -1 to 1 range
    float r2 = uv.x * uv.x + uv.y * uv.y;
    uv *= 1.0 + curvature * r2;
    float2 texCoord = (uv * 0.5 + 0.5); // Convert back to 0-1 range
    
    // Check if we're outside the texture
    if (texCoord.x < 0.0 || texCoord.x > 1.0 || texCoord.y < 0.0 || texCoord.y > 1.0) {
        return float4(0.0, 0.0, 0.0, 1.0); // Black for outside the screen
    }
    
    // RGB separation (subtle chromatic aberration)
    float aberration = 0.004;
    float4 colorR = texture.sample(textureSampler, float2(texCoord.x + aberration, texCoord.y));
    float4 colorG = texture.sample(textureSampler, texCoord);
    float4 colorB = texture.sample(textureSampler, float2(texCoord.x - aberration, texCoord.y));
    
    float4 color = float4(colorR.r, colorG.g, colorB.b, colorG.a);
    
    // Scanlines - use parameter from uniforms
    float scanlineIntensity = uniforms.scanlineIntensity;
    float scanline = sin(texCoord.y * textureHeight * 2.0) * 0.5 + 0.5;
    scanline = (scanline * 0.5 + 0.5) * scanlineIntensity;
    color.rgb *= 1.0 - scanline;
    
    // Add subtle CRT flicker based on time
    float flicker = 1.0 + 0.015 * sin(time * 5.0);
    color.rgb *= flicker;
    
    // Vignette (darker corners)
    float vignette = 1.0 - r2 * 0.5;
    color.rgb *= vignette;
    
    // Brightness and contrast
    color.rgb = color.rgb * 1.1 - 0.03;
    
    return color;
}

// Fragment shader for pixel-perfect scaling
fragment float4 fragmentShaderPixelPerfect(VertexOutput in [[stage_in]],
                                         texture2d<float> texture [[texture(0)]],
                                         constant Uniforms& uniforms [[buffer(0)]]) {
    constexpr sampler textureSampler(mag_filter::nearest, min_filter::nearest);
    return texture.sample(textureSampler, in.texCoord);
}

// Fragment shader for debug overlay
fragment float4 fragmentShaderDebug(VertexOutput in [[stage_in]],
                                   texture2d<float> texture [[texture(0)]],
                                   texture2d<float> debugTexture [[texture(1)]],
                                   sampler texSampler [[sampler(0)]],
                                   constant float* uniforms [[buffer(0)]]) {
    // Get main texture color
    float4 color = texture.sample(texSampler, in.texCoord);
    
    // Get debug texture color
    float4 debugColor = debugTexture.sample(texSampler, in.texCoord);
    
    // Show debug overlay in top-right corner
    float2 debugArea = step(float2(0.7, 0.0), in.texCoord) * step(in.texCoord, float2(1.0, 0.3));
    float inDebugArea = debugArea.x * debugArea.y;
    
    // Mix colors based on whether we're in debug area
    return mix(color, debugColor, inDebugArea);
}

// Bloom effect - horizonal blur pass
kernel void horizontalBlur(texture2d<float, access::read> inTexture [[texture(0)]],
                          texture2d<float, access::write> outTexture [[texture(1)]],
                          uint2 gid [[thread_position_in_grid]]) {
    if (gid.x >= outTexture.get_width() || gid.y >= outTexture.get_height()) {
        return;
    }
    
    float weights[5] = {0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216};
    float3 result = inTexture.read(gid).rgb * weights[0];
    
    for (int i = 1; i < 5; ++i) {
        uint2 offset1 = uint2(i, 0);
        uint2 offset2 = uint2(i, 0);
        
        if (gid.x + unsigned(i) < inTexture.get_width()) {
            result += inTexture.read(gid + offset1).rgb * weights[i];
        }
        
        if (gid.x >= unsigned(i)) {
            result += inTexture.read(gid - offset2).rgb * weights[i];
        }
    }
    
    outTexture.write(float4(result, 1.0), gid);
}

// Bloom effect - vertical blur pass
kernel void verticalBlur(texture2d<float, access::read> inTexture [[texture(0)]],
                        texture2d<float, access::write> outTexture [[texture(1)]],
                        uint2 gid [[thread_position_in_grid]]) {
    if (gid.x >= outTexture.get_width() || gid.y >= outTexture.get_height()) {
        return;
    }
    
    float weights[5] = {0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216};
    float3 result = inTexture.read(gid).rgb * weights[0];
    
    for (int i = 1; i < 5; ++i) {
        uint2 offset1 = uint2(0, i);
        uint2 offset2 = uint2(0, i);
        
        if (gid.y + unsigned(i) < inTexture.get_height()) {
            result += inTexture.read(gid + offset1).rgb * weights[i];
        }
        
        if (gid.y >= unsigned(i)) {
            result += inTexture.read(gid - offset2).rgb * weights[i];
        }
    }
    
    outTexture.write(float4(result, 1.0), gid);
}

// Combine the original image with the bloom effect
fragment float4 bloomCombine(VertexOutput in [[stage_in]],
                            texture2d<float> originalTexture [[texture(0)]],
                            texture2d<float> bloomTexture [[texture(1)]]) {
    constexpr sampler textureSampler(mag_filter::linear, min_filter::linear);
    
    float4 original = originalTexture.sample(textureSampler, in.texCoord);
    float4 bloom = bloomTexture.sample(textureSampler, in.texCoord);
    
    // Combine the original image with the bloom
    float bloomStrength = 0.5;
    return original + (bloom * bloomStrength);
}

// Compute shader for applying scanlines
kernel void scanlineCompute(texture2d<float, access::read> inputTexture [[texture(0)]],
                           texture2d<float, access::write> outputTexture [[texture(1)]],
                           constant float &intensity [[buffer(0)]],
                           uint2 gid [[thread_position_in_grid]]) {
    // Check if the thread is within the texture bounds
    if (gid.x >= inputTexture.get_width() || gid.y >= inputTexture.get_height()) {
        return;
    }
    
    // Read the input color
    float4 color = inputTexture.read(gid);
    
    // Apply scanline effect based on Y position (even/odd lines)
    float scanline = 1.0;
    if (gid.y % 2 == 0) {
        scanline = 1.0 - intensity;
    }
    
    // Apply the scanline effect to the color
    color.rgb *= scanline;
    
    // Write the output color
    outputTexture.write(color, gid);
}

// Compute shader for smart upscaling
kernel void upscaleCompute(texture2d<float, access::read> inputTexture [[texture(0)]],
                          texture2d<float, access::write> outputTexture [[texture(1)]],
                          uint2 gid [[thread_position_in_grid]]) {
    // Check if the thread is within the texture bounds
    if (gid.x >= outputTexture.get_width() || gid.y >= outputTexture.get_height()) {
        return;
    }
    
    // Calculate input texture coordinates
    float2 inputSize = float2(inputTexture.get_width(), inputTexture.get_height());
    float2 outputSize = float2(outputTexture.get_width(), outputTexture.get_height());
    float2 ratio = inputSize / outputSize;
    
    float2 inputCoord = float2(gid) * ratio;
    uint2 inputPixel = uint2(inputCoord);
    
    // Simple bilinear interpolation
    float2 frac = fract(inputCoord);
    
    uint2 inputPixel2 = uint2(min(inputPixel.x + 1, inputTexture.get_width() - 1), 
                             min(inputPixel.y + 1, inputTexture.get_height() - 1));
    
    float4 colorTL = inputTexture.read(uint2(inputPixel.x, inputPixel.y));
    float4 colorTR = inputTexture.read(uint2(inputPixel2.x, inputPixel.y));
    float4 colorBL = inputTexture.read(uint2(inputPixel.x, inputPixel2.y));
    float4 colorBR = inputTexture.read(uint2(inputPixel2.x, inputPixel2.y));
    
    // Bilinear interpolation
    float4 colorT = mix(colorTL, colorTR, frac.x);
    float4 colorB = mix(colorBL, colorBR, frac.x);
    float4 color = mix(colorT, colorB, frac.y);
    
    // Apply sharpening for pixel art (xBR-like effect)
    if (length(frac - 0.5) < 0.5) {
        // When close to the center of the pixel, enhance edges
        float4 dxColor = abs(colorTR - colorTL) + abs(colorBR - colorBL);
        float4 dyColor = abs(colorBL - colorTL) + abs(colorBR - colorTR);
        
        float dx = dxColor.r + dxColor.g + dxColor.b;
        float dy = dyColor.r + dyColor.g + dyColor.b;
        
        // Enhance horizontal edges
        if (dx > dy && dx > 0.05) {
            color = (frac.y < 0.5) ? colorT : colorB;
        }
        // Enhance vertical edges
        else if (dy > dx && dy > 0.05) {
            color = (frac.x < 0.5) ? colorTL : colorTR;
        }
    }
    
    // Write the output color
    outputTexture.write(color, gid);
}

// Overlay compute shader for debug info
kernel void debugOverlayCompute(texture2d<float, access::read> inputTexture [[texture(0)]],
                              texture2d<float, access::write> outputTexture [[texture(1)]],
                              device const float4 *debugRects [[buffer(0)]],
                              device const uint &rectCount [[buffer(1)]],
                              uint2 gid [[thread_position_in_grid]]) {
    // Check if the thread is within the texture bounds
    if (gid.x >= outputTexture.get_width() || gid.y >= outputTexture.get_height()) {
        return;
    }
    
    // Start with the input color
    float4 color = inputTexture.read(gid);
    
    // Check if this pixel is inside any debug rect
    for (uint i = 0; i < rectCount; i++) {
        float4 rect = debugRects[i];
        
        // Rect format: x, y, width, height
        if (gid.x >= rect.x && gid.x < rect.x + rect.z &&
            gid.y >= rect.y && gid.y < rect.y + rect.w) {
            
            // Draw rect outline
            if (gid.x == rect.x || gid.x == rect.x + rect.z - 1 ||
                gid.y == rect.y || gid.y == rect.y + rect.w - 1) {
                // Different colors for different types of rects (hitboxes, collision, etc.)
                if (i % 3 == 0) {
                    color = float4(1.0, 0.0, 0.0, 0.7); // Red for hitboxes
                } else if (i % 3 == 1) {
                    color = float4(0.0, 1.0, 0.0, 0.7); // Green for collision
                } else {
                    color = float4(0.0, 0.0, 1.0, 0.7); // Blue for other
                }
            }
        }
    }
    
    // Write the output color
    outputTexture.write(color, gid);
} 