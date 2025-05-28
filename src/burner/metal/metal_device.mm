#import <Metal/Metal.h>

// Global Metal device reference accessible throughout the application
id<MTLDevice> g_metalDevice = nil;

// Set up the Metal device
bool InitMetalDevice() {
    // Get the default system device
    g_metalDevice = MTLCreateSystemDefaultDevice();
    
    if (!g_metalDevice) {
        NSLog(@"Metal is not supported on this device");
        return false;
    }
    
    NSLog(@"Metal device initialized: %@", g_metalDevice.name);
    return true;
}

// Clean up Metal device
void ShutdownMetalDevice() {
    g_metalDevice = nil;
} 