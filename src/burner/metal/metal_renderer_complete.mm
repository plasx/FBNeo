#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#include "metal_renderer_defines.h"

// Forward declarations for C functions
extern "C" {
    void UpdateMetalFrameTexture(const void* frameData, unsigned int width, unsigned int height);
    int MetalRenderer_Init(void* view);
    void MetalRenderer_Exit();
    void MetalRenderer_Render(void* commandBuffer);
    void MetalRenderer_SetAspectRatio(int nWidth, int nHeight);
    void MetalRenderer_SetShaderType(int shaderType);
    void MetalRenderer_SetPreserveAspectRatio(int preserve);
    void MetalRenderer_SetScanlineIntensity(float intensity);
    void MetalRenderer_SetCRTCurvature(float curvature);
    void MetalRenderer_SetSharpness(float sharpness);
}

// Shader types
typedef NS_ENUM(NSInteger, ShaderType) {
    ShaderTypeStandard = 0,     // Standard bilinear filtering
    ShaderTypeCRT = 1,          // CRT emulation
    ShaderTypePixelPerfect = 2, // Pixel-perfect (nearest neighbor)
    ShaderTypeScanlines = 3,    // Scanlines effect
    ShaderTypeEnhanced = 4      // AI-enhanced upscaling (if available)
};

// Metal renderer implementation
@interface FBNeoMetalRenderer : NSObject <MTKViewDelegate>

// Core Metal objects
@property (nonatomic, strong) id<MTLDevice> device;
@property (nonatomic, strong) id<MTLCommandQueue> commandQueue;
@property (nonatomic, strong) id<MTLRenderPipelineState> pipelineState;
@property (nonatomic, strong) id<MTLLibrary> shaderLibrary;
@property (nonatomic, weak) MTKView *metalView;

// Frame buffer texture
@property (nonatomic, strong) id<MTLTexture> frameTexture;
@property (nonatomic, strong) id<MTLBuffer> vertexBuffer;
@property (nonatomic, assign) CGSize textureSize;

// Aspect ratio handling
@property (nonatomic, assign) CGSize originalSize;
@property (nonatomic, assign) CGSize aspectRatio;
@property (nonatomic, assign) BOOL preserveAspectRatio;

// Shader options
@property (nonatomic, assign) int shaderType;
@property (nonatomic, assign) BOOL bilinearFiltering;
@property (nonatomic, assign) float scanlineIntensity;
@property (nonatomic, assign) float crtCurvature;
@property (nonatomic, assign) float sharpness;

// Initialize with a Metal view
- (instancetype)initWithMetalView:(MTKView *)view;

// Update the frame texture with new data
- (void)updateTextureWithFrameData:(const void *)frameData width:(unsigned int)width height:(unsigned int)height;

// Set shader options
- (void)setShaderType:(int)type;
- (void)setScanlineIntensity:(float)intensity;
- (void)setCRTCurvature:(float)curvature;
- (void)setSharpness:(float)sharpness;

// Update vertex buffer for current aspect ratio and view size
- (void)updateVertexBufferForAspectRatio;

// Set aspect ratio preservation
- (void)setPreserveAspectRatio:(BOOL)preserve;

// Color format conversion methods
- (void)convertRGB888ToRGBA8888:(const void *)srcData size:(size_t)size destData:(void *)destData;
- (void)convertRGB565ToRGBA8888:(const void *)srcData size:(size_t)size destData:(void *)destData pitch:(unsigned int)pitch;
- (void)convertRGB555ToRGBA8888:(const void *)srcData size:(size_t)size destData:(void *)destData pitch:(unsigned int)pitch;

// Create texture with current dimensions
- (void)createTexture;

@end

@implementation FBNeoMetalRenderer

