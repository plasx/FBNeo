# Metal Rendering Implementation

This document details the implementation plan for replacing stub functions in the Metal rendering components of FBNeo Metal.

## Frame Buffer Integration

```objective-c
// Implementation for Metal_RenderFrame in metal_renderer.mm:
int Metal_RenderFrame(void* frameData, int width, int height)
{
    // Validate input parameters
    if (!frameData || width <= 0 || height <= 0) {
        return 0;
    }
    
    // Get the shared renderer instance
    MetalRenderer* renderer = [MetalRenderer sharedRenderer];
    if (!renderer) {
        return 0;
    }
    
    // Calculate the pitch (bytes per row)
    int pitch = width * 4; // RGBA format, 4 bytes per pixel
    
    // Update the Metal texture with the new frame data
    [renderer updateTextureWithBuffer:(uint8_t*)frameData 
                               width:width 
                              height:height 
                               pitch:pitch];
    
    // Request a redraw
    [renderer setNeedsRedraw:YES];
    
    return 1;
}
```

## MetalRenderer Class Implementation

```objective-c
@implementation MetalRenderer

// Shared renderer instance
static MetalRenderer* _sharedRenderer = nil;

+ (instancetype)sharedRenderer
{
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        _sharedRenderer = [[MetalRenderer alloc] init];
    });
    return _sharedRenderer;
}

- (instancetype)init
{
    self = [super init];
    if (self) {
        // Get the default Metal device
        _device = MTLCreateSystemDefaultDevice();
        if (!_device) {
            NSLog(@"Metal is not supported on this device");
            return nil;
        }
        
        // Create a command queue
        _commandQueue = [_device newCommandQueue];
        
        // Set up the default library containing our shaders
        NSError* error = nil;
        _defaultLibrary = [_device newDefaultLibrary];
        if (!_defaultLibrary) {
            NSLog(@"Failed to load default shader library: %@", error);
            return nil;
        }
        
        // Create the pipeline state
        [self createRenderPipelineState];
        
        // Initialize texture properties
        _textureWidth = 0;
        _textureHeight = 0;
        _frameTexture = nil;
        
        // Default render settings
        _useFiltering = YES;
        _useVSync = YES;
        _useCRT = NO;
        _useScanlines = NO;
        _scanlineIntensity = 0.3f;
        _scale = 1.0f;
    }
    return self;
}

// Create the render pipeline state
- (void)createRenderPipelineState
{
    // Load the vertex function from the library
    id<MTLFunction> vertexFunction = [_defaultLibrary newFunctionWithName:@"vertexShader"];
    
    // Load the fragment function from the library
    id<MTLFunction> fragmentFunction = [_defaultLibrary newFunctionWithName:@"fragmentShader"];
    
    // Create a render pipeline descriptor
    MTLRenderPipelineDescriptor* pipelineDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
    pipelineDescriptor.vertexFunction = vertexFunction;
    pipelineDescriptor.fragmentFunction = fragmentFunction;
    pipelineDescriptor.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
    
    // Create the pipeline state
    NSError* error = nil;
    _pipelineState = [_device newRenderPipelineStateWithDescriptor:pipelineDescriptor error:&error];
    if (!_pipelineState) {
        NSLog(@"Failed to create pipeline state: %@", error);
    }
}

// Create a texture with the given dimensions
- (void)createTextureWithWidth:(NSUInteger)width height:(NSUInteger)height
{
    // Create a texture descriptor
    MTLTextureDescriptor* textureDescriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                                                                                                 width:width
                                                                                                height:height
                                                                                             mipmapped:NO];
    textureDescriptor.usage = MTLTextureUsageShaderRead | MTLTextureUsageRenderTarget;
    
    // Create the texture
    _frameTexture = [_device newTextureWithDescriptor:textureDescriptor];
    
    // Store the dimensions
    _textureWidth = width;
    _textureHeight = height;
}

// Update the texture with new frame data
- (void)updateTextureWithBuffer:(uint8_t*)buffer width:(int)width height:(int)height pitch:(int)pitch
{
    // Check if we need to create a new texture
    if (!_frameTexture || width != _textureWidth || height != _textureHeight) {
        [self createTextureWithWidth:width height:height];
    }
    
    // Create a region covering the entire texture
    MTLRegion region = {
        { 0, 0, 0 },        // MTLOrigin
        { width, height, 1 } // MTLSize
    };
    
    // Replace the texture contents
    [_frameTexture replaceRegion:region
                     mipmapLevel:0
                       withBytes:buffer
                     bytesPerRow:pitch];
}

// Draw the current frame
- (void)drawFrame
{
    // Get the current drawable
    id<MTLDrawable> drawable = _view.currentDrawable;
    if (!drawable || !_frameTexture) {
        return;
    }
    
    // Create a render pass descriptor
    MTLRenderPassDescriptor* renderPassDescriptor = _view.currentRenderPassDescriptor;
    if (!renderPassDescriptor) {
        return;
    }
    
    // Create a command buffer
    id<MTLCommandBuffer> commandBuffer = [_commandQueue commandBuffer];
    
    // Create a render command encoder
    id<MTLRenderCommandEncoder> encoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
    [encoder setRenderPipelineState:_pipelineState];
    
    // Set up the vertices for a fullscreen quad
    float vertices[] = {
        -1.0f,  1.0f, 0.0f, 0.0f,  // Top left
         1.0f,  1.0f, 1.0f, 0.0f,  // Top right
        -1.0f, -1.0f, 0.0f, 1.0f,  // Bottom left
         1.0f, -1.0f, 1.0f, 1.0f   // Bottom right
    };
    
    // Set the vertex data
    [encoder setVertexBytes:vertices length:sizeof(vertices) atIndex:0];
    
    // Set the texture
    [encoder setFragmentTexture:_frameTexture atIndex:0];
    
    // Draw the quad (two triangles)
    [encoder drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];
    
    // End encoding
    [encoder endEncoding];
    
    // Present the drawable
    [commandBuffer presentDrawable:drawable];
    
    // Commit the command buffer
    [commandBuffer commit];
}

// Enable/disable CRT effect
- (void)setCRTEffect:(BOOL)enabled
{
    _useCRT = enabled;
    [self updateFragmentShader];
}

// Enable/disable scanlines
- (void)setScanlines:(BOOL)enabled intensity:(float)intensity
{
    _useScanlines = enabled;
    _scanlineIntensity = intensity;
    [self updateFragmentShader];
}

// Update the fragment shader based on current settings
- (void)updateFragmentShader
{
    NSString* fragmentFunctionName = @"fragmentShader";
    
    if (_useCRT) {
        fragmentFunctionName = @"fragmentShaderCRT";
    } else if (_useScanlines) {
        fragmentFunctionName = @"fragmentShaderScanlines";
    }
    
    // Load the fragment function
    id<MTLFunction> fragmentFunction = [_defaultLibrary newFunctionWithName:fragmentFunctionName];
    
    // Create a new pipeline descriptor
    MTLRenderPipelineDescriptor* pipelineDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
    pipelineDescriptor.vertexFunction = [_defaultLibrary newFunctionWithName:@"vertexShader"];
    pipelineDescriptor.fragmentFunction = fragmentFunction;
    pipelineDescriptor.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
    
    // Create the pipeline state
    NSError* error = nil;
    _pipelineState = [_device newRenderPipelineStateWithDescriptor:pipelineDescriptor error:&error];
    if (!_pipelineState) {
        NSLog(@"Failed to update pipeline state: %@", error);
    }
}

@end
```

