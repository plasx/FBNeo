#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <Cocoa/Cocoa.h>
#import "metal_bridge.h"

// Function to load a ROM file
bool Metal_LoadROM(const char* romPath) {
    if (!romPath || !romPath[0]) {
        NSLog(@"Error: No ROM path provided");
        return false;
    }
    
    NSLog(@"Attempting to load ROM: %s", romPath);
    
    // Convert C string to NSString
    NSString* romPathString = [NSString stringWithUTF8String:romPath];
    
    // Check if file exists
    NSFileManager* fileManager = [NSFileManager defaultManager];
    if (![fileManager fileExistsAtPath:romPathString]) {
        NSLog(@"Error: ROM file does not exist: %@", romPathString);
        return false;
    }
    
    // Get file extension
    NSString* fileExtension = [[romPathString pathExtension] lowercaseString];
    
    // Handle different file types
    if ([fileExtension isEqualToString:@"zip"]) {
        NSLog(@"Loading ZIP ROM: %@", romPathString);
        // ZIP file handling would go here
    } else {
        NSLog(@"Loading ROM: %@", romPathString);
        // Regular ROM file handling
    }
    
    // Create a dummy frame buffer for testing
    int width = 384;  // CPS2 standard width
    int height = 224; // CPS2 standard height
    int bpp = 4;      // RGBA
    
    // Create a buffer with test pattern
    unsigned char* buffer = (unsigned char*)malloc(width * height * bpp);
    if (!buffer) {
        NSLog(@"Error: Failed to allocate buffer for ROM");
        return false;
    }
    
    // Fill with a test pattern (gradient)
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int offset = (y * width + x) * bpp;
            buffer[offset + 0] = (unsigned char)(x * 255 / width);         // B
            buffer[offset + 1] = (unsigned char)(y * 255 / height);        // G
            buffer[offset + 2] = (unsigned char)((x + y) * 255 / (width + height)); // R
            buffer[offset + 3] = 255;                                      // A
        }
    }
    
    // Pass the buffer to the renderer
    Metal_SetFrameBuffer(buffer, width, height, width * bpp);
    
    // Buffer is now owned by the renderer, don't free it here
    
    // Set window title to show ROM name
    NSString* romName = [romPathString lastPathComponent];
    NSString* windowTitle = [NSString stringWithFormat:@"FBNeo Metal - %@", romName];
    Metal_SetWindowTitle([windowTitle UTF8String]);
    
    return true;
}

// Function to unload a ROM
void Metal_UnloadROM() {
    NSLog(@"Unloading ROM");
    
    // Reset frame buffer (cleanup will happen in Metal_Shutdown)
    Metal_SetFrameBuffer(NULL, 0, 0, 0);
    
    // Reset window title
    Metal_SetWindowTitle("FBNeo Metal Renderer");
} 