- (instancetype)initWithMetalView:(MTKView *)view {
    self = [super init];
    if (self) {
        _metalView = view;
        _device = view.device;
        
        if (!_device) {
            NSLog(@"Metal device not available");
            return nil;
        }
        
        // Create command queue
        _commandQueue = [_device newCommandQueue];
        if (!_commandQueue) {
            NSLog(@"Failed to create command queue");
            return nil;
        }
        
        // Set default properties
        _textureSize = CGSizeMake(320, 240);
        _originalSize = _textureSize;
        _aspectRatio = _textureSize;
        _preserveAspectRatio = YES;
        _shaderType = ShaderTypeStandard;
        _bilinearFiltering = YES;
        _scanlineIntensity = 0.15f;
        _crtCurvature = 0.1f;
        _sharpness = 0.5f;
        
        // Load shader library
        NSError *error = nil;
        
        // First try to load from app bundle
        NSBundle *bundle = [NSBundle mainBundle];
        NSURL *libraryURL = [bundle URLForResource:@"fbneo_shaders" withExtension:@"metallib"];
        
        if (libraryURL) {
            _shaderLibrary = [_device newLibraryWithURL:libraryURL error:&error];
        }
        
        // If that fails, try loading from default.metallib
        if (!_shaderLibrary) {
            libraryURL = [bundle URLForResource:@"default" withExtension:@"metallib"];
            if (libraryURL) {
                _shaderLibrary = [_device newLibraryWithURL:libraryURL error:&error];
            }
        }
        
        // If still no library, try embedded source
        if (!_shaderLibrary) {
            NSLog(@"Failed to load metallib from disk, using default shader");
            
            // Create default shaders
            NSString *shaderSource = @"#include <metal_stdlib>\n"
                                    "using namespace metal;\n"
                                    "\n"
                                    "struct VertexOut {\n"
                                    "    float4 position [[position]];\n"
                                    "    float2 texCoord;\n"
                                    "};\n"
                                    "\n"
                                    "vertex VertexOut default_vertexShader(uint vid [[vertex_id]],\n"
                                    "                                     constant float4 *vertices [[buffer(0)]]) {\n"
                                    "    VertexOut out;\n"
                                    "    out.position = float4(vertices[vid].xy, 0.0, 1.0);\n"
                                    "    out.texCoord = vertices[vid].zw;\n"
                                    "    return out;\n"
                                    "}\n"
                                    "\n"
                                    "fragment float4 default_fragmentShader(VertexOut in [[stage_in]],\n"
                                    "                                      texture2d<float> tex [[texture(0)]],\n"
                                    "                                      sampler texSampler [[sampler(0)]]) {\n"
                                    "    return tex.sample(texSampler, in.texCoord);\n"
                                    "}\n"
                                    "\n"
                                    "// Enhanced versions to avoid conflicts\n"
                                    "vertex VertexOut enhanced_vertexShader(uint vid [[vertex_id]],\n"
                                    "                                     constant float4 *vertices [[buffer(0)]]) {\n"
                                    "    VertexOut out;\n"
                                    "    out.position = float4(vertices[vid].xy, 0.0, 1.0);\n"
                                    "    out.texCoord = vertices[vid].zw;\n"
                                    "    return out;\n"
                                    "}\n"
                                    "\n"
                                    "fragment float4 enhanced_fragmentShader(VertexOut in [[stage_in]],\n"
                                    "                                      texture2d<float> tex [[texture(0)]],\n"
                                    "                                      sampler texSampler [[sampler(0)]]) {\n"
                                    "    return tex.sample(texSampler, in.texCoord);\n"
                                    "}\n";
            
            _shaderLibrary = [_device newLibraryWithSource:shaderSource options:nil error:&error];
        }
        
        if (!_shaderLibrary) {
            NSLog(@"Failed to create Metal shader library: %@", error);
            return nil;
        }
        
        // Create vertex buffer
        [self createVertexBuffer];
        
        // Create the render pipeline
        [self createRenderPipeline];
        
        // Set up the view delegate
        _metalView.delegate = self;
        _metalView.paused = NO;
        _metalView.enableSetNeedsDisplay = YES;
        
        // Create initial texture
        [self createTexture];
    }
    return self;
}