## Metal_UpdateTexture Implementation

```objective-c
// Implementation for Metal_UpdateTexture:
void Metal_UpdateTexture(void* data, int width, int height)
{
    if (!data || width <= 0 || height <= 0) {
        return;
    }
    
    // Calculate pitch (bytes per row)
    int pitch = width * 4; // RGBA format
    
    // Get the shared renderer instance
    MetalRenderer* renderer = [MetalRenderer sharedRenderer];
    if (!renderer) {
        return;
    }
    
    // Update the texture
    [renderer updateTextureWithBuffer:(uint8_t*)data 
                               width:width 
                              height:height 
                               pitch:pitch];
}
```

## Advanced Shader Effects

### CRT Shader Implementation

```metal
// Implementation for CRT shader effect:
fragment float4 fragmentShaderCRT(VertexOut in [[stage_in]],
                                 texture2d<float> texture [[texture(0)]]) {
    constexpr sampler textureSampler(mag_filter::linear, min_filter::linear);
    
    // CRT curvature
    float2 q = in.texCoord;
    float2 coords = q;
    coords = (coords - 0.5) * 2.0;
    float2 coordsSquared = coords * coords;
    float dist = coordsSquared.x + coordsSquared.y;
    coords *= 1.0 + dist * 0.15;  // Adjust curve amount
    coords = (coords * 0.5) + 0.5;
    
    // Only use the result if it's within texture bounds
    if (coords.x <= 0.0 || coords.x >= 1.0 || coords.y <= 0.0 || coords.y >= 1.0) {
        return float4(0.0, 0.0, 0.0, 1.0);
    }
    
    float4 color = texture.sample(textureSampler, coords);
    
    // Scanline effect
    float scanlineIntensity = 0.1;
    float scanlineWidth = 0.5;
    float scanline = sin(coords.y * texture.get_height() * 3.14159 * scanlineWidth);
    scanline = (scanline * scanlineIntensity) + (1.0 - scanlineIntensity);
    
    // Vignette effect (darker corners)
    float vignette = 1.0 - dist * 0.5;
    vignette = clamp(vignette, 0.0, 1.0);
    
    // Apply effects
    color *= scanline * vignette;
    
    // Color bleeding (slight RGB shifting)
    float4 colorShift = texture.sample(textureSampler, coords + float2(0.001, 0.0));
    color.r = color.r * 0.9 + colorShift.r * 0.1;
    
    // Brightness adjustment
    color = pow(color, 0.8);
    
    return color;
}
```

