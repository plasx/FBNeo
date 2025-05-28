#include <metal_stdlib>
#include <metal_math>
#include <metal_matrix>
#include <metal_geometric>
#include <metal_graphics>
#include <metal_texture>

using namespace metal;

// Shader input structure
struct VertexInput {
    float2 position [[attribute(0)]];
    float2 texCoord [[attribute(1)]];
};

// Vertex shader output/fragment shader input
struct RasterizerData {
    float4 position [[position]];
    float2 texCoord;
    float4 color;
};

// Uniform structures
struct RenderUniforms {
    float4x4 modelViewMatrix;
    float4 tint;
    float time;
    float aspectRatio;
    float scanlineIntensity;
    float curvature;
    float vignetteIntensity;
    float chromaticAberration;
    float sharpness;
    int effectMode;
};

// CRT emulation parameters
struct CRTParams {
    float mask_intensity;
    float mask_size;
    float mask_dot_width;
    float mask_dot_height;
    float curvature;
    float scanline_width;
    float scanline_intensity;
    float vignette_size;
    float vignette_intensity;
    float brightness;
    float contrast;
    float saturation;
    bool use_subpixel_layout;
};

// Vertex shader - RENAMED to avoid conflicts
vertex RasterizerData enhanced_vertexShader(VertexInput in [[stage_in]],
                                  constant RenderUniforms& uniforms [[buffer(1)]]) {
    RasterizerData out;
    
    // Transform position
    out.position = uniforms.modelViewMatrix * float4(in.position, 0.0, 1.0);
    
    // Pass texture coordinates
    out.texCoord = in.texCoord;
    
    // Pass color with tint
    out.color = uniforms.tint;
    
    return out;
}

// Standard fragment shader - RENAMED to avoid conflicts
fragment float4 enhanced_fragmentShader(RasterizerData in [[stage_in]],
                               texture2d<float> texture [[texture(0)]],
                               sampler textureSampler [[sampler(0)]],
                               constant RenderUniforms& uniforms [[buffer(1)]]) {
    // Sample the texture
    float4 color = texture.sample(textureSampler, in.texCoord);
    
    // Apply tint
    color *= in.color;
    
    return color;
}

// Utility function for RGB to HSV conversion
float3 rgb_to_hsv(float3 rgb) {
    float minVal = min(min(rgb.r, rgb.g), rgb.b);
    float maxVal = max(max(rgb.r, rgb.g), rgb.b);
    float delta = maxVal - minVal;
    
    float h = 0;
    if (delta > 0) {
        if (maxVal == rgb.r) {
            h = fmod((rgb.g - rgb.b) / delta, 6.0);
        } else if (maxVal == rgb.g) {
            h = ((rgb.b - rgb.r) / delta) + 2.0;
        } else {
            h = ((rgb.r - rgb.g) / delta) + 4.0;
        }
        h *= 60.0;
        if (h < 0) h += 360.0;
    }
    
    float s = (maxVal > 0) ? (delta / maxVal) : 0;
    float v = maxVal;
    
    return float3(h, s, v);
}

// Utility function for HSV to RGB conversion
float3 hsv_to_rgb(float3 hsv) {
    float h = hsv.x;
    float s = hsv.y;
    float v = hsv.z;
    
    float c = v * s;
    float x = c * (1 - abs(fmod(h / 60.0, 2.0) - 1));
    float m = v - c;
    
    float3 rgb;
    if (h < 60) {
        rgb = float3(c, x, 0);
    } else if (h < 120) {
        rgb = float3(x, c, 0);
    } else if (h < 180) {
        rgb = float3(0, c, x);
    } else if (h < 240) {
        rgb = float3(0, x, c);
    } else if (h < 300) {
        rgb = float3(x, 0, c);
    } else {
        rgb = float3(c, 0, x);
    }
    
    return rgb + m;
}

// Scanline effect
float3 apply_scanlines(float3 color, float2 texCoord, float intensity, float time) {
    // Dynamic scanline
    float scanline = 0.5 + 0.5 * sin(texCoord.y * 400.0 + time * 0.1);
    return color * (1.0 - intensity + intensity * scanline);
}

