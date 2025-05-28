#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <Cocoa/Cocoa.h>

// Frame buffer properties
static uint32_t* g_frameBuffer = NULL;
static int g_frameWidth = 384;    // Standard CPS2 width
static int g_frameHeight = 224;   // Standard CPS2 height
static bool g_frameUpdated = false;

@interface MetalRenderer : NSObject
@property (strong) id<MTLDevice> device;
@property (strong) id<MTLCommandQueue> commandQueue;
@property (strong) id<MTLLibrary> library;
@property (strong) id<MTLRenderPipelineState> pipelineState;
@property (strong) id<MTLBuffer> vertexBuffer;
@property (strong) id<MTLTexture> texture;
@property (strong) id<MTLSamplerState> samplerState;

- (instancetype)initWithDevice:(id<MTLDevice>)device;
- (void)render:(MTKView *)view;
- (void)updateTextureWithFrameBuffer;
@end

@implementation MetalRenderer

- (instancetype)initWithDevice:(id<MTLDevice>)device {
    if (self = [super init]) {
        _device = device;
        _commandQueue = [device newCommandQueue];
        
        // Try to load the Metal shader library
        NSError *error = nil;
        
        // Try to compile from string first for guaranteed results
        NSString *shaderSource = @"#include <metal_stdlib>\n"
                                "using namespace metal;\n"
                                "\n"
                                "struct VertexIn {\n"
                                "    float4 position [[attribute(0)]];\n"
                                "    float2 texCoord [[attribute(1)]];\n"
                                "};\n"
                                "\n"
                                "struct VertexOut {\n"
                                "    float4 position [[position]];\n"
                                "    float2 texCoord;\n"
                                "};\n"
                                "\n"
                                "vertex VertexOut vertex_main(uint vertexID [[vertex_id]],\n"
                                "                           constant VertexIn* vertices [[buffer(0)]]) {\n"
                                "    VertexOut out;\n"
                                "    out.position = vertices[vertexID].position;\n"
                                "    out.texCoord = vertices[vertexID].texCoord;\n"
                                "    return out;\n"
                                "}\n"
                                "\n"
                                "fragment float4 fragment_main(VertexOut in [[stage_in]],\n"
                                "                            texture2d<float> texture [[texture(0)]],\n"
                                "                            sampler textureSampler [[sampler(0)]]) {\n"
                                "    return texture.sample(textureSampler, in.texCoord);\n"
                                "}\n";
        
        _library = [device newLibraryWithSource:shaderSource options:nil error:&error];
        if (!_library) {
            NSLog(@"Failed to compile shader source: %@", error);
            
            // Try loading from file as a fallback
            NSString *path = @"metal_build/metal_shaders.metallib";
            if ([[NSFileManager defaultManager] fileExistsAtPath:path]) {
                NSURL *url = [NSURL fileURLWithPath:path];
                _library = [device newLibraryWithURL:url error:&error];
                if (!_library) {
                    NSLog(@"Failed to load shader library from file: %@", error);
                }
            }
            
            // Final fallback to default library
            if (!_library) {
                NSLog(@"Attempting to use default library...");
                _library = [device newDefaultLibrary];
                if (!_library) {
                    NSLog(@"Failed to load default shader library");
                    return nil;
                }
            }
        } else {
            NSLog(@"Successfully compiled shader source");
        }
        
        // Create pipeline state
        id<MTLFunction> vertexFunction = [_library newFunctionWithName:@"vertex_main"];
        id<MTLFunction> fragmentFunction = [_library newFunctionWithName:@"fragment_main"];
        
        if (!vertexFunction || !fragmentFunction) {
            NSLog(@"Failed to get shader functions");
            return nil;
        }
        
        // Create vertex buffer with quad vertices
        float quadVertices[] = {
            // positions        // texture coords
            -1.0f, -1.0f, 0.0f, 1.0f,  0.0f, 1.0f, // bottom left
             1.0f, -1.0f, 0.0f, 1.0f,  1.0f, 1.0f, // bottom right
            -1.0f,  1.0f, 0.0f, 1.0f,  0.0f, 0.0f, // top left
             1.0f,  1.0f, 0.0f, 1.0f,  1.0f, 0.0f  // top right
        };
        
        _vertexBuffer = [device newBufferWithBytes:quadVertices
                                           length:sizeof(quadVertices)
                                          options:MTLResourceStorageModeShared];
        
        // Create render pipeline state
        MTLRenderPipelineDescriptor *pipelineDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
        pipelineDescriptor.vertexFunction = vertexFunction;
        pipelineDescriptor.fragmentFunction = fragmentFunction;
        pipelineDescriptor.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
        
        _pipelineState = [device newRenderPipelineStateWithDescriptor:pipelineDescriptor error:&error];
        if (!_pipelineState) {
            NSLog(@"Failed to create pipeline state: %@", error);
            return nil;
        }
        
        // Create texture
        [self createFrameBufferTexture];
        
        // Create sampler state
        MTLSamplerDescriptor *samplerDescriptor = [[MTLSamplerDescriptor alloc] init];
        samplerDescriptor.minFilter = MTLSamplerMinMagFilterLinear;
        samplerDescriptor.magFilter = MTLSamplerMinMagFilterLinear;
        samplerDescriptor.sAddressMode = MTLSamplerAddressModeClampToEdge;
        samplerDescriptor.tAddressMode = MTLSamplerAddressModeClampToEdge;
        
        _samplerState = [device newSamplerStateWithDescriptor:samplerDescriptor];
        
        NSLog(@"Metal renderer initialized successfully");
    }
    return self;
}

