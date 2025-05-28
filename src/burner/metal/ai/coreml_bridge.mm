#import <Foundation/Foundation.h>
#import <CoreML/CoreML.h>
#import <Vision/Vision.h>
#include <cstdio>

#include "ai_definitions.h"
#include "ai_input_frame.h"
#include "ai_output_action.h"
#include "../metal_bridge.h"

// MARK: - CoreML Model Wrapper Implementation

/**
 * CoreML model wrapper that handles loading and inference
 */
@interface FBNeoCoreMLModel : NSObject

@property (nonatomic, strong) MLModel *model;
@property (nonatomic, strong) VNCoreMLModel *visionModel;
@property (nonatomic, assign) CGSize inputSize;
@property (nonatomic, assign) int outputSize;
@property (nonatomic, copy) NSString *modelName;
@property (nonatomic, strong) NSDictionary *modelInfo;

- (instancetype)initWithPath:(NSString *)path;
- (BOOL)processImage:(uint8_t *)imageData width:(int)width height:(int)height pitch:(int)pitch results:(float *)results count:(int)count;
- (void)getModelInfo:(fbneo::ai::AIModelInfo*)info;

@end

@implementation FBNeoCoreMLModel

- (instancetype)initWithPath:(NSString *)path {
    self = [super init];
    if (self) {
        NSError *error = nil;
        NSURL *modelURL = [NSURL fileURLWithPath:path];
        
        // Try to load the model
        _model = [MLModel modelWithContentsOfURL:modelURL error:&error];
        if (error || !_model) {
            NSLog(@"Failed to load CoreML model: %@", error.localizedDescription);
            return nil;
        }
        
        // Extract model name
        _modelName = [[path lastPathComponent] stringByDeletingPathExtension];
        
        // Create Vision model for image processing
        _visionModel = [VNCoreMLModel modelForMLModel:_model error:&error];
        if (error) {
            NSLog(@"Warning: Could not create Vision model: %@", error.localizedDescription);
            // Not fatal, we can still use direct MLModel API
        }
        
        // Get input and output parameters
        MLModelDescription *modelDescription = _model.modelDescription;
        NSDictionary *inputDescriptions = modelDescription.inputDescriptionsByName;
        NSDictionary *outputDescriptions = modelDescription.outputDescriptionsByName;
        
        // Set default input size
        _inputSize = CGSizeMake(224, 224);
        
        // Find image input
        for (NSString *inputName in inputDescriptions) {
            MLFeatureDescription *desc = inputDescriptions[inputName];
            if (desc.type == MLFeatureTypeImage) {
                // Use constraint if available
                CGImagePropertyOrientation orientation = kCGImagePropertyOrientationUp;
                _inputSize = CGSizeMake(224, 224);
                break;
            } else if (desc.type == MLFeatureTypeMultiArray) {
                // Check if this is a multi-array that could represent an image
                NSArray<NSNumber *> *shape = [desc valueForKey:@"multiArrayConstraint"];
                if (shape && shape.count >= 3) {
                    // Assuming CHW format
                    double channels = [shape[0] doubleValue];
                    double height = [shape[1] doubleValue];
                    double width = [shape[2] doubleValue];
                    _inputSize = CGSizeMake(width, height);
                    break;
                }
            }
        }
        
        // Find output size
        for (NSString *outputName in outputDescriptions) {
            MLFeatureDescription *desc = outputDescriptions[outputName];
            if (desc.type == MLFeatureTypeMultiArray) {
                NSArray<NSNumber *> *shape = [desc valueForKey:@"multiArrayConstraint"];
                if (shape && shape.count >= 1) {
                    _outputSize = [shape[0] intValue];
                }
            }
        }
        
        // Store model metadata for later retrieval
        _modelInfo = @{
            @"name": _modelName ?: @"unknown",
            @"input_width": @((int)_inputSize.width),
            @"input_height": @((int)_inputSize.height),
            @"output_size": @(_outputSize),
            @"description": _model.modelDescription.metadata[@"description"] ?: @"",
            @"author": _model.modelDescription.metadata[@"author"] ?: @"",
            @"version": _model.modelDescription.metadata[@"versionString"] ?: @"1.0",
            @"license": _model.modelDescription.metadata[@"license"] ?: @""
        };
        
        NSLog(@"CoreML model loaded: %@", _modelName);
        NSLog(@"Input size: %.0f x %.0f", _inputSize.width, _inputSize.height);
        NSLog(@"Output size: %d", _outputSize);
    }
    return self;
}

