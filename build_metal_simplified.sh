#!/bin/bash
# Simplified build script for FBNeo Metal - focusing on essential files

set -e  # Exit on error

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
YELLOW='\033[0;33m'
RESET='\033[0m'

echo -e "${BLUE}Building FBNeo Metal frontend (simplified version)...${RESET}"

# Set compiler flags
ARCH="-arch arm64"
CFLAGS="-std=c11 -O1 -DHAVE_STDINT_H -DHAVE_STDBOOL_H -DMETAL_IMPLEMENTATION_FILE -DUSE_METAL_RENDERER -DTCHAR_DEFINED=1"
CXXFLAGS="-std=c++17 -O1 -DMACOSX -DUSE_METAL_FIXES -DMETAL_IMPLEMENTATION_FILE -DUSE_METAL_RENDERER -DTCHAR_DEFINED=1"
OBJCXXFLAGS="-std=c++17 -O1 -fobjc-arc -DMACOSX -DUSE_METAL_FIXES -DMETAL_IMPLEMENTATION_FILE -DUSE_METAL_RENDERER -DTCHAR_DEFINED=1"
INCLUDES="-Isrc/burner/metal -Isrc -I$BUILD_DIR"
FRAMEWORKS="-framework Cocoa -framework Metal -framework MetalKit -framework CoreGraphics -framework QuartzCore"
LIBS=""

# Setup build directory
mkdir -p build/simplified
BUILD_DIR="build/simplified"

# Clean up any previous build
rm -f fbneo_metal_simple

# Create minimal error handling file
cat > $BUILD_DIR/error_handling.c << 'EOL'
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef struct {
    int code;
    char message[256];
} ErrorInfo;

static ErrorInfo g_lastError = {0};

void Metal_SetError(int code, const char* message) {
    g_lastError.code = code;
    if (message) {
        strncpy(g_lastError.message, message, sizeof(g_lastError.message) - 1);
    }
    printf("ERROR: %s (code %d)\n", message, code);
}

bool Metal_HasError(void) {
    return g_lastError.code != 0;
}

const char* Metal_GetLastErrorMessage(void) {
    return g_lastError.message;
}

void Metal_SetDebugMode(bool enable) {
    printf("Debug mode: %s\n", enable ? "ON" : "OFF");
}

int Metal_EnableFallbackAudio(void) {
    return 0;
}
EOL

# Create minimal frame buffer
cat > $BUILD_DIR/frame_buffer.c << 'EOL'
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

typedef struct {
    uint32_t* data;
    int width;
    int height;
    int pitch;
    bool updated;
} FrameBuffer;

FrameBuffer g_frameBuffer = {0};

bool InitFrameBuffer(int width, int height) {
    if (g_frameBuffer.data) {
        free(g_frameBuffer.data);
    }
    
    g_frameBuffer.data = malloc(width * height * sizeof(uint32_t));
    if (!g_frameBuffer.data) {
        printf("Failed to allocate frame buffer\n");
        return false;
    }
    
    g_frameBuffer.width = width;
    g_frameBuffer.height = height;
    g_frameBuffer.pitch = width * sizeof(uint32_t);
    memset(g_frameBuffer.data, 0, width * height * sizeof(uint32_t));
    
    return true;
}

int Metal_GenerateTestPattern(int patternType) {
    int width = g_frameBuffer.width;
    int height = g_frameBuffer.height;
    
    if (!g_frameBuffer.data) {
        return -1;
    }
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            uint32_t color = ((x / 16) + (y / 16)) % 2 ? 0xFFFFFFFF : 0xFF000000;
            g_frameBuffer.data[y * width + x] = color;
        }
    }
    
    g_frameBuffer.updated = true;
    return 0;
}
EOL

# Create minimal game status file
cat > $BUILD_DIR/game_status.c << 'EOL'
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

char g_gameTitle[256] = "FBNeo Metal";
bool g_gameRunning = false;

const char* Metal_GetGameTitle(void) {
    return g_gameTitle;
}