- (void)setupMetal {
    // Create command queue
    _commandQueue = [_device newCommandQueue];
    
    // Create shader library - first try to load from disk, then fallback to embedded
    NSError *error = nil;
    NSURL *libraryURL = [[NSBundle mainBundle] URLForResource:@"fbneo_shaders" withExtension:@"metallib"];
    
    if (libraryURL) {
        _shaderLibrary = [_device newLibraryWithURL:libraryURL error:&error];
    }
    
    // If that fails, fall back to the default compiled shader
    if (!_shaderLibrary) {
        NSLog(@"Failed to load metallib from disk, using default shader");
        
        // Create default shaders
        NSString *shaderSource = @"#include <metal_stdlib>\n"
                                "using namespace metal;\n"
                                "\n"
                                "struct VertexOut {\n"
                                "    float4 position [[position]];\n"
                                "    float2 texCoord;\n"
                                "};\n"
                                "\n"
                                "vertex VertexOut default_vertexShader(uint vid [[vertex_id]],\n"
                                "                                     constant float4 *vertices [[buffer(0)]]) {\n"
                                "    VertexOut out;\n"
                                "    out.position = float4(vertices[vid].xy, 0.0, 1.0);\n"
                                "    out.texCoord = vertices[vid].zw;\n"
                                "    return out;\n"
                                "}\n"
                                "\n"
                                "fragment float4 default_fragmentShader(VertexOut in [[stage_in]],\n"
                                "                                      texture2d<float> tex [[texture(0)]],\n"
                                "                                      sampler texSampler [[sampler(0)]]) {\n"
                                "    return tex.sample(texSampler, in.texCoord);\n"
                                "}\n";
        
        _shaderLibrary = [_device newLibraryWithSource:shaderSource options:nil error:&error];
    }
    
    if (!_shaderLibrary) {
        NSLog(@"Failed to create Metal shader library: %@", error);
        return;
    }
    
    // Create the render pipeline
    [self createRenderPipeline];
    
    // Set up the view delegate
    _metalView.delegate = self;
    _metalView.paused = NO;
    _metalView.enableSetNeedsDisplay = YES;
}

- (void)createRenderPipeline {
    NSError *error = nil;
    
    // Get shader functions based on shader type
    id<MTLFunction> vertexFunction = nil;
    id<MTLFunction> fragmentFunction = nil;
    
    // Select the appropriate shaders from the library
    switch (_shaderType) {
        case METAL_SHADER_CRT:
            vertexFunction = [_shaderLibrary newFunctionWithName:@"enhanced_vertexShader"];
            fragmentFunction = [_shaderLibrary newFunctionWithName:@"enhanced_crtFragmentShader"];
            break;
            
        case METAL_SHADER_SCANLINES:
            vertexFunction = [_shaderLibrary newFunctionWithName:@"enhanced_vertexShader"];
            fragmentFunction = [_shaderLibrary newFunctionWithName:@"enhanced_fragmentShader"];
            break;
            
        case METAL_SHADER_HQ2X:
            vertexFunction = [_shaderLibrary newFunctionWithName:@"enhanced_vertexShader"];
            fragmentFunction = [_shaderLibrary newFunctionWithName:@"enhanced_pixelArtFragmentShader"];
            break;
            
        case METAL_SHADER_BASIC:
        default:
            vertexFunction = [_shaderLibrary newFunctionWithName:@"enhanced_vertexShader"];
            fragmentFunction = [_shaderLibrary newFunctionWithName:@"enhanced_fragmentShader"];
            break;
    }
    
    // If failed to get functions, fall back to defaults
    if (!vertexFunction || !fragmentFunction) {
        vertexFunction = [_shaderLibrary newFunctionWithName:@"default_vertexShader"];
        fragmentFunction = [_shaderLibrary newFunctionWithName:@"default_fragmentShader"];
        
        if (!vertexFunction || !fragmentFunction) {
            NSLog(@"Failed to find shader functions - renderer may not work correctly");
            return;
        }
    }
    
    // Create pipeline descriptor
    MTLRenderPipelineDescriptor *pipelineDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
    pipelineDescriptor.vertexFunction = vertexFunction;
    pipelineDescriptor.fragmentFunction = fragmentFunction;
    pipelineDescriptor.colorAttachments[0].pixelFormat = _metalView.colorPixelFormat;
    
    // Enable blending
    pipelineDescriptor.colorAttachments[0].blendingEnabled = YES;
    pipelineDescriptor.colorAttachments[0].rgbBlendOperation = MTLBlendOperationAdd;
    pipelineDescriptor.colorAttachments[0].alphaBlendOperation = MTLBlendOperationAdd;
    pipelineDescriptor.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
    pipelineDescriptor.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorSourceAlpha;
    pipelineDescriptor.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
    pipelineDescriptor.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
    
    // Create pipeline state
    _pipelineState = [_device newRenderPipelineStateWithDescriptor:pipelineDescriptor error:&error];
    
    if (!_pipelineState) {
        NSLog(@"Failed to create render pipeline state: %@", error);
    }
}