- (BOOL)processImage:(uint8_t *)imageData width:(int)width height:(int)height pitch:(int)pitch results:(float *)results count:(int)count {
    if (!_model || !imageData || !results) {
        return NO;
    }
    
    @autoreleasepool {
        NSError *error = nil;
        
        // Create a CIImage from the raw pixel data
        size_t bytesPerRow = pitch;
        CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
        CGContextRef context = CGBitmapContextCreate(imageData, width, height, 8, bytesPerRow,
                                                   colorSpace, kCGImageAlphaPremultipliedLast);
        CGImageRef cgImage = CGBitmapContextCreateImage(context);
        CIImage *ciImage = [CIImage imageWithCGImage:cgImage];
        
        // If we have a Vision model, use it
        if (_visionModel) {
            __block BOOL completionCalled = NO;
            __block NSCondition *condition = [[NSCondition alloc] init];
            
            VNCoreMLRequest *request = [[VNCoreMLRequest alloc] initWithModel:_visionModel completionHandler:^(VNRequest * _Nonnull request, NSError * _Nullable error) {
                [condition lock];
                
                if (error) {
                    NSLog(@"Vision request failed: %@", error.localizedDescription);
                } else {
                    // Process results
                    VNCoreMLFeatureValueObservation *observation = (VNCoreMLFeatureValueObservation *)request.results.firstObject;
                    MLFeatureValue *featureValue = observation.featureValue;
                    if (featureValue.type == MLFeatureTypeMultiArray) {
                        MLMultiArray *multiArray = featureValue.multiArrayValue;
                        
                        // Copy output to results array
                        int outSize = MIN(count, (int)multiArray.count);
                        for (int i = 0; i < outSize; i++) {
                            results[i] = [multiArray[i] floatValue];
                        }
                    }
                }
                
                completionCalled = YES;
                [condition signal];
                [condition unlock];
            }];
            
            // Configure the request
            request.imageCropAndScaleOption = VNImageCropAndScaleOptionScaleFit;
            
            // Create a handler for processing the image
            VNImageRequestHandler *handler = [[VNImageRequestHandler alloc] initWithCIImage:ciImage options:@{}];
            
            // Perform the request
            [handler performRequests:@[request] error:&error];
            if (error) {
                NSLog(@"Failed to perform Vision request: %@", error.localizedDescription);
                CGImageRelease(cgImage);
                CGContextRelease(context);
                CGColorSpaceRelease(colorSpace);
                return NO;
            }
            
            // Wait for completion (with a reasonable timeout)
            [condition lock];
            if (!completionCalled) {
                [condition waitUntilDate:[NSDate dateWithTimeIntervalSinceNow:1.0]];
            }
            [condition unlock];
        } else {
            // Fallback to using MLModel directly for non-Vision models
            // This is more complex and would require preprocessing the image
            // to match the expected input format of the model
            NSLog(@"Warning: Vision model not available, using direct MLModel API");
            
            // Create a pixel buffer from the image
            CVPixelBufferRef pixelBuffer = NULL;
            CFDictionaryRef options = (__bridge CFDictionaryRef)@{
                (id)kCVPixelBufferCGImageCompatibilityKey: @YES,
                (id)kCVPixelBufferCGBitmapContextCompatibilityKey: @YES
            };
            
            CVReturn status = CVPixelBufferCreate(kCFAllocatorDefault, width, height, 
                                                kCVPixelFormatType_32BGRA, 
                                                options, &pixelBuffer);
            
            if (status != kCVReturnSuccess) {
                NSLog(@"Failed to create CVPixelBuffer: %d", status);
                CGImageRelease(cgImage);
                CGContextRelease(context);
                CGColorSpaceRelease(colorSpace);
                return NO;
            }
            
            // Lock the buffer and copy the frame data
            CVPixelBufferLockBaseAddress(pixelBuffer, 0);
            void *pixelData = CVPixelBufferGetBaseAddress(pixelBuffer);
            size_t dataSize = CVPixelBufferGetDataSize(pixelBuffer);
            size_t bytesPerRowBuf = CVPixelBufferGetBytesPerRow(pixelBuffer);
            
            // Copy data row by row
            for (int y = 0; y < height; y++) {
                memcpy((char*)pixelData + y * bytesPerRowBuf, 
                      imageData + y * pitch, 
                      MIN(width * 4, bytesPerRowBuf));
            }
            
            CVPixelBufferUnlockBaseAddress(pixelBuffer, 0);
            
            // Create feature value and input dictionary
            MLFeatureValue *inputValue = [MLFeatureValue featureValueWithPixelBuffer:pixelBuffer];
            NSString *inputName = _model.modelDescription.inputDescriptionsByName.allKeys.firstObject;
            
            if (!inputName) {
                NSLog(@"Error: No inputs found in model");
                CVPixelBufferRelease(pixelBuffer);
                CGImageRelease(cgImage);
                CGContextRelease(context);
                CGColorSpaceRelease(colorSpace);
                return NO;
            }
            
            MLDictionaryFeatureProvider *inputFeatures = [[MLDictionaryFeatureProvider alloc] 
                                                         initWithDictionary:@{inputName: inputValue}];
            
            // Make prediction
            id<MLFeatureProvider> outputFeatures = [_model predictionFromFeatures:inputFeatures 
                                                                           error:&error];
            
            if (error) {
                NSLog(@"Prediction error: %@", error.localizedDescription);
                CVPixelBufferRelease(pixelBuffer);
                CGImageRelease(cgImage);
                CGContextRelease(context);
                CGColorSpaceRelease(colorSpace);
                return NO;
            }
            
            // Get output and copy to results buffer
            NSString *outputName = _model.modelDescription.outputDescriptionsByName.allKeys.firstObject;
            MLFeatureValue *outputValue = [outputFeatures featureValueForName:outputName];
            
            if (outputValue && outputValue.type == MLFeatureTypeMultiArray) {
                MLMultiArray *multiArray = outputValue.multiArrayValue;
                int outSize = MIN(count, (int)multiArray.count);
                
                for (int i = 0; i < outSize; i++) {
                    results[i] = [multiArray[i] floatValue];
                }
            }
            
            CVPixelBufferRelease(pixelBuffer);
        }
        
        // Clean up resources
        CGImageRelease(cgImage);
        CGContextRelease(context);
        CGColorSpaceRelease(colorSpace);
        
        return YES;
    }
}