void Metal_SetGameTitle(const char* title) {
    if (title) {
        strncpy(g_gameTitle, title, sizeof(g_gameTitle) - 1);
    }
}

bool Metal_IsGameRunning(void) {
    return g_gameRunning;
}

void Metal_SetGameRunning(bool running) {
    g_gameRunning = running;
}

float Metal_GetFrameRate(void) {
    return 60.0f;
}

int Metal_GetTotalFrames(void) {
    return 0;
}
EOL

# Create minimal ROM loader
cat > $BUILD_DIR/rom_loader.c << 'EOL'
#include <stdio.h>
#include <stdbool.h>

bool Metal_LoadAndInitROM(const char* path) {
    printf("Loading ROM: %s\n", path);
    return true;
}

void Metal_UnloadROM(void) {
    printf("Unloading ROM\n");
}
EOL

# Create Objective-C++ Metal renderer
cat > $BUILD_DIR/main_test.mm << 'EOL'
#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#include <stdio.h>

// Struct for frame buffer
typedef struct {
    uint32_t* data;
    int width;
    int height;
    int pitch;
    bool updated;
} FrameBuffer;

// C interface functions declared as extern "C"
extern "C" {
    // Frame buffer access
    extern FrameBuffer g_frameBuffer;
    
    // Game status functions
    extern const char* Metal_GetGameTitle(void);
    extern bool Metal_IsGameRunning(void);
    extern int Metal_GenerateTestPattern(int patternType);
    
    // Error handling
    extern void Metal_SetError(int code, const char* message);
    
    // ROM loading
    extern bool Metal_LoadAndInitROM(const char* path);
    extern void Metal_UnloadROM(void);
}

@interface MetalRenderer : NSObject<MTKViewDelegate>
@property (nonatomic, strong) id<MTLDevice> device;
@property (nonatomic, strong) id<MTLCommandQueue> commandQueue;
@property (nonatomic, strong) id<MTLTexture> texture;
@property (nonatomic, strong) id<MTLRenderPipelineState> pipelineState;
@property (nonatomic, strong) MTLRenderPassDescriptor *renderPassDescriptor;
@end

@implementation MetalRenderer

- (instancetype)initWithView:(MTKView *)view {
    self = [super init];
    if (self) {
        _device = view.device = MTLCreateSystemDefaultDevice();
        if (!_device) {
            Metal_SetError(-1, "Failed to create Metal device");
            return nil;
        }
        
        _commandQueue = [_device newCommandQueue];
        if (!_commandQueue) {
            Metal_SetError(-1, "Failed to create command queue");
            return nil;
        }
        
        // Initialize the frame buffer
        InitFrameBuffer(384, 224);
        
        // Generate a test pattern
        Metal_GenerateTestPattern(1);
        
        // Create texture descriptor
        MTLTextureDescriptor *textureDescriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                                                                                                     width:g_frameBuffer.width
                                                                                                    height:g_frameBuffer.height
                                                                                                 mipmapped:NO];
        textureDescriptor.usage = MTLTextureUsageShaderRead;
        _texture = [_device newTextureWithDescriptor:textureDescriptor];
        
        // Create render pipeline
        id<MTLLibrary> defaultLibrary = [_device newDefaultLibrary];
        id<MTLFunction> vertexFunction = [defaultLibrary newFunctionWithName:@"vertexShader"];
        id<MTLFunction> fragmentFunction = [defaultLibrary newFunctionWithName:@"fragmentShader"];
        
        MTLRenderPipelineDescriptor *pipelineDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
        pipelineDescriptor.vertexFunction = vertexFunction;
        pipelineDescriptor.fragmentFunction = fragmentFunction;
        pipelineDescriptor.colorAttachments[0].pixelFormat = view.colorPixelFormat;
        
        NSError *error = nil;
        _pipelineState = [_device newRenderPipelineStateWithDescriptor:pipelineDescriptor error:&error];
        if (!_pipelineState) {
            NSLog(@"Failed to create pipeline state: %@", error);
            Metal_SetError(-1, "Failed to create pipeline state");
            return nil;
        }
        
        // Set view properties
        view.delegate = self;
        view.clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);
    }
    return self;
}