- (void)createVertexBuffer {
    // Default quad filling the screen - will be updated when aspect ratio changes
    float quadVertices[] = {
        // Positions       Texture coordinates
        -1.0,  1.0,       0.0, 0.0,  // top left
        -1.0, -1.0,       0.0, 1.0,  // bottom left
         1.0, -1.0,       1.0, 1.0,  // bottom right
         
         1.0, -1.0,       1.0, 1.0,  // bottom right
         1.0,  1.0,       1.0, 0.0,  // top right
        -1.0,  1.0,       0.0, 0.0,  // top left
    };
    
    _vertexBuffer = [_device newBufferWithBytes:quadVertices
                                         length:sizeof(quadVertices)
                                        options:MTLResourceStorageModeShared];
}

- (void)updateVertexBufferForAspectRatio {
    if (!_device || !_metalView) {
        return;
    }
    
    CGSize viewSize = _metalView.drawableSize;
    CGSize gameSize = _textureSize.width > 0 && _textureSize.height > 0 ? 
                      _textureSize : CGSizeMake(320, 240);
    
    if (viewSize.width <= 0 || viewSize.height <= 0 || 
        gameSize.width <= 0 || gameSize.height <= 0) {
        return;
    }
    
    // Calculate the aspect ratios
    float viewAspect = viewSize.width / viewSize.height;
    float gameAspect = gameSize.width / gameSize.height;
    
    // Default quad fills the entire view
    float quadVertices[24] = {
        // Positions       Texture coordinates
        -1.0,  1.0,       0.0, 0.0,  // top left
        -1.0, -1.0,       0.0, 1.0,  // bottom left
         1.0, -1.0,       1.0, 1.0,  // bottom right
         
         1.0, -1.0,       1.0, 1.0,  // bottom right
         1.0,  1.0,       1.0, 0.0,  // top right
        -1.0,  1.0,       0.0, 0.0,  // top left
    };
    
    if (_preserveAspectRatio) {
        // Calculate scaling to maintain aspect ratio
        float scale;
        float xOffset = 0.0f;
        float yOffset = 0.0f;
        
        if (gameAspect > viewAspect) {
            // Game is wider than view, letterbox top/bottom
            scale = viewSize.width / gameSize.width;
            float scaledHeight = gameSize.height * scale;
            yOffset = (viewSize.height - scaledHeight) / 2.0f / viewSize.height;
            
            // Update quad positions for letterboxing
            float yPos = 1.0f - 2.0f * yOffset;
            
            // Top-left, bottom-left, bottom-right
            quadVertices[1] = yPos;         // top-left Y
            quadVertices[5] = -yPos;        // bottom-left Y
            quadVertices[9] = -yPos;        // bottom-right Y
            
            // Bottom-right, top-right, top-left
            quadVertices[13] = -yPos;       // bottom-right Y
            quadVertices[17] = yPos;        // top-right Y
            quadVertices[21] = yPos;        // top-left Y
        } else {
            // Game is taller than view, letterbox left/right
            scale = viewSize.height / gameSize.height;
            float scaledWidth = gameSize.width * scale;
            xOffset = (viewSize.width - scaledWidth) / 2.0f / viewSize.width;
            
            // Update quad positions for letterboxing
            float xPos = 1.0f - 2.0f * xOffset;
            
            // Top-left, bottom-left, bottom-right
            quadVertices[0] = -xPos;        // top-left X
            quadVertices[4] = -xPos;        // bottom-left X
            quadVertices[8] = xPos;         // bottom-right X
            
            // Bottom-right, top-right, top-left
            quadVertices[12] = xPos;        // bottom-right X
            quadVertices[16] = xPos;        // top-right X
            quadVertices[20] = -xPos;       // top-left X
        }
    }
    
    // Create or update the vertex buffer
    if (!_vertexBuffer) {
        _vertexBuffer = [_device newBufferWithBytes:quadVertices
                                            length:sizeof(quadVertices)
                                           options:MTLResourceStorageModeShared];
    } else {
        void* bufferPtr = [_vertexBuffer contents];
        memcpy(bufferPtr, quadVertices, sizeof(quadVertices));
    }
}