- (void)getModelInfo:(fbneo::ai::AIModelInfo*)info {
    if (!info) return;
    
    // Clear the struct
    memset(info, 0, sizeof(fbneo::ai::AIModelInfo));
    
    // Copy model name
    strncpy(info->name, [_modelName UTF8String], sizeof(info->name) - 1);
    
    // Copy version
    NSString *version = _modelInfo[@"version"] ?: @"1.0";
    strncpy(info->version, [version UTF8String], sizeof(info->version) - 1);
    
    // Set dimensions
    info->input_width = (int)_inputSize.width;
    info->input_height = (int)_inputSize.height;
    info->input_channels = 3; // Assume RGB
    
    // Set action count
    info->action_count = _outputSize;
    
    // Set model type and compute backend
    info->model_type = fbneo::ai::FBNEO_AI_MODEL_TYPE_COREML;
    
    // Set compute backend based on device capabilities
    if (@available(macOS 11.0, *)) {
        info->compute_backend = fbneo::ai::FBNEO_AI_COMPUTE_ALL; // Use all available (CPU+GPU+ANE)
    } else {
        info->compute_backend = fbneo::ai::FBNEO_AI_COMPUTE_CPU_GPU; // Use CPU+GPU
    }
    
    // Set precision
    info->precision = fbneo::ai::FBNEO_AI_PRECISION_FP16; // Most models use FP16
    
    // Set features
    info->features = fbneo::ai::FBNEO_AI_FEATURE_PLAYER_ASSIST;
    
    // Set game_id if available
    NSString *gameId = _modelInfo[@"game_id"] ?: @"";
    strncpy(info->game_id, [gameId UTF8String], sizeof(info->game_id) - 1);
    
    // Set genre if available
    info->game_genre = fbneo::ai::FBNEO_AI_GENRE_FIGHTING; // Default to fighting games
}