// CRT aperture grille effect
float3 apply_aperture_grille(float3 color, float2 texCoord, float intensity) {
    // CRT-style RGB aperture grille pattern
    float3 mask = float3(
        0.5 + 0.5 * sin(texCoord.x * 3.14159265 * 320.0),
        0.5 + 0.5 * sin(texCoord.x * 3.14159265 * 320.0 + 2.0944), // 2π/3
        0.5 + 0.5 * sin(texCoord.x * 3.14159265 * 320.0 + 4.1888)  // 4π/3
    );
    return color * (1.0 - intensity + intensity * mask);
}

// Curvature/vignette effect
float2 apply_curvature(float2 texCoord, float amount) {
    // Convert to -1.0 to 1.0 range
    float2 cc = texCoord * 2.0 - 1.0;
    
    // Apply spherical distortion
    float dist = length(cc);
    if (dist < 1.0) {
        float factor = mix(1.0, smoothstep(0.0, 1.0, 1.0 - dist), amount);
        cc = cc * factor;
    }
    
    // Convert back to 0.0 to 1.0 range
    return (cc + 1.0) * 0.5;
}

// Fragment shader with CRT effects - RENAMED to avoid conflicts
fragment float4 enhanced_crtFragmentShader(RasterizerData in [[stage_in]],
                                 texture2d<float> texture [[texture(0)]],
                                 sampler textureSampler [[sampler(0)]],
                                 constant RenderUniforms& uniforms [[buffer(1)]],
                                 constant CRTParams& crtParams [[buffer(2)]]) {
    // Apply screen curvature
    float2 texCoord = in.texCoord;
    if (crtParams.curvature > 0.0) {
        texCoord = apply_curvature(texCoord, crtParams.curvature);
    }
    
    // Check if texture coordinates are out of bounds after curvature
    if (texCoord.x < 0.0 || texCoord.x > 1.0 || texCoord.y < 0.0 || texCoord.y > 1.0) {
        return float4(0.0, 0.0, 0.0, 1.0);
    }
    
    // Apply chromatic aberration
    float3 color;
    if (uniforms.chromaticAberration > 0.0) {
        float aberration = uniforms.chromaticAberration * 0.01;
        float2 redOffset = float2(-aberration, 0.0);
        float2 greenOffset = float2(0.0, 0.0);
        float2 blueOffset = float2(aberration, 0.0);
        
        color.r = texture.sample(textureSampler, texCoord + redOffset).r;
        color.g = texture.sample(textureSampler, texCoord + greenOffset).g;
        color.b = texture.sample(textureSampler, texCoord + blueOffset).b;
    } else {
        color = texture.sample(textureSampler, texCoord).rgb;
    }
    
    // Apply scanlines
    if (crtParams.scanline_intensity > 0.0) {
        color = apply_scanlines(color, texCoord, crtParams.scanline_intensity, uniforms.time);
    }
    
    // Apply aperture grille
    if (crtParams.mask_intensity > 0.0 && crtParams.use_subpixel_layout) {
        color = apply_aperture_grille(color, texCoord, crtParams.mask_intensity);
    }
    
    // Apply vignette effect
    if (crtParams.vignette_intensity > 0.0) {
        float2 center = texCoord - 0.5;
        float vignette = 1.0 - dot(center * crtParams.vignette_size, center);
        vignette = saturate(pow(vignette, crtParams.vignette_intensity));
        color *= vignette;
    }
    
    // Apply brightness/contrast/saturation
    if (crtParams.contrast != 1.0 || crtParams.brightness != 1.0 || crtParams.saturation != 1.0) {
        // Brightness
        color *= crtParams.brightness;
        
        // Contrast
        color = (color - 0.5) * crtParams.contrast + 0.5;
        
        // Saturation
        float3 hsv = rgb_to_hsv(color);
        hsv.y *= crtParams.saturation;
        color = hsv_to_rgb(hsv);
    }
    
    return float4(color, 1.0) * in.color;
}