- (void)updateTextureWithFrameData:(const void *)frameData width:(unsigned int)width height:(unsigned int)height {
    if (!frameData || width == 0 || height == 0) {
        NSLog(@"Invalid frame data or dimensions");
        return;
    }
    
    // Store original dimensions for aspect ratio calculations
    if (_originalSize.width != width || _originalSize.height != height) {
        _originalSize = CGSizeMake(width, height);
        _textureSize = _originalSize;
        
        // Update aspect ratio
        if (_aspectRatio.width == 0 || _aspectRatio.height == 0) {
            _aspectRatio = _originalSize;
        }
        
        // Update vertex buffer for new aspect ratio
        [self updateVertexBufferForAspectRatio];
    }
    
    // Create a new texture if needed
    if (!_frameTexture || 
        _frameTexture.width != width || 
        _frameTexture.height != height) {
        
        MTLTextureDescriptor *textureDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                                                                                             width:width
                                                                                            height:height
                                                                                         mipmapped:NO];
        textureDesc.usage = MTLTextureUsageShaderRead;
        textureDesc.storageMode = MTLStorageModeManaged;
        
        _frameTexture = [_device newTextureWithDescriptor:textureDesc];
        
        if (!_frameTexture) {
            NSLog(@"Failed to create frame texture");
            return;
        }
    }
    
    // Copy the frame data to the texture
    MTLRegion region = MTLRegionMake2D(0, 0, width, height);
    [_frameTexture replaceRegion:region mipmapLevel:0 withBytes:frameData bytesPerRow:width * 4];
}

- (void)setShaderType:(int)type {
    if (type < 0 || type > METAL_SHADER_HQ2X) {
        type = METAL_SHADER_BASIC;
    }
    
    _shaderType = type;
    
    // Update rendering pipeline state for selected shader
    [self createRenderPipeline];
    
    // Configure additional shader options based on type
    switch (_shaderType) {
        case METAL_SHADER_CRT:
            _bilinearFiltering = YES;
            break;
            
        case METAL_SHADER_SCANLINES:
            _bilinearFiltering = YES;
            break;
            
        case METAL_SHADER_HQ2X:
            _bilinearFiltering = YES;
            break;
            
        case METAL_SHADER_BASIC:
        default:
            _bilinearFiltering = YES;
            break;
    }
}

#pragma mark - MTKViewDelegate

- (void)mtkView:(MTKView *)view drawableSizeWillChange:(CGSize)size {
    // Update when drawable size changes
    [self updateVertexBufferForAspectRatio];
}

