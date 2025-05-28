#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>

@interface MetalView : MTKView
@property (nonatomic, strong) id<MTLRenderPipelineState> pipelineState;
@property (nonatomic, strong) id<MTLCommandQueue> commandQueue;
@property (nonatomic, strong) id<MTLBuffer> vertexBuffer;
@property (nonatomic, strong) id<MTLBuffer> indexBuffer;
@property (nonatomic, strong) id<MTLTexture> texture;
@end

@implementation MetalView

- (instancetype)initWithFrame:(CGRect)frame device:(id<MTLDevice>)device {
    self = [super initWithFrame:frame device:device];
    if (self) {
        [self setupMetal];
    }
    return self;
}

- (void)setupMetal {
    self.commandQueue = [self.device newCommandQueue];
    self.clearColor = MTLClearColorMake(0.1, 0.0, 0.2, 1.0);
    
    // Create a texture
    MTLTextureDescriptor *textureDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                                                                                          width:384
                                                                                         height:224
                                                                                      mipmapped:NO];
    self.texture = [self.device newTextureWithDescriptor:textureDesc];
    
    // Create a test pattern
    uint32_t *data = (uint32_t*)malloc(384 * 224 * 4);
    for (int y = 0; y < 224; y++) {
        for (int x = 0; x < 384; x++) {
            data[y * 384 + x] = (0xFF << 24) | ((x % 256) << 16) | ((y % 256) << 8) | ((x*y) % 256);
        }
    }
    
    // Upload test pattern
    [self.texture replaceRegion:MTLRegionMake2D(0, 0, 384, 224)
                    mipmapLevel:0
                      withBytes:data
                    bytesPerRow:384 * 4];
    free(data);
    
    // Create vertex data for a quad
    float quadVertices[] = {
        -1.0f, -1.0f, 0.0f, 1.0f,  0.0f, 1.0f,
         1.0f, -1.0f, 0.0f, 1.0f,  1.0f, 1.0f,
         1.0f,  1.0f, 0.0f, 1.0f,  1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f, 1.0f,  0.0f, 0.0f
    };
    self.vertexBuffer = [self.device newBufferWithBytes:quadVertices
                                                 length:sizeof(quadVertices)
                                                options:MTLResourceStorageModeShared];
    
    // Create index data
    uint16_t indices[] = {
        0, 1, 2,
        2, 3, 0
    };
    self.indexBuffer = [self.device newBufferWithBytes:indices
                                                length:sizeof(indices)
                                               options:MTLResourceStorageModeShared];
    
    // Create a shader
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
                             "vertex VertexOut vertexShader(uint vertexID [[vertex_id]],\n"
                             "                              const device VertexIn* vertices [[buffer(0)]]) {\n"
                             "    VertexOut out;\n"
                             "    out.position = vertices[vertexID].position;\n"
                             "    out.texCoord = vertices[vertexID].texCoord;\n"
                             "    return out;\n"
                             "}\n"
                             "\n"
                             "fragment float4 fragmentShader(VertexOut in [[stage_in]],\n"
                             "                               texture2d<float> colorTexture [[texture(0)]]) {\n"
                             "    constexpr sampler textureSampler (mag_filter::linear, min_filter::linear);\n"
                             "    return colorTexture.sample(textureSampler, in.texCoord);\n"
                             "}\n";
    
    // Create a library
    NSError *error = nil;
    id<MTLLibrary> library = [self.device newLibraryWithSource:shaderSource options:nil error:&error];
    if (!library) {
        NSLog(@"Failed to create library: %@", error);
        return;
    }
    
    // Create functions
    id<MTLFunction> vertexFunc = [library newFunctionWithName:@"vertexShader"];
    id<MTLFunction> fragmentFunc = [library newFunctionWithName:@"fragmentShader"];
    
    // Create pipeline
    MTLRenderPipelineDescriptor *pipelineDesc = [[MTLRenderPipelineDescriptor alloc] init];
    pipelineDesc.vertexFunction = vertexFunc;
    pipelineDesc.fragmentFunction = fragmentFunc;
    pipelineDesc.colorAttachments[0].pixelFormat = self.colorPixelFormat;
    
    // Vertex descriptor
    MTLVertexDescriptor *vertexDesc = [MTLVertexDescriptor vertexDescriptor];
    vertexDesc.attributes[0].format = MTLVertexFormatFloat4;
    vertexDesc.attributes[0].offset = 0;
    vertexDesc.attributes[0].bufferIndex = 0;
    vertexDesc.attributes[1].format = MTLVertexFormatFloat2;
    vertexDesc.attributes[1].offset = 16;
    vertexDesc.attributes[1].bufferIndex = 0;
    vertexDesc.layouts[0].stride = 24;
    
    pipelineDesc.vertexDescriptor = vertexDesc;
    
    // Create pipeline state
    self.pipelineState = [self.device newRenderPipelineStateWithDescriptor:pipelineDesc error:&error];
    if (!self.pipelineState) {
        NSLog(@"Failed to create pipeline state: %@", error);
    }
}