- (void)updateTexture {
    if (g_frameBuffer.data && g_frameBuffer.updated) {
        [_texture replaceRegion:MTLRegionMake2D(0, 0, g_frameBuffer.width, g_frameBuffer.height)
                    mipmapLevel:0
                      withBytes:g_frameBuffer.data
                    bytesPerRow:g_frameBuffer.pitch];
        g_frameBuffer.updated = false;
    }
}

- (void)drawInMTKView:(MTKView *)view {
    [self updateTexture];
    
    id<MTLCommandBuffer> commandBuffer = [_commandQueue commandBuffer];
    MTLRenderPassDescriptor *renderPassDescriptor = view.currentRenderPassDescriptor;
    
    if (renderPassDescriptor) {
        id<MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
        
        [renderEncoder setRenderPipelineState:_pipelineState];
        // Add drawing code here when we have proper rendering
        
        [renderEncoder endEncoding];
        [commandBuffer presentDrawable:view.currentDrawable];
    }
    
    [commandBuffer commit];
}

- (void)mtkView:(MTKView *)view drawableSizeWillChange:(CGSize)size {
    // Handle resize if needed
}

@end

// Main application class
@interface AppDelegate : NSObject <NSApplicationDelegate>
@property (nonatomic, strong) NSWindow *window;
@property (nonatomic, strong) MTKView *metalView;
@property (nonatomic, strong) MetalRenderer *renderer;
@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    NSRect frame = NSMakeRect(0, 0, 800, 600);
    _window = [[NSWindow alloc] initWithContentRect:frame
                                          styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable
                                            backing:NSBackingStoreBuffered
                                              defer:NO];
    [_window setTitle:@"FBNeo Metal"];
    
    _metalView = [[MTKView alloc] initWithFrame:frame];
    _metalView.device = MTLCreateSystemDefaultDevice();
    _metalView.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
    _metalView.clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);
    
    _renderer = [[MetalRenderer alloc] initWithView:_metalView];
    
    _window.contentView = _metalView;
    [_window makeKeyAndOrderFront:nil];
    [_window center];
    
    // Process command line arguments
    NSArray *args = [[NSProcessInfo processInfo] arguments];
    if (args.count > 1) {
        NSString *romPath = args[1];
        Metal_LoadAndInitROM([romPath UTF8String]);
        Metal_SetGameTitle([[NSString stringWithFormat:@"FBNeo - %@", [romPath lastPathComponent]] UTF8String]);
        [_window setTitle:[NSString stringWithUTF8String:Metal_GetGameTitle()]];
    }
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    return YES;
}

- (void)applicationWillTerminate:(NSNotification *)notification {
    Metal_UnloadROM();
}

@end

// Main function
int main(int argc, const char * argv[]) {
    @autoreleasepool {
        NSApplication *app = [NSApplication sharedApplication];
        AppDelegate *appDelegate = [[AppDelegate alloc] init];
        [app setDelegate:appDelegate];
        [app run];
    }
    return 0;
}
EOL

# Create the Metal shader file
cat > $BUILD_DIR/default.metal << 'EOL'
#include <metal_stdlib>
using namespace metal;

// Vertex shader inputs
struct VertexInput {
    float4 position [[position]];
    float2 texCoord;
};

// Vertex shader outputs and fragment shader inputs
struct RasterizerData {
    float4 position [[position]];
    float2 texCoord;
};

// Vertex shader
vertex RasterizerData vertexShader(uint vertexID [[vertex_id]]) {
    // Define a triangle filling the screen
    const float4 positions[] = {
        float4(-1.0, -1.0, 0.0, 1.0),
        float4( 3.0, -1.0, 0.0, 1.0),
        float4(-1.0,  3.0, 0.0, 1.0)
    };
    
    const float2 texCoords[] = {
        float2(0.0, 1.0),
        float2(2.0, 1.0),
        float2(0.0, -1.0)
    };
    
    RasterizerData out;
    out.position = positions[vertexID];
    out.texCoord = texCoords[vertexID];
    
    return out;
}

