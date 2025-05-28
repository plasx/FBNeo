#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>

// Simple Metal app that shows a test pattern
@interface TestPatternView : NSView <MTKViewDelegate>
@property (nonatomic, strong) MTKView *mtkView;
@property (nonatomic, strong) id<MTLDevice> device;
@property (nonatomic, strong) id<MTLCommandQueue> commandQueue;
@property (nonatomic, strong) id<MTLTexture> texture;
@property (nonatomic, strong) id<MTLRenderPipelineState> pipelineState;
@end

@implementation TestPatternView

- (instancetype)initWithFrame:(NSRect)frameRect {
    if (self = [super initWithFrame:frameRect]) {
        // Create a Metal device
        self.device = MTLCreateSystemDefaultDevice();
        if (!self.device) {
            NSLog(@"Metal is not supported on this device");
            return nil;
        }
        
        // Create a Metal view
        self.mtkView = [[MTKView alloc] initWithFrame:self.bounds device:self.device];
        self.mtkView.delegate = self;
        self.mtkView.clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);
        [self addSubview:self.mtkView];
        
        // Set up Metal
        [self setupMetal];
        
        // Create and update test pattern
        [self createTexture];
        [self generateTestPattern];
    }
    return self;
}

- (void)setupMetal {
    self.commandQueue = [self.device newCommandQueue];
    
    // Create shaders
    NSString *shaderSrc = @"#include <metal_stdlib>\n"
                           "using namespace metal;\n"
                           "\n"
                           "struct VertexOut {\n"
                           "    float4 position [[position]];\n"
                           "    float2 texCoord;\n"
                           "};\n"
                           "\n"
                           "vertex VertexOut vertexShader(uint vertexID [[vertex_id]]) {\n"
                           "    const float2 positions[] = {\n"
                           "        float2(-1.0, -1.0),\n"
                           "        float2( 1.0, -1.0),\n"
                           "        float2(-1.0,  1.0),\n"
                           "        float2( 1.0,  1.0),\n"
                           "    };\n"
                           "    \n"
                           "    const float2 texCoords[] = {\n"
                           "        float2(0.0, 1.0),\n"
                           "        float2(1.0, 1.0),\n"
                           "        float2(0.0, 0.0),\n"
                           "        float2(1.0, 0.0),\n"
                           "    };\n"
                           "    \n"
                           "    VertexOut out;\n"
                           "    out.position = float4(positions[vertexID], 0.0, 1.0);\n"
                           "    out.texCoord = texCoords[vertexID];\n"
                           "    return out;\n"
                           "}\n"
                           "\n"
                           "fragment float4 fragmentShader(VertexOut in [[stage_in]],\n"
                           "                             texture2d<float> tex [[texture(0)]]) {\n"
                           "    constexpr sampler s(mag_filter::nearest, min_filter::nearest);\n"
                           "    return tex.sample(s, in.texCoord);\n"
                           "}\n";
    
    NSError *error = nil;
    id<MTLLibrary> library = [self.device newLibraryWithSource:shaderSrc options:nil error:&error];
    if (!library) {
        NSLog(@"Failed to create shader library: %@", error);
        return;
    }
    
    // Create pipeline state
    MTLRenderPipelineDescriptor *pipelineDesc = [[MTLRenderPipelineDescriptor alloc] init];
    pipelineDesc.label = @"Texture Pipeline";
    pipelineDesc.vertexFunction = [library newFunctionWithName:@"vertexShader"];
    pipelineDesc.fragmentFunction = [library newFunctionWithName:@"fragmentShader"];
    pipelineDesc.colorAttachments[0].pixelFormat = self.mtkView.colorPixelFormat;
    
    self.pipelineState = [self.device newRenderPipelineStateWithDescriptor:pipelineDesc error:&error];
    if (!self.pipelineState) {
        NSLog(@"Failed to create pipeline state: %@", error);
    }
}

- (void)createTexture {
    MTLTextureDescriptor *textureDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                                                                           width:384
                                                                                          height:224
                                                                                       mipmapped:NO];
    textureDesc.usage = MTLTextureUsageShaderRead;
    self.texture = [self.device newTextureWithDescriptor:textureDesc];
}

- (void)generateTestPattern {
    int width = 384;
    int height = 224;
    uint32_t *pixels = (uint32_t*)malloc(width * height * sizeof(uint32_t));
    
    // Create a colorful test pattern
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            uint32_t color;
            
            // Divide into quadrants with different colors
            if (y < height/2) {
                if (x < width/2) {
                    // Top-left: Red
                    color = 0xFF0000FF; // BGRA
                } else {
                    // Top-right: Green
                    color = 0xFF00FF00; // BGRA
                }
            } else {
                if (x < width/2) {
                    // Bottom-left: Blue
                    color = 0xFFFF0000; // BGRA
                } else {
                    // Bottom-right: Yellow
                    color = 0xFF00FFFF; // BGRA
                }
            }
            
            // Grid lines
            if (x % 32 == 0 || y % 32 == 0) {
                color = 0xFFFFFFFF; // White grid
            }
            
            pixels[y * width + x] = color;
        }
    }
    
    // Update texture with pixel data
    [self.texture replaceRegion:MTLRegionMake2D(0, 0, width, height)
                    mipmapLevel:0
                      withBytes:pixels
                    bytesPerRow:width * sizeof(uint32_t)];
    
    free(pixels);
}

#pragma mark - MTKViewDelegate

- (void)mtkView:(MTKView *)view drawableSizeWillChange:(CGSize)size {
    // Handle resize if needed
}

- (void)drawInMTKView:(MTKView *)view {
    id<MTLCommandBuffer> commandBuffer = [self.commandQueue commandBuffer];
    
    if (view.currentRenderPassDescriptor && view.currentDrawable) {
        id<MTLRenderCommandEncoder> encoder = [commandBuffer renderCommandEncoderWithDescriptor:view.currentRenderPassDescriptor];
        
        [encoder setRenderPipelineState:self.pipelineState];
        [encoder setFragmentTexture:self.texture atIndex:0];
        [encoder drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];
        [encoder endEncoding];
        
        [commandBuffer presentDrawable:view.currentDrawable];
    }
    
    [commandBuffer commit];
}

@end

@interface AppDelegate : NSObject <NSApplicationDelegate>
@property (nonatomic, strong) NSWindow *window;
@property (nonatomic, strong) TestPatternView *testView;
@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    // Create a window
    NSRect frame = NSMakeRect(100, 100, 800, 600);
    self.window = [[NSWindow alloc] initWithContentRect:frame
                                              styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable
                                                backing:NSBackingStoreBuffered
                                                  defer:NO];
    [self.window setTitle:@"FBNeo Metal Test Pattern"];
    [self.window center];
    
    // Create test pattern view
    self.testView = [[TestPatternView alloc] initWithFrame:frame];
    self.window.contentView = self.testView;
    
    // Show window
    [self.window makeKeyAndOrderFront:nil];
}

@end

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        NSApplication *app = [NSApplication sharedApplication];
        AppDelegate *delegate = [[AppDelegate alloc] init];
        [app setDelegate:delegate];
        [app activateIgnoringOtherApps:YES];
        [app run];
    }
    return 0;
} 