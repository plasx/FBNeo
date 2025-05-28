#import <Metal/Metal.h>
#import <Foundation/Foundation.h>

// Metal sampler state provider class
@interface MetalSamplerProvider : NSObject

// Shared instance
+ (instancetype)shared;

// Get a sampler state from the cache
- (id<MTLSamplerState>)samplerStateWithDevice:(id<MTLDevice>)device 
                                     minFilter:(MTLSamplerMinMagFilter)minFilter 
                                     magFilter:(MTLSamplerMinMagFilter)magFilter 
                                  addressMode:(MTLSamplerAddressMode)addressMode;

// Linear filtering sampler (for smooth scaling)
- (id<MTLSamplerState>)linearSamplerWithDevice:(id<MTLDevice>)device;

// Nearest filtering sampler (for pixel-perfect scaling)
- (id<MTLSamplerState>)nearestSamplerWithDevice:(id<MTLDevice>)device;

// Clear the cache
- (void)clearCache;

@end

// Implementation
@implementation MetalSamplerProvider {
    NSMutableDictionary<NSString*, id<MTLSamplerState>>* _samplerStateCache;
}

// Shared instance
+ (instancetype)shared {
    static MetalSamplerProvider* sharedInstance = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        sharedInstance = [[self alloc] init];
    });
    return sharedInstance;
}

// Initialize
- (instancetype)init {
    self = [super init];
    if (self) {
        _samplerStateCache = [NSMutableDictionary dictionary];
    }
    return self;
}

// Get a key for the sampler state cache
- (NSString*)keyForMinFilter:(MTLSamplerMinMagFilter)minFilter 
                   magFilter:(MTLSamplerMinMagFilter)magFilter 
                addressMode:(MTLSamplerAddressMode)addressMode {
    return [NSString stringWithFormat:@"%lu-%lu-%lu", 
            (unsigned long)minFilter, 
            (unsigned long)magFilter, 
            (unsigned long)addressMode];
}

// Get a sampler state with the specified parameters
- (id<MTLSamplerState>)samplerStateWithDevice:(id<MTLDevice>)device 
                                     minFilter:(MTLSamplerMinMagFilter)minFilter 
                                     magFilter:(MTLSamplerMinMagFilter)magFilter 
                                  addressMode:(MTLSamplerAddressMode)addressMode {
    // Check for a cached sampler state
    NSString* key = [self keyForMinFilter:minFilter magFilter:magFilter addressMode:addressMode];
    id<MTLSamplerState> samplerState = _samplerStateCache[key];
    
    if (!samplerState) {
        // Create a new sampler state
        MTLSamplerDescriptor* descriptor = [[MTLSamplerDescriptor alloc] init];
        descriptor.minFilter = minFilter;
        descriptor.magFilter = magFilter;
        descriptor.sAddressMode = addressMode;
        descriptor.tAddressMode = addressMode;
        
        samplerState = [device newSamplerStateWithDescriptor:descriptor];
        
        // Cache the sampler state
        if (samplerState) {
            _samplerStateCache[key] = samplerState;
        }
    }
    
    return samplerState;
}

// Get a linear filtering sampler
- (id<MTLSamplerState>)linearSamplerWithDevice:(id<MTLDevice>)device {
    return [self samplerStateWithDevice:device 
                              minFilter:MTLSamplerMinMagFilterLinear 
                              magFilter:MTLSamplerMinMagFilterLinear 
                           addressMode:MTLSamplerAddressModeClampToEdge];
}

// Get a nearest filtering sampler
- (id<MTLSamplerState>)nearestSamplerWithDevice:(id<MTLDevice>)device {
    return [self samplerStateWithDevice:device 
                              minFilter:MTLSamplerMinMagFilterNearest 
                              magFilter:MTLSamplerMinMagFilterNearest 
                           addressMode:MTLSamplerAddressModeClampToEdge];
}

// Clear the cache
- (void)clearCache {
    [_samplerStateCache removeAllObjects];
}

@end

// C interface for the sampler provider
extern "C" {
    // Get a linear filtering sampler
    void* Metal_GetLinearSampler(void* device) {
        id<MTLDevice> mtlDevice = (__bridge id<MTLDevice>)device;
        return (__bridge_retained void*)[MetalSamplerProvider.shared linearSamplerWithDevice:mtlDevice];
    }
    
    // Get a nearest filtering sampler
    void* Metal_GetNearestSampler(void* device) {
        id<MTLDevice> mtlDevice = (__bridge id<MTLDevice>)device;
        return (__bridge_retained void*)[MetalSamplerProvider.shared nearestSamplerWithDevice:mtlDevice];
    }
    
    // Release a sampler
    void Metal_ReleaseSampler(void* sampler) {
        if (sampler) {
            CFRelease(sampler);
        }
    }
    
    // Clear the sampler cache
    void Metal_ClearSamplerCache() {
        [MetalSamplerProvider.shared clearCache];
    }
} 