- (void)createFrameBufferTexture {
    if (g_frameWidth <= 0 || g_frameHeight <= 0) {
        NSLog(@"Invalid frame buffer dimensions: %dx%d", g_frameWidth, g_frameHeight);
        return;
    }
    
    // Create a texture descriptor
    MTLTextureDescriptor *textureDescriptor = [[MTLTextureDescriptor alloc] init];
    textureDescriptor.pixelFormat = MTLPixelFormatBGRA8Unorm;
    textureDescriptor.width = g_frameWidth;
    textureDescriptor.height = g_frameHeight;
    textureDescriptor.usage = MTLTextureUsageShaderRead;
    
    // Create the texture
    _texture = [_device newTextureWithDescriptor:textureDescriptor];
    if (!_texture) {
        NSLog(@"Failed to create texture");
        return;
    }
    
    NSLog(@"Created frame buffer texture: %dx%d", g_frameWidth, g_frameHeight);
    
    // Create frame buffer if it doesn't exist
    if (!g_frameBuffer) {
        g_frameBuffer = (uint32_t*)malloc(g_frameWidth * g_frameHeight * sizeof(uint32_t));
        if (g_frameBuffer) {
            // Generate a test pattern
            for (int y = 0; y < g_frameHeight; y++) {
                for (int x = 0; x < g_frameWidth; x++) {
                    uint8_t r = (uint8_t)((float)x / g_frameWidth * 255);
                    uint8_t g = (uint8_t)((float)y / g_frameHeight * 255);
                    uint8_t b = (uint8_t)((float)(x + y) / (g_frameWidth + g_frameHeight) * 255);
                    g_frameBuffer[y * g_frameWidth + x] = 0xFF000000 | (r << 16) | (g << 8) | b;
                }
            }
            g_frameUpdated = true;
            NSLog(@"Created frame buffer: %dx%d", g_frameWidth, g_frameHeight);
        }
    }
}

- (void)updateTextureWithFrameBuffer {
    if (!g_frameBuffer || !_texture || !g_frameUpdated) {
        return;
    }
    
    // Update the texture with the frame buffer data
    MTLRegion region = MTLRegionMake2D(0, 0, g_frameWidth, g_frameHeight);
    [_texture replaceRegion:region
                mipmapLevel:0
                  withBytes:g_frameBuffer
                bytesPerRow:g_frameWidth * sizeof(uint32_t)];
    
    g_frameUpdated = false;
    
    static int frameCounter = 0;
    if (++frameCounter % 60 == 0) {
        NSLog(@"Updated texture with frame data (frame %d)", frameCounter);
    }
}

- (void)render:(MTKView *)view {
    // Update the texture from the frame buffer
    [self updateTextureWithFrameBuffer];
    
    // Get the command buffer and render pass descriptor
    id<MTLCommandBuffer> commandBuffer = [_commandQueue commandBuffer];
    MTLRenderPassDescriptor *renderPassDescriptor = view.currentRenderPassDescriptor;
    
    if (renderPassDescriptor) {
        // Create render encoder
        id<MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
        
        // Set the pipeline state
        [renderEncoder setRenderPipelineState:_pipelineState];
        
        // Set the vertex buffer
        [renderEncoder setVertexBuffer:_vertexBuffer offset:0 atIndex:0];
        
        // Set the texture and sampler
        [renderEncoder setFragmentTexture:_texture atIndex:0];
        [renderEncoder setFragmentSamplerState:_samplerState atIndex:0];
        
        // Draw the quad
        [renderEncoder drawPrimitives:MTLPrimitiveTypeTriangleStrip
                          vertexStart:0
                          vertexCount:4];
        
        // End encoding
        [renderEncoder endEncoding];
        
        // Present the drawable
        [commandBuffer presentDrawable:view.currentDrawable];
    }
    
    // Commit the command buffer
    [commandBuffer commit];
}

