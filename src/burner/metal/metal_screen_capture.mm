#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <Cocoa/Cocoa.h>
#import <Foundation/Foundation.h>

#include <string>
#include <chrono>
#include <ctime>

// Screen capture class
@interface MetalScreenCapture : NSObject

// Properties
@property (nonatomic, strong) id<MTLDevice> device;
@property (nonatomic, strong) id<MTLCommandQueue> commandQueue;
@property (nonatomic, strong) id<MTLTexture> sourceTexture;
@property (nonatomic, assign) NSString* captureDirectory;
@property (nonatomic, assign) bool captureRequested;
@property (nonatomic, assign) bool videoRecording;
@property (nonatomic, assign) int frameCount;
@property (nonatomic, assign) NSTimeInterval recordingStartTime;

// Initialization
- (instancetype)initWithDevice:(id<MTLDevice>)device;

// Set source texture
- (void)setSourceTexture:(id<MTLTexture>)texture;

// Set capture directory
- (void)setCaptureDirectory:(NSString*)directory;

// Capture current frame
- (bool)captureScreenshot;

// Start video recording
- (bool)startRecording;

// Stop video recording
- (bool)stopRecording;

// Update for each frame
- (void)update;

@end

@implementation MetalScreenCapture

- (instancetype)initWithDevice:(id<MTLDevice>)device {
    self = [super init];
    if (self) {
        _device = device;
        _commandQueue = [device newCommandQueue];
        _captureDirectory = [@"~/Documents/FBNeo/Captures" stringByExpandingTildeInPath];
        _captureRequested = false;
        _videoRecording = false;
        _frameCount = 0;
        
        // Create capture directory if it doesn't exist
        NSFileManager* fileManager = [NSFileManager defaultManager];
        if (![fileManager fileExistsAtPath:_captureDirectory]) {
            NSError* error = nil;
            [fileManager createDirectoryAtPath:_captureDirectory
                  withIntermediateDirectories:YES
                                   attributes:nil
                                        error:&error];
            
            if (error) {
                NSLog(@"Failed to create capture directory: %@", error);
            }
        }
    }
    return self;
}

- (void)setSourceTexture:(id<MTLTexture>)texture {
    _sourceTexture = texture;
}

- (void)setCaptureDirectory:(NSString*)directory {
    _captureDirectory = [directory copy];
    
    // Create directory if it doesn't exist
    NSFileManager* fileManager = [NSFileManager defaultManager];
    if (![fileManager fileExistsAtPath:_captureDirectory]) {
        NSError* error = nil;
        [fileManager createDirectoryAtPath:_captureDirectory
              withIntermediateDirectories:YES
                               attributes:nil
                                    error:&error];
        
        if (error) {
            NSLog(@"Failed to create capture directory: %@", error);
        }
    }
}

- (bool)captureScreenshot {
    if (!_sourceTexture) {
        NSLog(@"No source texture available for capture");
        return false;
    }
    
    _captureRequested = true;
    return true;
}

- (bool)startRecording {
    if (_videoRecording) {
        NSLog(@"Already recording");
        return false;
    }
    
    if (!_sourceTexture) {
        NSLog(@"No source texture available for recording");
        return false;
    }
    
    _videoRecording = true;
    _frameCount = 0;
    _recordingStartTime = [NSDate timeIntervalSinceReferenceDate];
    
    // Create a subdirectory for this recording
    NSDateFormatter* formatter = [[NSDateFormatter alloc] init];
    [formatter setDateFormat:@"yyyy-MM-dd_HH-mm-ss"];
    NSString* timestamp = [formatter stringFromDate:[NSDate date]];
    NSString* recordingDir = [_captureDirectory stringByAppendingPathComponent:
                             [NSString stringWithFormat:@"Recording_%@", timestamp]];
    
    NSFileManager* fileManager = [NSFileManager defaultManager];
    if (![fileManager fileExistsAtPath:recordingDir]) {
        NSError* error = nil;
        [fileManager createDirectoryAtPath:recordingDir
              withIntermediateDirectories:YES
                               attributes:nil
                                    error:&error];
        
        if (error) {
            NSLog(@"Failed to create recording directory: %@", error);
            _videoRecording = false;
            return false;
        }
    }
    
    _captureDirectory = recordingDir;
    
    NSLog(@"Started video recording to %@", recordingDir);
    return true;
}

- (bool)stopRecording {
    if (!_videoRecording) {
        return false;
    }
    
    _videoRecording = false;
    
    NSLog(@"Stopped video recording. Captured %d frames.", _frameCount);
    return true;
}

- (void)update {
    if (_captureRequested || _videoRecording) {
        [self captureCurrentFrame];
    }
}