// Fragment shader with retro pixel art upscaling - RENAMED to avoid conflicts
fragment float4 enhanced_pixelArtFragmentShader(RasterizerData in [[stage_in]],
                                      texture2d<float> texture [[texture(0)]],
                                      sampler textureSampler [[sampler(0)]],
                                      constant RenderUniforms& uniforms [[buffer(1)]]) {
    float2 textureSize = float2(texture.get_width(), texture.get_height());
    float2 texelSize = 1.0 / textureSize;
    float2 texCoord = in.texCoord;
    
    // Sample the center pixel
    float3 center = texture.sample(textureSampler, texCoord).rgb;
    
    // Sample neighboring pixels
    float3 left = texture.sample(textureSampler, texCoord + float2(-texelSize.x, 0.0)).rgb;
    float3 right = texture.sample(textureSampler, texCoord + float2(texelSize.x, 0.0)).rgb;
    float3 top = texture.sample(textureSampler, texCoord + float2(0.0, -texelSize.y)).rgb;
    float3 bottom = texture.sample(textureSampler, texCoord + float2(0.0, texelSize.y)).rgb;
    
    // Calculate differences
    float3 diffHorizontal = abs(left - right);
    float3 diffVertical = abs(top - bottom);
    
    // Calculate difference magnitude
    float diffH = length(diffHorizontal);
    float diffV = length(diffVertical);
    
    // Apply sharpening based on edge detection
    float3 color = center;
    if (uniforms.sharpness > 0.0) {
        float edgeFactor = max(diffH, diffV) * uniforms.sharpness;
        color = mix(color, saturate(color * (1.0 + edgeFactor)), 0.5);
    }
    
    return float4(color, 1.0) * in.color;
}

// Fragment shader with MetalFX-style temporal upscaling - RENAMED to avoid conflicts
fragment float4 enhanced_temporalUpscaleFragmentShader(RasterizerData in [[stage_in]],
                                            texture2d<float> currTexture [[texture(0)]],
                                            texture2d<float> prevTexture [[texture(1)]],
                                            texture2d<float> motionVectors [[texture(2)]],
                                            sampler textureSampler [[sampler(0)]],
                                            constant RenderUniforms& uniforms [[buffer(1)]]) {
    float2 texCoord = in.texCoord;
    
    // Sample current frame
    float3 currentColor = currTexture.sample(textureSampler, texCoord).rgb;
    
    // Sample motion vectors
    float2 motion = motionVectors.sample(textureSampler, texCoord).rg;
    
    // Sample previous frame with motion offset
    float2 prevTexCoord = texCoord - motion;
    float3 previousColor = prevTexture.sample(textureSampler, prevTexCoord).rgb;
    
    // Simple temporal blend (can be improved with more sophisticated algorithms)
    float blendFactor = 0.9; // Weight for current frame
    float3 finalColor = mix(previousColor, currentColor, blendFactor);
    
    return float4(finalColor, 1.0) * in.color;
}

// Compute shader for applying scanlines - RENAMED to avoid conflicts
kernel void enhanced_applyScanlines(texture2d<float, access::read> inTexture [[texture(0)]],
                          texture2d<float, access::write> outTexture [[texture(1)]],
                          constant float& intensity [[buffer(0)]],
                          constant float& time [[buffer(1)]],
                          uint2 gid [[thread_position_in_grid]]) {
    // Check if within texture bounds
    if (gid.x >= inTexture.get_width() || gid.y >= inTexture.get_height()) {
        return;
    }
    
    // Read input color
    float4 color = inTexture.read(gid);
    
    // Apply scanline effect
    float scanline = 0.5 + 0.5 * sin(float(gid.y) * 3.14159 + time);
    color.rgb *= (1.0 - intensity + intensity * scanline);
    
    // Write output
    outTexture.write(color, gid);
}