@end

@interface MetalView : MTKView
@property (strong) MetalRenderer *renderer;
@end

@implementation MetalView

- (instancetype)initWithFrame:(NSRect)frameRect device:(id<MTLDevice>)device {
    self = [super initWithFrame:frameRect device:device];
    if (self) {
        self.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
        self.clearColor = MTLClearColorMake(0.0, 0.0, 0.2, 1.0);
        self.renderer = [[MetalRenderer alloc] initWithDevice:device];
        self.paused = NO;
        self.enableSetNeedsDisplay = YES;
    }
    return self;
}

- (void)drawRect:(NSRect)dirtyRect {
    [self.renderer render:self];
}

@end

@interface AppDelegate : NSObject <NSApplicationDelegate>
@property (strong) NSWindow *window;
@property (strong) MetalView *metalView;
@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    // Create device
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    if (!device) {
        NSLog(@"Metal is not supported on this device");
        return;
    }
    
    // Create window
    NSRect frame = NSMakeRect(0, 0, 800, 600);
    NSUInteger styleMask = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | 
                           NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable;
    
    self.window = [[NSWindow alloc] initWithContentRect:frame
                                             styleMask:styleMask
                                               backing:NSBackingStoreBuffered
                                                 defer:NO];
    
    self.window.title = @"FBNeo Metal Simple";
    [self.window center];
    
    // Create Metal view
    self.metalView = [[MetalView alloc] initWithFrame:self.window.contentView.bounds device:device];
    self.metalView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    [self.window.contentView addSubview:self.metalView];
    
    // Show window
    [self.window makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];
    
    // Set up animation timer
    [NSTimer scheduledTimerWithTimeInterval:1.0/60.0
                                     target:self
                                   selector:@selector(updateAnimation:)
                                   userInfo:nil
                                    repeats:YES];
}

- (void)updateAnimation:(NSTimer *)timer {
    static int frameCount = 0;
    frameCount++;
    
    // Update frame buffer every frame
    if (g_frameBuffer) {
        for (int y = 0; y < g_frameHeight; y++) {
            for (int x = 0; x < g_frameWidth; x++) {
                float time = frameCount / 60.0;
                
                // Make a moving pattern
                float r = (sin(x * 0.05 + time) * 0.5 + 0.5) * 255;
                float g = (cos(y * 0.05 + time) * 0.5 + 0.5) * 255;
                float b = (sin((x + y) * 0.05 + time) * 0.5 + 0.5) * 255;
                
                g_frameBuffer[y * g_frameWidth + x] = 0xFF000000 | ((uint8_t)r << 16) | ((uint8_t)g << 8) | (uint8_t)b;
            }
        }
        g_frameUpdated = true;
    }
    
    // Make view redraw
    [self.metalView setNeedsDisplay:YES];
}

@end

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        // Set up initial frame buffer dimensions
        g_frameWidth = 384;
        g_frameHeight = 224;
        
        // Create application
        [NSApplication sharedApplication];
        
        // Create app delegate
        AppDelegate *delegate = [[AppDelegate alloc] init];
        [NSApp setDelegate:delegate];
        
        // Create basic menu
        NSMenu *menubar = [[NSMenu alloc] init];
        NSMenuItem *appMenuItem = [[NSMenuItem alloc] init];
        [menubar addItem:appMenuItem];
        NSMenu *appMenu = [[NSMenu alloc] init];
        NSMenuItem *quitMenuItem = [[NSMenuItem alloc] initWithTitle:@"Quit" 
                                                             action:@selector(terminate:) 
                                                      keyEquivalent:@"q"];
        [appMenu addItem:quitMenuItem];
        [appMenuItem setSubmenu:appMenu];
        [NSApp setMainMenu:menubar];
        
        // Run the application
        [NSApp activateIgnoringOtherApps:YES];
        [NSApp run];
    }
    return 0;
}