- (void)captureCurrentFrame {
    if (!_sourceTexture) {
        _captureRequested = false;
        return;
    }
    
    @autoreleasepool {
        // Create a command buffer
        id<MTLCommandBuffer> commandBuffer = [_commandQueue commandBuffer];
        
        // Create a temporary texture to receive the data
        MTLTextureDescriptor* textureDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:_sourceTexture.pixelFormat
                                                                                              width:_sourceTexture.width
                                                                                             height:_sourceTexture.height
                                                                                          mipmapped:NO];
        textureDesc.usage = MTLTextureUsageShaderRead | MTLTextureUsageShaderWrite;
        
        id<MTLTexture> tempTexture = [_device newTextureWithDescriptor:textureDesc];
        
        // Copy the source texture content
        id<MTLBlitCommandEncoder> blitEncoder = [commandBuffer blitCommandEncoder];
        [blitEncoder copyFromTexture:_sourceTexture
                        sourceSlice:0
                        sourceLevel:0
                       sourceOrigin:MTLOriginMake(0, 0, 0)
                         sourceSize:MTLSizeMake(_sourceTexture.width, _sourceTexture.height, 1)
                          toTexture:tempTexture
                   destinationSlice:0
                   destinationLevel:0
                  destinationOrigin:MTLOriginMake(0, 0, 0)];
        [blitEncoder endEncoding];
        
        // Generate the file name
        NSString* fileName;
        if (_videoRecording) {
            fileName = [NSString stringWithFormat:@"frame_%06d.png", _frameCount++];
        } else {
            NSDateFormatter* formatter = [[NSDateFormatter alloc] init];
            [formatter setDateFormat:@"yyyy-MM-dd_HH-mm-ss"];
            NSString* timestamp = [formatter stringFromDate:[NSDate date]];
            fileName = [NSString stringWithFormat:@"screenshot_%@.png", timestamp];
        }
        
        NSString* filePath = [_captureDirectory stringByAppendingPathComponent:fileName];
        
        // Wait for the command buffer to complete before saving the image
        __block bool finished = false;
        [commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> buffer) {
            // Get the texture contents
            NSUInteger bytesPerRow = 4 * tempTexture.width;
            NSUInteger byteCount = bytesPerRow * tempTexture.height;
            void* imageBytes = malloc(byteCount);
            
            MTLRegion region = MTLRegionMake2D(0, 0, tempTexture.width, tempTexture.height);
            [tempTexture getBytes:imageBytes bytesPerRow:bytesPerRow fromRegion:region mipmapLevel:0];
            
            // Create a CGImage
            CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
            CGContextRef context = CGBitmapContextCreate(imageBytes,
                                                        tempTexture.width,
                                                        tempTexture.height,
                                                        8,
                                                        bytesPerRow,
                                                        colorSpace,
                                                        kCGImageAlphaPremultipliedLast);
            
            CGImageRef cgImage = CGBitmapContextCreateImage(context);
            
            // Create an NSImage
            NSImage* image = [[NSImage alloc] initWithCGImage:cgImage size:NSMakeSize(tempTexture.width, tempTexture.height)];
            
            // Save the image as PNG
            NSData* pngData = [image TIFFRepresentation];
            NSBitmapImageRep* imageRep = [NSBitmapImageRep imageRepWithData:pngData];
            NSDictionary* properties = @{NSImageCompressionFactor: @(1.0)};
            NSData* fileData = [imageRep representationUsingType:NSBitmapImageFileTypePNG properties:properties];
            
            [fileData writeToFile:filePath atomically:YES];
            
            // Clean up
            CGImageRelease(cgImage);
            CGContextRelease(context);
            CGColorSpaceRelease(colorSpace);
            free(imageBytes);
            
            finished = true;
            
            if (!self->_videoRecording) {
                NSLog(@"Screenshot saved to: %@", filePath);
                self->_captureRequested = false;
            }
        }];
        
        [commandBuffer commit];
    }
}

@end

// C interface for the screen capture
extern "C" {
    // Create a screen capture
    void* Metal_CreateScreenCapture(void* device) {
        id<MTLDevice> mtlDevice = (__bridge id<MTLDevice>)device;
        
        MetalScreenCapture* capture = [[MetalScreenCapture alloc] initWithDevice:mtlDevice];
        return (__bridge_retained void*)capture;
    }
    
    // Set the source texture
    void Metal_ScreenCaptureSetTexture(void* capture, void* texture) {
        if (!capture || !texture) return;
        
        MetalScreenCapture* screenCapture = (__bridge MetalScreenCapture*)capture;
        id<MTLTexture> mtlTexture = (__bridge id<MTLTexture>)texture;
        
        [screenCapture setSourceTexture:mtlTexture];
    }
    
    // Set the capture directory
    void Metal_ScreenCaptureSetDirectory(void* capture, const char* directory) {
        if (!capture || !directory) return;
        
        MetalScreenCapture* screenCapture = (__bridge MetalScreenCapture*)capture;
        NSString* dirPath = [NSString stringWithUTF8String:directory];
        
        [screenCapture setCaptureDirectory:dirPath];
    }
    
    // Capture a screenshot
    bool Metal_ScreenCaptureScreenshot(void* capture) {
        if (!capture) return false;
        
        MetalScreenCapture* screenCapture = (__bridge MetalScreenCapture*)capture;
        return [screenCapture captureScreenshot];
    }
    
    // Start video recording
    bool Metal_ScreenCaptureStartRecording(void* capture) {
        if (!capture) return false;
        
        MetalScreenCapture* screenCapture = (__bridge MetalScreenCapture*)capture;
        return [screenCapture startRecording];
    }
    
    // Stop video recording
    bool Metal_ScreenCaptureStopRecording(void* capture) {
        if (!capture) return false;
        
        MetalScreenCapture* screenCapture = (__bridge MetalScreenCapture*)capture;
        return [screenCapture stopRecording];
    }
    
    // Update screen capture (call every frame)
    void Metal_ScreenCaptureUpdate(void* capture) {
        if (!capture) return;
        
        MetalScreenCapture* screenCapture = (__bridge MetalScreenCapture*)capture;
        [screenCapture update];
    }
    
    // Destroy screen capture
    void Metal_DestroyScreenCapture(void* capture) {
        if (capture) {
            CFRelease(capture);
        }
    }
} 