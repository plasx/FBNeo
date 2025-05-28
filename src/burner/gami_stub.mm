#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>

// This is a minimal stub implementation for the Metal interface
// In a real implementation, this would contain proper Metal code

// Declare a C function that can be called from test_main.cpp
__attribute__((visibility("default")))
extern "C" {
    bool InitMetalTest() {
        @autoreleasepool {
            // Check if Metal is supported on this device
            id<MTLDevice> device = MTLCreateSystemDefaultDevice();
            if (!device) {
                NSLog(@"Metal is not supported on this device");
                return false;
            }
            
            NSLog(@"Metal is supported on this device");
            NSLog(@"Device name: %@", device.name);
            return true;
        }
    }
}

@interface MetalRenderer : NSObject
@property (nonatomic, strong) id<MTLDevice> device;
@property (nonatomic, strong) id<MTLCommandQueue> commandQueue;
@end

@implementation MetalRenderer

- (instancetype)init {
    self = [super init];
    if (self) {
        _device = MTLCreateSystemDefaultDevice();
        if (_device) {
            _commandQueue = [_device newCommandQueue];
            NSLog(@"Metal renderer initialized successfully");
        } else {
            NSLog(@"Failed to create Metal device");
        }
    }
    return self;
}

@end 