// Fragment shader
fragment float4 fragmentShader(RasterizerData in [[stage_in]], 
                               texture2d<float> texture [[texture(0)]]) {
    constexpr sampler textureSampler(mag_filter::linear, min_filter::linear);
    float4 color = texture.sample(textureSampler, in.texCoord);
    return color;
}
EOL

# Add extern "C" function declarations for C++/Objective-C++ 
cat > $BUILD_DIR/metal_bridge.h << 'EOL'
#ifndef METAL_BRIDGE_H
#define METAL_BRIDGE_H

#ifdef __cplusplus
extern "C" {
#endif

// Frame buffer structure
typedef struct {
    uint32_t* data;
    int width;
    int height;
    int pitch;
    bool updated;
} FrameBuffer;

// Frame buffer functions
bool InitFrameBuffer(int width, int height);
int Metal_GenerateTestPattern(int patternType);

// Game status functions
const char* Metal_GetGameTitle(void);
void Metal_SetGameTitle(const char* title);
bool Metal_IsGameRunning(void);
void Metal_SetGameRunning(bool running);
float Metal_GetFrameRate(void);
int Metal_GetTotalFrames(void);

// Error handling
void Metal_SetError(int code, const char* message);
bool Metal_HasError(void);
const char* Metal_GetLastErrorMessage(void);
void Metal_SetDebugMode(bool enable);
int Metal_EnableFallbackAudio(void);

// ROM loading
bool Metal_LoadAndInitROM(const char* path);
void Metal_UnloadROM(void);

#ifdef __cplusplus
}
#endif

#endif // METAL_BRIDGE_H
EOL

# Compile Metal shader
echo -e "${BLUE}Compiling Metal shader...${RESET}"
xcrun -sdk macosx metal -c $BUILD_DIR/default.metal -o $BUILD_DIR/default.air
xcrun -sdk macosx metallib $BUILD_DIR/default.air -o $BUILD_DIR/default.metallib

# Compile C files
echo -e "${BLUE}Compiling C files...${RESET}"
clang $CFLAGS $ARCH $INCLUDES -c $BUILD_DIR/error_handling.c -o $BUILD_DIR/error_handling.o
clang $CFLAGS $ARCH $INCLUDES -c $BUILD_DIR/frame_buffer.c -o $BUILD_DIR/frame_buffer.o
clang $CFLAGS $ARCH $INCLUDES -c $BUILD_DIR/game_status.c -o $BUILD_DIR/game_status.o
clang $CFLAGS $ARCH $INCLUDES -c $BUILD_DIR/rom_loader.c -o $BUILD_DIR/rom_loader.o

# Compile and link Objective-C++ file directly
echo -e "${BLUE}Compiling and linking...${RESET}"
clang++ $OBJCXXFLAGS $ARCH $INCLUDES -o fbneo_metal_simple \
    $BUILD_DIR/error_handling.o \
    $BUILD_DIR/frame_buffer.o \
    $BUILD_DIR/game_status.o \
    $BUILD_DIR/rom_loader.o \
    $BUILD_DIR/main_test.mm \
    $FRAMEWORKS $LIBS

# Copy metallib file to the same directory as the executable
cp $BUILD_DIR/default.metallib ./default.metallib

# Check if build succeeded
if [ $? -eq 0 ]; then
    echo -e "${GREEN}Build successful!${RESET}"
    chmod +x fbneo_metal_simple
    echo -e "${BLUE}Output: ${YELLOW}fbneo_metal_simple${RESET}"
    echo -e "${BLUE}Run it with: ${YELLOW}./fbneo_metal_simple /path/to/rom.zip${RESET}"
else
    echo -e "${RED}Build failed!${RESET}"
    exit 1
fi 