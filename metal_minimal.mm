#import <Cocoa/Cocoa.h>
#import <MetalKit/MetalKit.h>

// Include the simplified renderer
#include "src/burner/metal/simplified_renderer.mm"

@interface MetalMinimalApp : NSObject <NSApplicationDelegate, MTKViewDelegate>
@property (strong) NSWindow *window;
@property (strong) MTKView *metalView;
@property (strong) id<MTLDevice> device;
@property (strong) id<MTLCommandQueue> commandQueue;
@property (strong) id<MTLRenderPipelineState> pipelineState;
@property (strong) id<MTLTexture> frameTexture;
@property (strong) id<MTLBuffer> vertexBuffer;
@property (nonatomic) NSTimer *timer;
@end

@implementation MetalMinimalApp

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    // Create Metal device
    self.device = MTLCreateSystemDefaultDevice();
    if (!self.device) {
        NSLog(@"Metal is not supported on this device");
        return;
    }
    
    NSLog(@"Metal device: %@", self.device.name);
    
    // Create window
    NSRect frame = NSMakeRect(0, 0, 640, 480);
    self.window = [[NSWindow alloc] initWithContentRect:frame
                                              styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable
                                                backing:NSBackingStoreBuffered
                                                  defer:NO];
    self.window.title = @"FBNeo Metal Minimal";
    
    // Create Metal view
    self.metalView = [[MTKView alloc] initWithFrame:frame device:self.device];
    self.metalView.delegate = self;
    self.metalView.clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);
    self.window.contentView = self.metalView;
    
    // Create command queue
    self.commandQueue = [self.device newCommandQueue];
    
    // Setup Metal pipeline
    [self setupRenderPipeline];
    
    // Initialize renderer
    VidInit();
    
    // Create vertex buffer
    [self createVertexBuffer];
    
    // Create texture
    [self createFrameTexture];
    
    // Show window
    [self.window center];
    [self.window makeKeyAndOrderFront:nil];
    
    // Create menu
    [self setupMenu];
    
    // Start animation timer
    self.timer = [NSTimer scheduledTimerWithTimeInterval:1.0/60.0
                                                  target:self
                                                selector:@selector(updateFrame)
                                                userInfo:nil
                                                 repeats:YES];
    
    [NSApp activateIgnoringOtherApps:YES];
}

- (void)setupRenderPipeline {
    // Create metal shaders inline
    NSString *shaderSource = @"\
    #include <metal_stdlib>\n\
    using namespace metal;\n\
    \n\
    struct VertexIn {\n\
        float2 position [[attribute(0)]];\n\
        float2 texCoord [[attribute(1)]];\n\
    };\n\
    \n\
    struct VertexOut {\n\
        float4 position [[position]];\n\
        float2 texCoord;\n\
    };\n\
    \n\
    vertex VertexOut vertexShader(uint vertexID [[vertex_id]],\n\
                                 constant float4 *vertices [[buffer(0)]]) {\n\
        VertexOut out;\n\
        out.position = float4(vertices[vertexID].x, vertices[vertexID].y, 0.0, 1.0);\n\
        out.texCoord = float2(vertices[vertexID].z, vertices[vertexID].w);\n\
        return out;\n\
    }\n\
    \n\
    fragment float4 fragmentShader(VertexOut in [[stage_in]],\n\
                                  texture2d<float> texture [[texture(0)]]) {\n\
        constexpr sampler textureSampler(mag_filter::nearest, min_filter::nearest);\n\
        return texture.sample(textureSampler, in.texCoord);\n\
    }\n";
    
    NSError *error = nil;
    id<MTLLibrary> library = [self.device newLibraryWithSource:shaderSource options:nil error:&error];
    if (!library) {
        NSLog(@"Failed to create Metal library: %@", error);
        return;
    }
    
    id<MTLFunction> vertexFunction = [library newFunctionWithName:@"vertexShader"];
    id<MTLFunction> fragmentFunction = [library newFunctionWithName:@"fragmentShader"];
    
    MTLRenderPipelineDescriptor *pipelineDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
    pipelineDescriptor.vertexFunction = vertexFunction;
    pipelineDescriptor.fragmentFunction = fragmentFunction;
    pipelineDescriptor.colorAttachments[0].pixelFormat = self.metalView.colorPixelFormat;
    
    self.pipelineState = [self.device newRenderPipelineStateWithDescriptor:pipelineDescriptor error:&error];
    if (!self.pipelineState) {
        NSLog(@"Failed to create pipeline state: %@", error);
    }
}

- (void)createVertexBuffer {
    // Create a quad filling the screen
    float quadVertices[] = {
        -1.0f,  1.0f, 0.0f, 0.0f,  // top left
        -1.0f, -1.0f, 0.0f, 1.0f,  // bottom left
         1.0f, -1.0f, 1.0f, 1.0f,  // bottom right
        
        -1.0f,  1.0f, 0.0f, 0.0f,  // top left
         1.0f, -1.0f, 1.0f, 1.0f,  // bottom right
         1.0f,  1.0f, 1.0f, 0.0f   // top right
    };
    
    self.vertexBuffer = [self.device newBufferWithBytes:quadVertices
                                                 length:sizeof(quadVertices)
                                                options:MTLResourceStorageModeShared];
}

