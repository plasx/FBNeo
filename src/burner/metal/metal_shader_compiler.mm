#import <Metal/Metal.h>
#import <Foundation/Foundation.h>

// Metal shader compiler class
@interface MetalShaderCompiler : NSObject

// Shared instance
+ (instancetype)shared;

// Compile a shader library from source
- (id<MTLLibrary>)compileLibraryWithSource:(NSString*)source 
                                    device:(id<MTLDevice>)device 
                                     error:(NSError**)error;

// Load a shader library from a Metal library file
- (id<MTLLibrary>)loadLibraryWithURL:(NSURL*)url 
                              device:(id<MTLDevice>)device 
                               error:(NSError**)error;

// Create a render pipeline state
- (id<MTLRenderPipelineState>)createRenderPipelineStateWithDevice:(id<MTLDevice>)device
                                                    vertexFunction:(NSString*)vertexFunctionName
                                                  fragmentFunction:(NSString*)fragmentFunctionName
                                                      pixelFormat:(MTLPixelFormat)pixelFormat
                                                           error:(NSError**)error;

// Create a compute pipeline state
- (id<MTLComputePipelineState>)createComputePipelineStateWithDevice:(id<MTLDevice>)device
                                                          function:(NSString*)functionName
                                                             error:(NSError**)error;
@end

// Implementation
@implementation MetalShaderCompiler

// Shared instance
+ (instancetype)shared {
    static MetalShaderCompiler* sharedInstance = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        sharedInstance = [[self alloc] init];
    });
    return sharedInstance;
}

// Compile a shader library from source
- (id<MTLLibrary>)compileLibraryWithSource:(NSString*)source 
                                    device:(id<MTLDevice>)device 
                                     error:(NSError**)error {
    MTLCompileOptions* options = [[MTLCompileOptions alloc] init];
    options.fastMathEnabled = YES; // Enable fast math for better performance
    
    id<MTLLibrary> library = [device newLibraryWithSource:source options:options error:error];
    
    return library;
}

// Load a shader library from a Metal library file
- (id<MTLLibrary>)loadLibraryWithURL:(NSURL*)url 
                              device:(id<MTLDevice>)device 
                               error:(NSError**)error {
    id<MTLLibrary> library = [device newLibraryWithURL:url error:error];
    
    if (!library && error && *error) {
        NSLog(@"Failed to load Metal library from %@: %@", url, [*error localizedDescription]);
    }
    
    return library;
}

// Search for Metal library in common locations
- (id<MTLLibrary>)loadDefaultLibraryWithDevice:(id<MTLDevice>)device error:(NSError**)error {
    NSArray<NSString*>* searchPaths = @[
        @"Shaders.metallib", // Current directory
        @"bin/metal/Shaders.metallib", // bin/metal directory
        @"../bin/metal/Shaders.metallib", // One level up
        @"Resources/Shaders.metallib" // Resources directory
    ];
    
    for (NSString* path in searchPaths) {
        NSURL* url = [NSURL fileURLWithPath:path];
        if ([[NSFileManager defaultManager] fileExistsAtPath:[url path]]) {
            id<MTLLibrary> library = [self loadLibraryWithURL:url device:device error:error];
            if (library) {
                return library;
            }
        }
    }
    
    // Try to find in the bundle
    NSURL* bundleURL = [[NSBundle mainBundle] URLForResource:@"Shaders" withExtension:@"metallib"];
    if (bundleURL) {
        id<MTLLibrary> library = [self loadLibraryWithURL:bundleURL device:device error:error];
        if (library) {
            return library;
        }
    }
    
    // If all else fails, try the default library
    return [device newDefaultLibrary];
}

// Create a render pipeline state
- (id<MTLRenderPipelineState>)createRenderPipelineStateWithDevice:(id<MTLDevice>)device
                                                    vertexFunction:(NSString*)vertexFunctionName
                                                  fragmentFunction:(NSString*)fragmentFunctionName
                                                      pixelFormat:(MTLPixelFormat)pixelFormat
                                                           error:(NSError**)error {
    // Load the default library
    id<MTLLibrary> library = [self loadDefaultLibraryWithDevice:device error:error];
    if (!library) {
        return nil;
    }
    
    // Get the functions
    id<MTLFunction> vertexFunction = [library newFunctionWithName:vertexFunctionName];
    id<MTLFunction> fragmentFunction = [library newFunctionWithName:fragmentFunctionName];
    
    if (!vertexFunction || !fragmentFunction) {
        if (error) {
            *error = [NSError errorWithDomain:@"com.fbneo.metal"
                                         code:1
                                     userInfo:@{NSLocalizedDescriptionKey: @"Failed to find shader functions"}];
        }
        return nil;
    }
    
    // Create a pipeline descriptor
    MTLRenderPipelineDescriptor* pipelineDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
    pipelineDescriptor.vertexFunction = vertexFunction;
    pipelineDescriptor.fragmentFunction = fragmentFunction;
    pipelineDescriptor.colorAttachments[0].pixelFormat = pixelFormat;
    
    // Create the pipeline state
    id<MTLRenderPipelineState> pipelineState = [device newRenderPipelineStateWithDescriptor:pipelineDescriptor
                                                                                      error:error];
    
    return pipelineState;
}

