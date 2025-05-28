#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#include "metal_bridge.h"

@interface MetalView : MTKView
@property (nonatomic, strong) id<MTLBuffer> vertexBuffer;
@property (nonatomic, strong) id<MTLRenderPipelineState> pipelineState;
@property (nonatomic, strong) id<MTLTexture> texture;
@property (nonatomic, assign) vector_uint2 viewportSize;
@end

@implementation MetalView

- (instancetype)initWithFrame:(NSRect)frameRect device:(id<MTLDevice>)device {
    self = [super initWithFrame:frameRect device:device];
    if (self) {
        self.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
        self.clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);
        [self setupMetal];
    }
    return self;
}

- (void)setupMetal {
    id<MTLDevice> device = self.device;
    
    // Create vertices for a quad
    static const float vertices[] = {
        -1.0f, -1.0f, 0.0f, 1.0f,  // position, texcoord
         1.0f, -1.0f, 1.0f, 1.0f,
        -1.0f,  1.0f, 0.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 0.0f,
    };
    
    _vertexBuffer = [device newBufferWithBytes:vertices
                                        length:sizeof(vertices)
                                       options:MTLResourceStorageModeShared];
    
    // Create texture
    MTLTextureDescriptor *textureDescriptor = [MTLTextureDescriptor
        texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                     width:384
                                    height:224
                                 mipmapped:NO];
    textureDescriptor.usage = MTLTextureUsageShaderRead;
    _texture = [device newTextureWithDescriptor:textureDescriptor];
    
    // Create shaders
    id<MTLLibrary> defaultLibrary = [device newDefaultLibrary];
    if (!defaultLibrary) {
        NSLog(@"Failed to create default shader library");
        NSString *shaderSource = @"#include <metal_stdlib>\n"
                                 "using namespace metal;\n"
                                 "struct VertexIn {\n"
                                 "    float2 position [[attribute(0)]];\n"
                                 "    float2 texCoord [[attribute(1)]];\n"
                                 "};\n"
                                 "struct VertexOut {\n"
                                 "    float4 position [[position]];\n"
                                 "    float2 texCoord;\n"
                                 "};\n"
                                 "vertex VertexOut vertexShader(uint vertexID [[vertex_id]],\n"
                                 "                              constant float4 *vertices [[buffer(0)]]) {\n"
                                 "    VertexOut out;\n"
                                 "    float4 position = float4(vertices[vertexID].xy, 0.0, 1.0);\n"
                                 "    out.position = position;\n"
                                 "    out.texCoord = vertices[vertexID].zw;\n"
                                 "    return out;\n"
                                 "}\n"
                                 "fragment float4 fragmentShader(VertexOut in [[stage_in]],\n"
                                 "                               texture2d<float> tex [[texture(0)]]) {\n"
                                 "    constexpr sampler linearSampler(mag_filter::linear, min_filter::linear);\n"
                                 "    return tex.sample(linearSampler, in.texCoord);\n"
                                 "}\n";
        
        NSError *error = nil;
        MTLCompileOptions *options = [[MTLCompileOptions alloc] init];
        defaultLibrary = [device newLibraryWithSource:shaderSource options:options error:&error];
        if (!defaultLibrary) {
            NSLog(@"Failed to create library: %@", error);
            return;
        }
    }
    
    id<MTLFunction> vertexFunction = [defaultLibrary newFunctionWithName:@"vertexShader"];
    id<MTLFunction> fragmentFunction = [defaultLibrary newFunctionWithName:@"fragmentShader"];
    
    // Create render pipeline
    MTLRenderPipelineDescriptor *pipelineDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
    pipelineDescriptor.vertexFunction = vertexFunction;
    pipelineDescriptor.fragmentFunction = fragmentFunction;
    pipelineDescriptor.colorAttachments[0].pixelFormat = self.colorPixelFormat;
    
    NSError *error = nil;
    _pipelineState = [device newRenderPipelineStateWithDescriptor:pipelineDescriptor error:&error];
    if (!_pipelineState) {
        NSLog(@"Failed to create pipeline state: %@", error);
    }
    
    _viewportSize = (vector_uint2){self.drawableSize.width, self.drawableSize.height};
}

