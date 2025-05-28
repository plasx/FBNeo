#!/bin/bash
# Build a standalone Metal app without FBNeo integration

set -e

# Create directories
mkdir -p build/metal

# Create a Metal app
cat > build/metal/metal_app.mm << 'EOL'
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <Cocoa/Cocoa.h>

// Global variables
bool g_bMetalInitialized = false;
unsigned char* g_pFrameBuffer = NULL;
int g_nFrameWidth = 384;
int g_nFrameHeight = 224;

// Metal objects
id<MTLDevice> g_device = nil;
id<MTLTexture> g_texture = nil;
MTKView* g_metalView = nil;

@interface MetalView : MTKView
@property (nonatomic, strong) id<MTLRenderPipelineState> pipelineState;
@property (nonatomic, strong) id<MTLCommandQueue> commandQueue;
@property (nonatomic, strong) id<MTLBuffer> vertexBuffer;
@property (nonatomic, strong) id<MTLBuffer> indexBuffer;
@end

@implementation MetalView

- (instancetype)initWithFrame:(NSRect)frameRect device:(id<MTLDevice>)device {
    NSLog(@"MetalView initWithFrame called: %@, device: %@", NSStringFromRect(frameRect), device);
    self = [super initWithFrame:frameRect device:device];
    if (self) {
        [self setupMetal];
    }
    return self;
}

- (void)setupMetal {
    NSLog(@"Setting up Metal renderer...");
    
    // Create a command queue
    self.commandQueue = [self.device newCommandQueue];
    
    // Enable continuous drawing
    self.enableSetNeedsDisplay = NO;
    self.paused = NO;
    self.clearColor = MTLClearColorMake(0.2, 0.0, 0.3, 1.0); // Purple for visibility
    
    // Create a texture
    MTLTextureDescriptor *textureDescriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                                                                                                 width:g_nFrameWidth
                                                                                                height:g_nFrameHeight
                                                                                             mipmapped:NO];
    g_texture = [self.device newTextureWithDescriptor:textureDescriptor];
    
    // Create vertex buffer (quad)
    float quadVertices[] = {
        -1.0, -1.0, 0.0, 1.0,  0.0, 1.0, // bottom left
         1.0, -1.0, 0.0, 1.0,  1.0, 1.0, // bottom right
         1.0,  1.0, 0.0, 1.0,  1.0, 0.0, // top right
        -1.0,  1.0, 0.0, 1.0,  0.0, 0.0  // top left
    };
    self.vertexBuffer = [self.device newBufferWithBytes:quadVertices
                                                 length:sizeof(quadVertices)
                                                options:MTLResourceStorageModeShared];
    
    // Create index buffer
    uint16_t quadIndices[] = {
        0, 1, 2,
        2, 3, 0
    };
    self.indexBuffer = [self.device newBufferWithBytes:quadIndices
                                                length:sizeof(quadIndices)
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
    
    // Compile the shader
    NSError *error = nil;
    id<MTLLibrary> library = [self.device newLibraryWithSource:shaderSource options:nil error:&error];
    if (!library) {
        NSLog(@"Failed to create shader library: %@", error);
        return;
    }
    
    id<MTLFunction> vertexFunction = [library newFunctionWithName:@"vertexShader"];
    id<MTLFunction> fragmentFunction = [library newFunctionWithName:@"fragmentShader"];
    
    // Create the pipeline
    MTLRenderPipelineDescriptor *pipelineDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
    pipelineDescriptor.vertexFunction = vertexFunction;
    pipelineDescriptor.fragmentFunction = fragmentFunction;
    pipelineDescriptor.colorAttachments[0].pixelFormat = self.colorPixelFormat;
    
    // Create vertex descriptor
    MTLVertexDescriptor *vertexDescriptor = [MTLVertexDescriptor vertexDescriptor];
    vertexDescriptor.attributes[0].format = MTLVertexFormatFloat4;
    vertexDescriptor.attributes[0].offset = 0;
    vertexDescriptor.attributes[0].bufferIndex = 0;
    vertexDescriptor.attributes[1].format = MTLVertexFormatFloat2;
    vertexDescriptor.attributes[1].offset = 16;
    vertexDescriptor.attributes[1].bufferIndex = 0;
    vertexDescriptor.layouts[0].stride = 24;
    pipelineDescriptor.vertexDescriptor = vertexDescriptor;
    
    // Create pipeline state
    self.pipelineState = [self.device newRenderPipelineStateWithDescriptor:pipelineDescriptor
                                                                     error:&error];
    if (!self.pipelineState) {
        NSLog(@"Failed to create pipeline state: %@", error);
        return;
    }
    
    // Create frame buffer
    g_pFrameBuffer = (unsigned char*)malloc(g_nFrameWidth * g_nFrameHeight * 4);
    if (g_pFrameBuffer) {
        // Fill with a pattern
        uint32_t* pixel = (uint32_t*)g_pFrameBuffer;
        for (int y = 0; y < g_nFrameHeight; y++) {
            for (int x = 0; x < g_nFrameWidth; x++) {
                *(pixel++) = 0xFF000000 | (y << 16) | (x << 8) | ((x^y) & 0xFF);
            }
        }
        
        // Update texture
        MTLRegion region = MTLRegionMake2D(0, 0, g_nFrameWidth, g_nFrameHeight);
        [g_texture replaceRegion:region mipmapLevel:0 withBytes:g_pFrameBuffer bytesPerRow:g_nFrameWidth * 4];
    }
    
    // Save the view
    g_metalView = self;
    
    NSLog(@"Metal setup completed successfully");
    g_bMetalInitialized = true;
}