- (void)drawInMTKView:(MTKView *)view {
    // Don't render if no texture
    if (!_frameTexture) {
        return;
    }
    
    // Create command buffer
    id<MTLCommandBuffer> commandBuffer = [_commandQueue commandBuffer];
    
    // Create render pass descriptor
    MTLRenderPassDescriptor *renderPassDescriptor = view.currentRenderPassDescriptor;
    if (!renderPassDescriptor) {
        return;
    }
    
    // Create render command encoder
    id<MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
    
    // Set render pipeline state
    [renderEncoder setRenderPipelineState:_pipelineState];
    
    // Set vertex buffer
    [renderEncoder setVertexBuffer:_vertexBuffer offset:0 atIndex:0];
    
    // Set texture
    [renderEncoder setFragmentTexture:_frameTexture atIndex:0];
    
    // Create sampler state for texture filtering
    MTLSamplerDescriptor *samplerDesc = [[MTLSamplerDescriptor alloc] init];
    samplerDesc.minFilter = _bilinearFiltering ? MTLSamplerMinMagFilterLinear : MTLSamplerMinMagFilterNearest;
    samplerDesc.magFilter = _bilinearFiltering ? MTLSamplerMinMagFilterLinear : MTLSamplerMinMagFilterNearest;
    samplerDesc.sAddressMode = MTLSamplerAddressModeClampToEdge;
    samplerDesc.tAddressMode = MTLSamplerAddressModeClampToEdge;
    
    id<MTLSamplerState> samplerState = [_device newSamplerStateWithDescriptor:samplerDesc];
    [renderEncoder setFragmentSamplerState:samplerState atIndex:0];
    
    // Set additional uniforms for enhanced shaders if needed
    if (_shaderType != METAL_SHADER_BASIC) {
        // Create uniform buffer for shader parameters
        float time = CFAbsoluteTimeGetCurrent();
        float aspectRatio = (float)view.drawableSize.width / (float)view.drawableSize.height;
        
        // Define render uniforms
        typedef struct {
            float modelViewMatrix[16];
            float tint[4];
            float time;
            float aspectRatio;
            float scanlineIntensity;
            float curvature;
            float vignetteIntensity;
            float chromaticAberration;
            float sharpness;
            int effectMode;
        } RenderUniforms;
        
        RenderUniforms uniforms;
        
        // Identity matrix
        memset(uniforms.modelViewMatrix, 0, sizeof(uniforms.modelViewMatrix));
        uniforms.modelViewMatrix[0] = 1.0f;
        uniforms.modelViewMatrix[5] = 1.0f;
        uniforms.modelViewMatrix[10] = 1.0f;
        uniforms.modelViewMatrix[15] = 1.0f;
        
        // Tint (white)
        uniforms.tint[0] = 1.0f;
        uniforms.tint[1] = 1.0f;
        uniforms.tint[2] = 1.0f;
        uniforms.tint[3] = 1.0f;
        
        // Effect parameters
        uniforms.time = time;
        uniforms.aspectRatio = aspectRatio;
        uniforms.scanlineIntensity = 0.4f;  // Moderate scanlines
        uniforms.curvature = 0.1f;          // Slight curvature
        uniforms.vignetteIntensity = 0.2f;  // Subtle vignette
        uniforms.chromaticAberration = 0.02f; // Slight chromatic aberration
        uniforms.sharpness = 1.0f;          // Normal sharpness
        uniforms.effectMode = _shaderType;
        
        // Create buffer and set it
        id<MTLBuffer> uniformBuffer = [_device newBufferWithBytes:&uniforms 
                                                          length:sizeof(uniforms) 
                                                         options:MTLResourceStorageModeShared];
        
        [renderEncoder setVertexBuffer:uniformBuffer offset:0 atIndex:1];
        [renderEncoder setFragmentBuffer:uniformBuffer offset:0 atIndex:1];
        
        // For CRT shader, add additional CRT parameters
        if (_shaderType == METAL_SHADER_CRT) {
            typedef struct {
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
                int use_subpixel_layout;
            } CRTParams;
            
            CRTParams crtParams;
            crtParams.mask_intensity = 0.3f;
            crtParams.mask_size = 1.0f;
            crtParams.mask_dot_width = 1.0f;
            crtParams.mask_dot_height = 1.0f;
            crtParams.curvature = 0.1f;
            crtParams.scanline_width = 1.0f;
            crtParams.scanline_intensity = 0.5f;
            crtParams.vignette_size = 1.0f;
            crtParams.vignette_intensity = 0.3f;
            crtParams.brightness = 1.1f;
            crtParams.contrast = 1.1f;
            crtParams.saturation = 1.1f;
            crtParams.use_subpixel_layout = 1;
            
            id<MTLBuffer> crtBuffer = [_device newBufferWithBytes:&crtParams 
                                                          length:sizeof(crtParams) 
                                                         options:MTLResourceStorageModeShared];
            
            [renderEncoder setFragmentBuffer:crtBuffer offset:0 atIndex:2];
        }
    }
    
    // Draw 6 vertices (2 triangles)
    [renderEncoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:6];
    
    // End rendering
    [renderEncoder endEncoding];
    
    // Present drawable
    [commandBuffer presentDrawable:view.currentDrawable];
    
    // Commit command buffer
    [commandBuffer commit];
}

// Color format conversion methods
- (void)convertRGB888ToRGBA8888:(const void *)srcData size:(size_t)size destData:(void *)destData {
    const uint8_t *src = (const uint8_t *)srcData;
    uint32_t *dest = (uint32_t *)destData;
    
    for (size_t i = 0; i < size / 3; i++) {
        uint8_t r = src[i * 3];
        uint8_t g = src[i * 3 + 1];
        uint8_t b = src[i * 3 + 2];
        
        // Pack as RGBA8888 (RGBA format expected by Metal)
        dest[i] = (r << 24) | (g << 16) | (b << 8) | 0xFF;
    }
}

- (void)convertRGB565ToRGBA8888:(const void *)srcData size:(size_t)size destData:(void *)destData pitch:(unsigned int)pitch {
    const uint16_t *src = (const uint16_t *)srcData;
    uint32_t *dest = (uint32_t *)destData;
    
    // Calculate pixels per row and rows
    size_t pixelsPerRow = pitch / 2;  // 2 bytes per pixel in RGB565
    size_t rows = size / pitch;
    
    for (size_t row = 0; row < rows; row++) {
        for (size_t col = 0; col < pixelsPerRow; col++) {
            uint16_t pixel = src[row * pixelsPerRow + col];
            
            // Extract RGB565 components (5 bits R, 6 bits G, 5 bits B)
            uint8_t r = ((pixel >> 11) & 0x1F) << 3;  // 5 bits to 8 bits
            uint8_t g = ((pixel >> 5) & 0x3F) << 2;   // 6 bits to 8 bits
            uint8_t b = (pixel & 0x1F) << 3;          // 5 bits to 8 bits
            
            // Add the lower bits to improve color accuracy
            r |= r >> 5;
            g |= g >> 6;
            b |= b >> 5;
            
            // Pack as RGBA8888 (RGBA format expected by Metal)
            dest[row * pixelsPerRow + col] = (r << 24) | (g << 16) | (b << 8) | 0xFF;
        }
    }
}