// Compute shader for CRT effects - RENAMED to avoid conflicts
kernel void enhanced_applyCRTEffects(texture2d<float, access::read> inTexture [[texture(0)]],
                           texture2d<float, access::write> outTexture [[texture(1)]],
                           constant CRTParams& params [[buffer(0)]],
                           constant float& time [[buffer(1)]],
                           uint2 gid [[thread_position_in_grid]]) {
    // Check if within texture bounds
    if (gid.x >= inTexture.get_width() || gid.y >= inTexture.get_height()) {
        return;
    }
    
    float2 texCoord = float2(gid) / float2(inTexture.get_width(), inTexture.get_height());
    
    // Read input color
    float4 color = inTexture.read(gid);
    
    // Apply scanlines
    if (params.scanline_intensity > 0.0) {
        float scanline = 0.5 + 0.5 * sin(float(gid.y) * params.scanline_width + time);
        color.rgb *= (1.0 - params.scanline_intensity + params.scanline_intensity * scanline);
    }
    
    // Apply RGB mask/aperture grille
    if (params.mask_intensity > 0.0 && params.use_subpixel_layout) {
        float3 mask = float3(
            0.5 + 0.5 * sin(float(gid.x) * params.mask_dot_width),
            0.5 + 0.5 * sin(float(gid.x) * params.mask_dot_width + 2.0944),
            0.5 + 0.5 * sin(float(gid.x) * params.mask_dot_width + 4.1888)
        );
        color.rgb *= (1.0 - params.mask_intensity + params.mask_intensity * mask);
    }
    
    // Apply vignette
    if (params.vignette_intensity > 0.0) {
        float2 center = texCoord - 0.5;
        float vignette = 1.0 - dot(center * params.vignette_size, center);
        vignette = saturate(pow(vignette, params.vignette_intensity));
        color.rgb *= vignette;
    }
    
    // Apply brightness/contrast
    color.rgb = (color.rgb - 0.5) * params.contrast + 0.5;
    color.rgb *= params.brightness;
    
    // Write output
    outTexture.write(color, gid);
}

// Compute shader for AI-enhanced upscaling - RENAMED to avoid conflicts
kernel void enhanced_aiEnhancedUpscaling(texture2d<float, access::read> inTexture [[texture(0)]],
                               texture2d<float, access::write> outTexture [[texture(1)]],
                               texture2d<float, access::read> aiFeatures [[texture(2)]],
                               constant float2& scale [[buffer(0)]],
                               uint2 gid [[thread_position_in_grid]]) {
    // Check if within output texture bounds
    if (gid.x >= outTexture.get_width() || gid.y >= outTexture.get_height()) {
        return;
    }
    
    // Calculate input coordinates
    float2 inCoord = float2(gid) / scale;
    uint2 inPixel = uint2(inCoord);
    
    // Check if within input texture bounds
    if (inPixel.x >= inTexture.get_width() || inPixel.y >= inTexture.get_height()) {
        outTexture.write(float4(0, 0, 0, 1), gid);
        return;
    }
    
    // Read input color
    float4 color = inTexture.read(inPixel);
    
    // Read AI features if available (features guide the upscaling)
    float4 features = float4(0);
    if (gid.x < aiFeatures.get_width() && gid.y < aiFeatures.get_height()) {
        features = aiFeatures.read(gid);
    }
    
    // Enhanced color with features
    float4 enhancedColor = color;
    
    // If we have meaningful features, use them to enhance the upscale
    if (length(features.rgb) > 0.1) {
        // Edge-aware enhancement
        if (features.r > 0.5) {  // Edge detected in feature map
            // Enhance edges
            enhancedColor.rgb *= 1.2;  // Boost contrast at edges
        }
        
        // Detail preservation
        if (features.g > 0.5) {  // Detail detected in feature map
            // Preserve details
            enhancedColor.rgb = mix(color.rgb, features.rgb, 0.3);
        }
        
        // Artifact removal
        if (features.b > 0.5) {  // Potential artifact in feature map
            // Smooth out artifacts
            enhancedColor.rgb = mix(color.rgb, features.rgb, 0.5);
        }
    } else {
        // Without features, just use color
        enhancedColor = color;
    }
    
    // Write output
    outTexture.write(enhancedColor, gid);
} 