- (void)drawRect:(NSRect)dirtyRect {
    static int frameCount = 0;
    frameCount++;
    
    if (frameCount % 60 == 0) {
        NSLog(@"Drawing frame %d", frameCount);
    }
    
    id<MTLCommandBuffer> commandBuffer = [self.commandQueue commandBuffer];
    
    MTLRenderPassDescriptor *renderPassDescriptor = self.currentRenderPassDescriptor;
    if (renderPassDescriptor != nil) {
        id<MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
        
        [renderEncoder setRenderPipelineState:self.pipelineState];
        [renderEncoder setVertexBuffer:self.vertexBuffer offset:0 atIndex:0];
        [renderEncoder setFragmentTexture:g_texture atIndex:0];
        [renderEncoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                                  indexCount:6
                                   indexType:MTLIndexTypeUInt16
                                 indexBuffer:self.indexBuffer
                           indexBufferOffset:0];
        
        [renderEncoder endEncoding];
        
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
    NSLog(@"Application launched");
    
    // Create menu
    NSMenu *menubar = [NSMenu new];
    NSMenuItem *appMenuItem = [NSMenuItem new];
    [menubar addItem:appMenuItem];
    [NSApp setMainMenu:menubar];
    
    NSMenu *appMenu = [NSMenu new];
    NSMenuItem *quitMenuItem = [[NSMenuItem alloc] initWithTitle:@"Quit" 
                                                         action:@selector(terminate:) 
                                                  keyEquivalent:@"q"];
    [appMenu addItem:quitMenuItem];
    [appMenuItem setSubmenu:appMenu];
    
    // Create window
    NSRect frame = NSMakeRect(100, 100, 640, 480);
    NSUInteger styleMask = NSWindowStyleMaskTitled | 
                           NSWindowStyleMaskClosable | 
                           NSWindowStyleMaskMiniaturizable | 
                           NSWindowStyleMaskResizable;
    
    self.window = [[NSWindow alloc] initWithContentRect:frame
                                             styleMask:styleMask
                                               backing:NSBackingStoreBuffered
                                                 defer:NO];
    
    self.window.title = @"Metal Standalone";
    [self.window center];
    
    // Create Metal view
    g_device = MTLCreateSystemDefaultDevice();
    if (!g_device) {
        NSLog(@"Metal is not supported on this device");
        [NSApp terminate:nil];
        return;
    }
    
    self.metalView = [[MetalView alloc] initWithFrame:self.window.contentView.bounds device:g_device];
    self.metalView.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
    self.metalView.clearColor = MTLClearColorMake(0.2, 0.0, 0.3, 1.0);
    self.metalView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    
    [self.window.contentView addSubview:self.metalView];
    
    // Create status label
    self.statusLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(20, 20, 600, 24)];
    self.statusLabel.stringValue = @"FBNeo Metal Standalone";
    self.statusLabel.bezeled = NO;
    self.statusLabel.drawsBackground = NO;
    self.statusLabel.editable = NO;
    self.statusLabel.selectable = NO;
    self.statusLabel.textColor = [NSColor whiteColor];
    
    [self.metalView addSubview:self.statusLabel];
    
    // Show window
    [self.window makeKeyAndOrderFront:nil];
    
    // Start timer
    self.timer = [NSTimer scheduledTimerWithTimeInterval:1.0/60.0
                                                  target:self
                                                selector:@selector(update)
                                                userInfo:nil
                                                 repeats:YES];
    
    NSLog(@"Window setup complete");
}

- (void)update {
    // Update status label with time
    NSDateFormatter *formatter = [[NSDateFormatter alloc] init];
    [formatter setDateFormat:@"HH:mm:ss"];
    NSString *timeString = [formatter stringFromDate:[NSDate date]];
    
    self.statusLabel.stringValue = [NSString stringWithFormat:@"Metal Standalone - %@", timeString];
    
    // Make the view redraw
    [self.metalView setNeedsDisplay:YES];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    return YES;
}

- (void)applicationWillTerminate:(NSNotification *)notification {
    // Clean up
    [self.timer invalidate];
    
    if (g_pFrameBuffer) {
        free(g_pFrameBuffer);
        g_pFrameBuffer = NULL;
    }
}

@end

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        [NSApplication sharedApplication];
        
        AppDelegate *delegate = [[AppDelegate alloc] init];
        [NSApp setDelegate:delegate];
        
        [NSApp finishLaunching];
        [NSApp activateIgnoringOtherApps:YES];
        [NSApp run];
    }
    
    return 0;
}
EOL

# Build the app
echo "Building Metal standalone app..."
clang++ -g -O2 -Wall -fobjc-arc \
    -framework Metal -framework MetalKit -framework Cocoa -framework CoreGraphics \
    build/metal/metal_app.mm \
    -o fbneo_metal_standalone

# Make it executable
chmod +x fbneo_metal_standalone

echo ""
echo "Build complete! You can now run ./fbneo_metal_standalone"
echo "" 