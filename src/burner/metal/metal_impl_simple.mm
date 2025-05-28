#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>

#include "metal_intf.h"

// Simple Metal implementation just for testing

// Function specifically for test_main.cpp
extern "C" bool InitMetalTest() {
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

// Basic Metal implementation
bool InitMetal(void* view) {
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

void ShutdownMetal() {
    NSLog(@"Metal shutdown");
}

void MetalRenderFrame(unsigned char* buffer, int width, int height, int pitch) {
    // In a real implementation, this would render the frame to the Metal view
    // For testing, we just log that we received a frame
    if (buffer && width && height && pitch) {
        NSLog(@"Rendering frame: %dx%d, pitch: %d", width, height, pitch);
    }
}

void MetalSetWindowTitle(const char* title) {
    if (title) {
        NSString* titleString = [NSString stringWithUTF8String:title];
        NSLog(@"Setting window title: %@", titleString);
    }
}

void MetalResizeWindow(int width, int height) {
    NSLog(@"Resizing window to %dx%d", width, height);
}

int RunMetalGame() {
    NSLog(@"FBNeo Metal Test - Press Ctrl+C to exit");
    NSLog(@"This is a test implementation that doesn't actually run the game");
    
    // For testing, we just return success immediately
    return 0;
}

// Input handling for FBNeo core
void MetalHandleInput(bool bCopy) {
    // In a real implementation, this would handle input from the user
} 