- (void)createFrameTexture {
    // Create a texture for our frame buffer
    MTLTextureDescriptor *textureDescriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                                                                                width:320
                                                                                               height:240
                                                                                            mipmapped:NO];
    
    textureDescriptor.usage = MTLTextureUsageShaderRead | MTLTextureUsageRenderTarget;
    textureDescriptor.storageMode = MTLStorageModeShared;
    
    self.frameTexture = [self.device newTextureWithDescriptor:textureDescriptor];
}

- (void)updateFrame {
    // Have our renderer create the test pattern
    RenderTestPattern();
    
    // Request redraw of the view
    [self.metalView setNeedsDisplay:YES];
}

- (void)setupMenu {
    // Create the main menu
    NSMenu *mainMenu = [[NSMenu alloc] init];
    
    // Application menu
    NSMenuItem *appMenuItem = [[NSMenuItem alloc] init];
    NSMenu *appMenu = [[NSMenu alloc] init];
    
    [appMenu addItemWithTitle:@"About FBNeo Minimal" action:@selector(orderFrontStandardAboutPanel:) keyEquivalent:@""];
    [appMenu addItem:[NSMenuItem separatorItem]];
    [appMenu addItemWithTitle:@"Quit" action:@selector(terminate:) keyEquivalent:@"q"];
    
    [appMenuItem setSubmenu:appMenu];
    [mainMenu addItem:appMenuItem];
    
    // View menu
    NSMenuItem *viewMenuItem = [[NSMenuItem alloc] init];
    NSMenu *viewMenu = [[NSMenu alloc] initWithTitle:@"View"];
    
    NSMenuItem *fullscreenItem = [[NSMenuItem alloc] initWithTitle:@"Toggle Fullscreen" action:@selector(toggleFullScreen:) keyEquivalent:@"f"];
    [viewMenu addItem:fullscreenItem];
    
    [viewMenuItem setSubmenu:viewMenu];
    [mainMenu addItem:viewMenuItem];
    
    // Set the menu
    [NSApp setMainMenu:mainMenu];
}

- (void)updateMetalTexture:(const void *)frameData width:(unsigned int)width height:(unsigned int)height {
    if (!self.frameTexture || !frameData) {
        return;
    }
    
    // If dimensions changed, recreate texture
    if (self.frameTexture.width != width || self.frameTexture.height != height) {
        MTLTextureDescriptor *textureDescriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                                                                                  width:width
                                                                                                 height:height
                                                                                              mipmapped:NO];
        textureDescriptor.usage = MTLTextureUsageShaderRead | MTLTextureUsageRenderTarget;
        textureDescriptor.storageMode = MTLStorageModeShared;
        
        self.frameTexture = [self.device newTextureWithDescriptor:textureDescriptor];
    }
    
    // Update the texture with the frame data
    MTLRegion region = MTLRegionMake2D(0, 0, width, height);
    [self.frameTexture replaceRegion:region
                          mipmapLevel:0
                            withBytes:frameData
                          bytesPerRow:width * 4];
}

#pragma mark - MTKViewDelegate

- (void)mtkView:(MTKView *)view drawableSizeWillChange:(CGSize)size {
    // Handle window resize
}

- (void)drawInMTKView:(MTKView *)view {
    if (!self.frameTexture) {
        return;
    }
    
    // Create a command buffer
    id<MTLCommandBuffer> commandBuffer = [self.commandQueue commandBuffer];
    
    // Get the render pass descriptor
    MTLRenderPassDescriptor *renderPassDescriptor = view.currentRenderPassDescriptor;
    
    if (renderPassDescriptor != nil) {
        // Create a render encoder
        id<MTLRenderCommandEncoder> encoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
        
        // Set the pipeline state and vertex buffer
        [encoder setRenderPipelineState:self.pipelineState];
        [encoder setVertexBuffer:self.vertexBuffer offset:0 atIndex:0];
        
        // Set the texture
        [encoder setFragmentTexture:self.frameTexture atIndex:0];
        
        // Draw
        [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:6];
        
        // End encoding
        [encoder endEncoding];
        
        // Present the drawable
        [commandBuffer presentDrawable:view.currentDrawable];
    }
    
    // Commit the command buffer
    [commandBuffer commit];
}

@end

// Implement the function called by the renderer
void UpdateMetalFrameTexture(const void *frameData, unsigned int width, unsigned int height) {
    MetalMinimalApp *delegate = (MetalMinimalApp *)[NSApp delegate];
    [delegate updateMetalTexture:frameData width:width height:height];
}

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        NSApplication *app = [NSApplication sharedApplication];
        app.activationPolicy = NSApplicationActivationPolicyRegular;
        MetalMinimalApp *delegate = [[MetalMinimalApp alloc] init];
        [app setDelegate:delegate];
        [app run];
    }
    return 0;
} 