// Create a compute pipeline state
- (id<MTLComputePipelineState>)createComputePipelineStateWithDevice:(id<MTLDevice>)device
                                                          function:(NSString*)functionName
                                                             error:(NSError**)error {
    // Load the default library
    id<MTLLibrary> library = [self loadDefaultLibraryWithDevice:device error:error];
    if (!library) {
        return nil;
    }
    
    // Get the function
    id<MTLFunction> function = [library newFunctionWithName:functionName];
    
    if (!function) {
        if (error) {
            *error = [NSError errorWithDomain:@"com.fbneo.metal"
                                         code:1
                                     userInfo:@{NSLocalizedDescriptionKey: @"Failed to find compute function"}];
        }
        return nil;
    }
    
    // Create the pipeline state
    id<MTLComputePipelineState> pipelineState = [device newComputePipelineStateWithFunction:function
                                                                                      error:error];
    
    return pipelineState;
}

@end

// C interface for the shader compiler
extern "C" {
    // Compile a shader library from source
    void* Metal_CompileLibraryWithSource(const char* source, void* device) {
        if (!source || !device) {
            return NULL;
        }
        
        NSString* sourceStr = [NSString stringWithUTF8String:source];
        id<MTLDevice> mtlDevice = (__bridge id<MTLDevice>)device;
        
        NSError* error = nil;
        id<MTLLibrary> library = [MetalShaderCompiler.shared compileLibraryWithSource:sourceStr
                                                                              device:mtlDevice
                                                                               error:&error];
        
        if (!library) {
            NSLog(@"Failed to compile shader: %@", [error localizedDescription]);
            return NULL;
        }
        
        return (__bridge_retained void*)library;
    }
    
    // Load a shader library from a file
    void* Metal_LoadLibraryFromFile(const char* path, void* device) {
        if (!path || !device) {
            return NULL;
        }
        
        NSURL* url = [NSURL fileURLWithPath:[NSString stringWithUTF8String:path]];
        id<MTLDevice> mtlDevice = (__bridge id<MTLDevice>)device;
        
        NSError* error = nil;
        id<MTLLibrary> library = [MetalShaderCompiler.shared loadLibraryWithURL:url
                                                                        device:mtlDevice
                                                                         error:&error];
        
        if (!library) {
            NSLog(@"Failed to load shader library: %@", [error localizedDescription]);
            return NULL;
        }
        
        return (__bridge_retained void*)library;
    }
    
    // Create a render pipeline state
    void* Metal_CreateRenderPipeline(void* device, const char* vertexFunc, const char* fragmentFunc, int pixelFormat) {
        if (!device || !vertexFunc || !fragmentFunc) {
            return NULL;
        }
        
        id<MTLDevice> mtlDevice = (__bridge id<MTLDevice>)device;
        NSString* vertexFuncStr = [NSString stringWithUTF8String:vertexFunc];
        NSString* fragmentFuncStr = [NSString stringWithUTF8String:fragmentFunc];
        
        NSError* error = nil;
        id<MTLRenderPipelineState> pipeline = [MetalShaderCompiler.shared createRenderPipelineStateWithDevice:mtlDevice
                                                                                               vertexFunction:vertexFuncStr
                                                                                             fragmentFunction:fragmentFuncStr
                                                                                                 pixelFormat:(MTLPixelFormat)pixelFormat
                                                                                                      error:&error];
        
        if (!pipeline) {
            NSLog(@"Failed to create render pipeline: %@", [error localizedDescription]);
            return NULL;
        }
        
        return (__bridge_retained void*)pipeline;
    }
    
    // Release a Metal object
    void Metal_ReleaseObject(void* obj) {
        if (obj) {
            CFRelease(obj);
        }
    }
} 