- (void)updateGameTexture:(const void *)data width:(NSUInteger)width height:(NSUInteger)height {
    if (!data || !_texture) return;
    
    MTLRegion region = MTLRegionMake2D(0, 0, width, height);
    [_texture replaceRegion:region
                mipmapLevel:0
                  withBytes:data
                bytesPerRow:width * 4]; // 4 bytes per pixel (BGRA8)
}

- (void)drawRect:(NSRect)dirtyRect {
    id<MTLCommandBuffer> commandBuffer = [self.device newCommandBuffer];
    MTLRenderPassDescriptor *renderPassDescriptor = self.currentRenderPassDescriptor;
    
    if (renderPassDescriptor) {
        id<MTLRenderCommandEncoder> encoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
        [encoder setViewport:(MTLViewport){0.0, 0.0, _viewportSize.x, _viewportSize.y, 0.0, 1.0}];
        [encoder setRenderPipelineState:_pipelineState];
        [encoder setVertexBuffer:_vertexBuffer offset:0 atIndex:0];
        [encoder setFragmentTexture:_texture atIndex:0];
        [encoder drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];
        [encoder endEncoding];
        
        [commandBuffer presentDrawable:self.currentDrawable];
    }
    
    [commandBuffer commit];
}

@end

@interface AppDelegate : NSObject <NSApplicationDelegate>
@property (strong) NSWindow *window;
@property (strong) MetalView *metalView;
@property (strong) NSTimer *gameTimer;
@property (strong) dispatch_source_t renderTimer;
@property (assign) BOOL gameRunning;
@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    NSRect frame = NSMakeRect(0, 0, 800, 600);
    NSUInteger styleMask = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable;
    
    self.window = [[NSWindow alloc] initWithContentRect:frame
                                             styleMask:styleMask
                                               backing:NSBackingStoreBuffered
                                                 defer:NO];
    
    [self.window setTitle:@"FBNeo Metal Test"];
    [self.window center];
    
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    self.metalView = [[MetalView alloc] initWithFrame:frame device:device];
    self.window.contentView = self.metalView;
    
    [self.window makeKeyAndOrderFront:nil];
    
    // Initialize FBNeo Metal bridge
    if (Metal_Init() != 0) {
        NSLog(@"Failed to initialize FBNeo Metal bridge");
        return;
    }
    
    // Load test ROM
    const char* romPath = "/Users/plasx/ROMs/arcade/mvsc";
    if (Metal_LoadROM(romPath) != 0) {
        NSLog(@"Failed to load ROM: %s", romPath);
        
        // Show test pattern if ROM can't be loaded
        Metal_ShowTestPattern(384, 224);
    } else {
        NSLog(@"ROM loaded successfully: %s", romPath);
    }

    // Set up game loop timer at 60 FPS
    self.gameRunning = YES;
    self.gameTimer = [NSTimer scheduledTimerWithTimeInterval:1.0/60.0
                                                     target:self
                                                   selector:@selector(runGameFrame)
                                                   userInfo:nil
                                                    repeats:YES];
}

- (void)runGameFrame {
    if (!self.gameRunning) return;
    
    // Allocate frame buffer if needed
    static void* frameBuffer = NULL;
    static int frameWidth = 384;
    static int frameHeight = 224;
    
    if (!frameBuffer) {
        frameBuffer = malloc(frameWidth * frameHeight * 4); // BGRA format, 4 bytes per pixel
        if (!frameBuffer) {
            NSLog(@"Failed to allocate frame buffer");
            return;
        }
        memset(frameBuffer, 0, frameWidth * frameHeight * 4);
    }
    
    // Run game frame and render
    Metal_RunFrame(true);
    Metal_RenderFrame(frameBuffer, frameWidth, frameHeight);
    
    // Update Metal texture with frame data
    dispatch_async(dispatch_get_main_queue(), ^{
        [self.metalView updateGameTexture:frameBuffer width:frameWidth height:frameHeight];
        [self.metalView setNeedsDisplay:YES];
    });
}

- (void)applicationWillTerminate:(NSNotification *)notification {
    self.gameRunning = NO;
    [self.gameTimer invalidate];
    Metal_Exit();
}

@end

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        NSApplication *app = [NSApplication sharedApplication];
        AppDelegate *appDelegate = [[AppDelegate alloc] init];
        [app setDelegate:appDelegate];
        [app activateIgnoringOtherApps:YES];
        [app run];
    }
    return 0;
} 