### Scanline Shader Implementation

```metal
// Implementation for scanline shader effect:
fragment float4 fragmentShaderScanlines(VertexOut in [[stage_in]],
                                       texture2d<float> texture [[texture(0)]],
                                       constant float &intensity [[buffer(0)]]) {
    constexpr sampler textureSampler(mag_filter::linear, min_filter::linear);
    
    float4 color = texture.sample(textureSampler, in.texCoord);
    
    // Scanline effect
    float scanlineIntensity = intensity;
    float scanlineWidth = 0.5;
    float scanline = sin(in.texCoord.y * texture.get_height() * 3.14159 * scanlineWidth);
    scanline = (scanline * scanlineIntensity) + (1.0 - scanlineIntensity);
    
    // Apply scanline effect
    color *= scanline;
    
    return color;
}
```

## Metal_SetRenderState Implementation

```objective-c
// Implementation for Metal_SetRenderState:
void Metal_SetRenderState(int state, int value)
{
    // Get the shared renderer instance
    MetalRenderer* renderer = [MetalRenderer sharedRenderer];
    if (!renderer) {
        return;
    }
    
    // Process the render state
    switch (state) {
        case METAL_STATE_VSYNC:
            [renderer setVSync:(value != 0)];
            break;
            
        case METAL_STATE_FILTERING:
            [renderer setFiltering:(value != 0)];
            break;
            
        case METAL_STATE_CRT:
            [renderer setCRTEffect:(value != 0)];
            break;
            
        case METAL_STATE_SCANLINES:
            [renderer setScanlines:(value != 0) intensity:0.3f];
            break;
            
        case METAL_STATE_SCANLINE_INTENSITY:
            [renderer setScanlineIntensity:(float)value / 100.0f];
            break;
            
        case METAL_STATE_SCALE:
            [renderer setScale:(float)value / 100.0f];
            break;
            
        default:
            break;
    }
}
```

## Implementation Steps

1. **Implement Metal_RenderFrame**
   - Create full implementation that integrates with frame buffer
   - Add texture handling and rendering

2. **Implement MetalRenderer Class**
   - Create shared instance pattern
   - Implement Metal device initialization
   - Add texture management
   - Create rendering pipeline

3. **Add Shader Implementations**
   - Implement basic texture rendering shader
   - Add CRT simulation shader
   - Implement scanline rendering
   - Create bloom and other effects

4. **Implement Configuration Interface**
   - Add Metal_SetRenderState implementation
   - Create configuration UI in Metal application
   - Add runtime switching of effects

5. **Optimize Performance**
   - Implement MTLHeaps for resource management
   - Add triple buffering support
   - Optimize command buffer usage
   - Add performance metrics

## Testing Strategy

For each component:
1. Create test patterns to validate rendering
2. Compare output against reference images
3. Measure performance with different configurations
4. Test on different macOS devices
5. Validate integration with core emulation

## Dependencies

- Metal framework
- MetalKit framework
- FBNeo core emulation
- Shader compilation pipeline
- Frame buffer management 