- (void)updateTexture:(const void*)data width:(int)width height:(int)height {
    if (!data || width <= 0 || height <= 0) return;
    
    // Check if texture needs to be recreated
    if (self.texture.width != width || self.texture.height != height) {
        MTLTextureDescriptor *textureDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                                                                                              width:width
                                                                                             height:height
                                                                                          mipmapped:NO];
        self.texture = [self.device newTextureWithDescriptor:textureDesc];
    }
    
    // Update the texture
    [self.texture replaceRegion:MTLRegionMake2D(0, 0, width, height)
                    mipmapLevel:0
                      withBytes:data
                    bytesPerRow:width * 4];
    
    // Mark for redraw
    [self setNeedsDisplay:YES];
}

- (void)drawRect:(NSRect)dirtyRect {
    id<MTLCommandBuffer> commandBuffer = [self.commandQueue commandBuffer];
    MTLRenderPassDescriptor *passDesc = self.currentRenderPassDescriptor;
    
    if (passDesc) {
        id<MTLRenderCommandEncoder> encoder = [commandBuffer renderCommandEncoderWithDescriptor:passDesc];
        [encoder setRenderPipelineState:self.pipelineState];
        [encoder setVertexBuffer:self.vertexBuffer offset:0 atIndex:0];
        [encoder setFragmentTexture:self.texture atIndex:0];
        [encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                            indexCount:6
                             indexType:MTLIndexTypeUInt16
                           indexBuffer:self.indexBuffer
                     indexBufferOffset:0];
        [encoder endEncoding];
        
        [commandBuffer presentDrawable:self.currentDrawable];
    }
    
    [commandBuffer commit];
}

@end

@interface AppDelegate : NSObject <NSApplicationDelegate>
@property (nonatomic, strong) NSWindow *window;
@property (nonatomic, strong) MetalView *metalView;
@property (nonatomic, strong) NSTextField *statusLabel;
@property (nonatomic, strong) NSTimer *timer;
@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    // Create window
    NSRect frame = NSMakeRect(100, 100, 800, 600);
    self.window = [[NSWindow alloc] initWithContentRect:frame
                                              styleMask:NSWindowStyleMaskTitled |
                                                        NSWindowStyleMaskClosable |
                                                        NSWindowStyleMaskMiniaturizable |
                                                        NSWindowStyleMaskResizable
                                                backing:NSBackingStoreBuffered
                                                  defer:NO];
    self.window.title = @"FBNeo Metal Demo";
    
    // Create Metal view
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    self.metalView = [[MetalView alloc] initWithFrame:self.window.contentView.bounds device:device];
    self.metalView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    [self.window.contentView addSubview:self.metalView];
    
    // Create status label
    self.statusLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(20, 20, 760, 30)];
    self.statusLabel.stringValue = @"FBNeo Metal Demo - No ROM loaded";
    self.statusLabel.bezeled = NO;
    self.statusLabel.drawsBackground = NO;
    self.statusLabel.editable = NO;
    self.statusLabel.selectable = NO;
    self.statusLabel.textColor = [NSColor whiteColor];
    [self.metalView addSubview:self.statusLabel];
    
    // Show window
    [self.window center];
    [self.window makeKeyAndOrderFront:nil];
    
    // Process command line args
    NSArray *args = [[NSProcessInfo processInfo] arguments];
    if (args.count > 1) {
        NSString *romPath = args[1];
        [self loadROM:romPath];
    }
    
    // Start animation
    self.timer = [NSTimer scheduledTimerWithTimeInterval:1.0/60.0
                                                  target:self
                                                selector:@selector(update)
                                                userInfo:nil
                                                 repeats:YES];
}