- (void)convertRGB555ToRGBA8888:(const void *)srcData size:(size_t)size destData:(void *)destData pitch:(unsigned int)pitch {
    const uint16_t *src = (const uint16_t *)srcData;
    uint32_t *dest = (uint32_t *)destData;
    
    // Calculate pixels per row and rows
    size_t pixelsPerRow = pitch / 2;  // 2 bytes per pixel in RGB555
    size_t rows = size / pitch;
    
    for (size_t row = 0; row < rows; row++) {
        for (size_t col = 0; col < pixelsPerRow; col++) {
            uint16_t pixel = src[row * pixelsPerRow + col];
            
            // Extract RGB555 components (5 bits each for R, G, B)
            uint8_t r = ((pixel >> 10) & 0x1F) << 3;  // 5 bits to 8 bits
            uint8_t g = ((pixel >> 5) & 0x1F) << 3;   // 5 bits to 8 bits
            uint8_t b = (pixel & 0x1F) << 3;          // 5 bits to 8 bits
            
            // Add the lower bits to improve color accuracy
            r |= r >> 5;
            g |= g >> 5;
            b |= b >> 5;
            
            // Pack as RGBA8888 (RGBA format expected by Metal)
            dest[row * pixelsPerRow + col] = (r << 24) | (g << 16) | (b << 8) | 0xFF;
        }
    }
}

// Set aspect ratio preservation
- (void)setPreserveAspectRatio:(BOOL)preserve {
    if (_preserveAspectRatio != preserve) {
        _preserveAspectRatio = preserve;
        [self updateVertexBufferForAspectRatio];
    }
}

// Set scanline intensity
- (void)setScanlineIntensity:(float)intensity {
    // Clamp to valid range
    _scanlineIntensity = MAX(0.0f, MIN(1.0f, intensity));
}

// Set CRT curvature
- (void)setCRTCurvature:(float)curvature {
    // Clamp to valid range
    _crtCurvature = MAX(0.0f, MIN(1.0f, curvature));
}

// Set sharpness
- (void)setSharpness:(float)sharpness {
    // Clamp to valid range
    _sharpness = MAX(0.0f, MIN(1.0f, sharpness));
}

// Create texture with current dimensions
- (void)createTexture {
    if (_textureSize.width <= 0 || _textureSize.height <= 0) {
        return;
    }
    
    // Create texture descriptor
    MTLTextureDescriptor *textureDesc = [MTLTextureDescriptor 
                                        texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                                        width:_textureSize.width
                                        height:_textureSize.height
                                        mipmapped:NO];
    
    textureDesc.usage = MTLTextureUsageShaderRead | MTLTextureUsageRenderTarget;
    
    // Create texture
    _frameTexture = [_device newTextureWithDescriptor:textureDesc];
    
    if (!_frameTexture) {
        NSLog(@"Failed to create frame texture");
    }
}

@end

// Global renderer instance
static FBNeoMetalRenderer *g_renderer = nil;

// Conversion buffer for color formats
static void *g_conversionBuffer = NULL;
static size_t g_conversionBufferSize = 0;