@end

// MARK: - Global CoreML Manager

/**
 * Singleton manager for CoreML functionality
 */
@interface FBNeoCoreMLManager : NSObject

@property (nonatomic, strong) FBNeoCoreMLModel *currentModel;
@property (nonatomic, assign) BOOL initialized;

+ (instancetype)sharedManager;
- (BOOL)initialize;
- (void)shutdown;
- (BOOL)loadModel:(NSString *)path;
- (BOOL)getModelInfo:(fbneo::ai::AIModelInfo *)info;
- (BOOL)processFrame:(const void *)frameData width:(int)width height:(int)height pitch:(int)pitch results:(float *)results resultCount:(int)resultCount;

@end

@implementation FBNeoCoreMLManager

+ (instancetype)sharedManager {
    static FBNeoCoreMLManager *sharedManager = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        sharedManager = [[self alloc] init];
    });
    return sharedManager;
}

- (instancetype)init {
    self = [super init];
    if (self) {
        _initialized = NO;
        _currentModel = nil;
    }
    return self;
}

- (BOOL)initialize {
    if (_initialized) {
        return YES; // Already initialized
    }
    
    NSLog(@"Initializing CoreML framework");
    
    _initialized = YES;
    return YES;
}

- (void)shutdown {
    NSLog(@"Shutting down CoreML framework");
    
    _currentModel = nil;
    _initialized = NO;
}

- (BOOL)loadModel:(NSString *)path {
    if (!_initialized) {
        if (![self initialize]) {
            return NO;
        }
    }
    
    NSLog(@"Loading CoreML model: %@", path);
    
    _currentModel = [[FBNeoCoreMLModel alloc] initWithPath:path];
    return (_currentModel != nil);
}

- (BOOL)getModelInfo:(fbneo::ai::AIModelInfo *)info {
    if (!_initialized || !_currentModel) {
        return NO;
    }
    
    [_currentModel getModelInfo:info];
    return YES;
}

- (BOOL)processFrame:(const void *)frameData width:(int)width height:(int)height pitch:(int)pitch results:(float *)results resultCount:(int)resultCount {
    if (!_initialized || !_currentModel) {
        return NO;
    }
    
    return [_currentModel processImage:(uint8_t *)frameData width:width height:height pitch:pitch results:results count:resultCount];
}

@end

// MARK: - C Interface Functions
// These functions are exported with C linkage for use from C/C++ code

#ifdef __cplusplus
extern "C" {
#endif

// Initialize the CoreML framework
bool CoreML_Initialize() {
    @autoreleasepool {
        return [[FBNeoCoreMLManager sharedManager] initialize];
    }
}

// Shutdown the CoreML framework
void CoreML_Shutdown() {
    @autoreleasepool {
        [[FBNeoCoreMLManager sharedManager] shutdown];
    }
}

// Load a CoreML model
bool CoreML_LoadModel(const char* path) {
    @autoreleasepool {
        if (!path) return false;
        NSString *modelPath = @(path);
        return [[FBNeoCoreMLManager sharedManager] loadModel:modelPath];
    }
}

// Get information about the loaded model
bool CoreML_GetModelInfo(fbneo::ai::AIModelInfo* info) {
    @autoreleasepool {
        if (!info) return false;
        return [[FBNeoCoreMLManager sharedManager] getModelInfo:info];
    }
}

// Process a frame using the CoreML model
bool CoreML_ProcessFrame(const void* frameData, int width, int height, int pitch, float* results, int resultSize) {
    @autoreleasepool {
        if (!frameData || !results || width <= 0 || height <= 0 || pitch <= 0 || resultSize <= 0) {
            return false;
        }
        
        return [[FBNeoCoreMLManager sharedManager] processFrame:frameData 
                                                      width:width 
                                                     height:height 
                                                      pitch:pitch 
                                                    results:results 
                                                resultCount:resultSize];
    }
}

// Render visualization overlays (stub for now)
bool CoreML_RenderVisualization(void* overlayData, int width, int height, int pitch, int visualizationType) {
    // This would be implemented to render visualizations for debugging
    // Not implemented in this minimal version
    return false;
}

#ifdef __cplusplus
}
#endif 