- (void)loadROM:(NSString *)romPath {
    // Check if file exists
    BOOL isDir = NO;
    if (![[NSFileManager defaultManager] fileExistsAtPath:romPath isDirectory:&isDir] || isDir) {
        self.statusLabel.stringValue = [NSString stringWithFormat:@"Error: ROM file not found: %@", romPath];
        return;
    }
    
    self.statusLabel.stringValue = [NSString stringWithFormat:@"Attempting to load ROM: %@", romPath];
    
    // Extract filename from path
    NSString *filename = [romPath lastPathComponent];
    
    // For this demo, just update status and show a test pattern
    self.window.title = [NSString stringWithFormat:@"FBNeo Metal Demo - %@", filename];
    self.statusLabel.stringValue = [NSString stringWithFormat:@"ROM loaded: %@", filename];
    
    // Generate a test pattern that looks like a game screen
    int width = 384;
    int height = 224;
    uint32_t *data = (uint32_t*)malloc(width * height * 4);
    
    // Draw a background
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // Dark blue/purple background
            uint8_t r = 20 + (x % 32);
            uint8_t g = 10 + (y % 16);
            uint8_t b = 40 + ((x+y) % 32);
            data[y * width + x] = (0xFF << 24) | (r << 16) | (g << 8) | b;
        }
    }
    
    // Draw some game-like elements
    // Top score area
    for (int y = 10; y < 30; y++) {
        for (int x = 50; x < width-50; x++) {
            data[y * width + x] = (x % 20 < 10) ? 0xFFFFFFFF : 0xFF888888;
        }
    }
    
    // Character
    for (int y = 120; y < 200; y++) {
        for (int x = 150; x < 230; x++) {
            int dx = x - 190;
            int dy = y - 160;
            if (dx*dx + dy*dy < 900) {
                data[y * width + x] = 0xFFFF0000; // Red character
            }
        }
    }
    
    // Update the texture
    [self.metalView updateTexture:data width:width height:height];
    free(data);
}

- (void)update {
    static int frame = 0;
    frame++;
    
    // Example of animation: moving pattern
    int width = 384;
    int height = 224;
    uint32_t *data = (uint32_t*)malloc(width * height * 4);
    
    // Create a different pattern each frame
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int dx = x - 190 + 30 * sin(frame / 30.0);
            int dy = y - 160 + 20 * cos(frame / 20.0);
            
            if (dx*dx + dy*dy < 900) {
                data[y * width + x] = 0xFFFF0000; // Red character
            } else {
                // Dark blue/purple background
                uint8_t r = 20 + (x % 32);
                uint8_t g = 10 + (y % 16);
                uint8_t b = 40 + ((x+y+frame) % 32);
                data[y * width + x] = (0xFF << 24) | (r << 16) | (g << 8) | b;
            }
        }
    }
    
    // Update the text at the top
    for (int y = 10; y < 30; y++) {
        for (int x = 50; x < width-50; x++) {
            if ((x + frame) % 20 < 10) {
                data[y * width + x] = 0xFFFFFFFF;
            } else {
                data[y * width + x] = 0xFF888888;
            }
        }
    }
    
    // Update the texture
    [self.metalView updateTexture:data width:width height:height];
    free(data);
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    return YES;
}

@end

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        NSApplication *app = [NSApplication sharedApplication];
        AppDelegate *delegate = [[AppDelegate alloc] init];
        [app setDelegate:delegate];
        [app run];
    }
    return 0;
}