// C interface implementation
extern "C" {

// We'll use shader types defined in metal_renderer_defines.h
// No need to define METAL_SHADER_* constants again

// Initialize Metal renderer with a view
int MetalRenderer_Init(void* view) {
    // Create a Metal renderer instance
    MTKView *metalView = (__bridge MTKView *)view;
    if (!metalView) {
        NSLog(@"Invalid Metal view");
        return METAL_ERROR_NO_VIEW;
    }
    
    // Make sure we have a Metal device
    if (!metalView.device) {
        NSLog(@"No Metal device available");
        return METAL_ERROR_NO_DEVICE;
    }
    
    // Create renderer
    g_renderer = [[FBNeoMetalRenderer alloc] initWithMetalView:metalView];
    if (!g_renderer) {
        NSLog(@"Failed to create Metal renderer");
        return METAL_ERROR_NOT_INITIALIZED;
    }
    
    return METAL_ERROR_NONE;
}

// Shutdown the Metal renderer
void MetalRenderer_Exit() {
    g_renderer = nil;
    
    // Free conversion buffer if allocated
    if (g_conversionBuffer) {
        free(g_conversionBuffer);
        g_conversionBuffer = NULL;
        g_conversionBufferSize = 0;
    }
}

// Set the aspect ratio 
void MetalRenderer_SetAspectRatio(int nWidth, int nHeight) {
    if (!g_renderer) {
        return;
    }
    
    g_renderer.aspectRatio = CGSizeMake(nWidth, nHeight);
    [g_renderer updateVertexBufferForAspectRatio];
}

// Set whether to preserve aspect ratio
void MetalRenderer_SetPreserveAspectRatio(int preserve) {
    if (!g_renderer) {
        return;
    }
    
    g_renderer.preserveAspectRatio = (preserve != 0);
    [g_renderer updateVertexBufferForAspectRatio];
}

// Set the shader type
void MetalRenderer_SetShaderType(int shaderType) {
    if (!g_renderer) {
        return;
    }
    
    [g_renderer setShaderType:shaderType];
}

// Set scanline intensity (0.0-1.0)
void MetalRenderer_SetScanlineIntensity(float intensity) {
    if (!g_renderer) {
        return;
    }
    
    [g_renderer setScanlineIntensity:intensity];
}

// Set CRT curvature (0.0-1.0)
void MetalRenderer_SetCRTCurvature(float curvature) {
    if (!g_renderer) {
        return;
    }
    
    [g_renderer setCRTCurvature:curvature];
}

// Set sharpness (0.0-1.0)
void MetalRenderer_SetSharpness(float sharpness) {
    if (!g_renderer) {
        return;
    }
    
    [g_renderer setSharpness:sharpness];
}

void UpdateMetalFrameTexture(const void* frameData, unsigned int width, unsigned int height) {
    if (!g_renderer || !frameData || width == 0 || height == 0) {
        return;
    }
    
    [g_renderer updateTextureWithFrameData:frameData width:width height:height];
}

// This is a key function that will handle different color formats
void Metal_RenderFrame(const void* frameData, unsigned int width, unsigned int height, unsigned int pitch, unsigned int bpp) {
    if (!g_renderer || !frameData || width == 0 || height == 0) {
        return;
    }
    
    // Check if buffer allocation or resizing is needed
    size_t requiredBufferSize = width * height * 4; // Always RGBA8888
    if (!g_conversionBuffer || g_conversionBufferSize < requiredBufferSize) {
        if (g_conversionBuffer) {
            free(g_conversionBuffer);
        }
        
        g_conversionBuffer = malloc(requiredBufferSize);
        g_conversionBufferSize = requiredBufferSize;
        
        if (!g_conversionBuffer) {
            NSLog(@"Failed to allocate conversion buffer");
            return;
        }
    }
    
    // Convert based on bits per pixel
    switch (bpp) {
        case 32: // RGBA8888 or BGRA8888, ready to use
            // Just update the texture directly
            [g_renderer updateTextureWithFrameData:frameData width:width height:height];
            break;
            
        case 24: // RGB888, needs alpha
            [g_renderer convertRGB888ToRGBA8888:frameData size:width * height * 3 destData:g_conversionBuffer];
            [g_renderer updateTextureWithFrameData:g_conversionBuffer width:width height:height];
            break;
            
        case 16: // RGB565, needs conversion
            [g_renderer convertRGB565ToRGBA8888:frameData size:width * height * 2 destData:g_conversionBuffer pitch:pitch];
            [g_renderer updateTextureWithFrameData:g_conversionBuffer width:width height:height];
            break;
            
        case 15: // RGB555, needs conversion
            [g_renderer convertRGB555ToRGBA8888:frameData size:width * height * 2 destData:g_conversionBuffer pitch:pitch];
            [g_renderer updateTextureWithFrameData:g_conversionBuffer width:width height:height];
            break;
            
        default:
            NSLog(@"Unsupported bits per pixel: %u", bpp);
            break;
    }
    
    // No explicit render call needed as MTKViewDelegate handles rendering
}

// Render frame with command buffer (for external rendering)
void MetalRenderer_Render(void* commandBuffer) {
    // This function is mainly for external command buffer support
    // Typically the MTKViewDelegate handles rendering automatically
}

} // extern "C" 