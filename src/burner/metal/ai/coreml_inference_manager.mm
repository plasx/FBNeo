#import <Foundation/Foundation.h>
#import <CoreML/CoreML.h>
#import <Vision/Vision.h>
#import <Metal/Metal.h>

#include <string>
#include <vector>
#include <deque>
#include <chrono>
#include "ai_definitions.h"
#include "coreml_integration.h"

// CoreML Inference Manager for high-performance AI inference
@interface FBNeoCoreMLInferenceManager : NSObject

// Properties
@property (nonatomic, strong) MLModel* model;
@property (nonatomic, strong) VNCoreMLModel* visionModel;
@property (nonatomic, strong) NSString* modelPath;
@property (nonatomic, strong) MLModelConfiguration* configuration;
@property (nonatomic, assign) BOOL isModelLoaded;
@property (nonatomic, assign) int inferenceCounter;
@property (nonatomic, assign) float lastInferenceTimeMs;
@property (nonatomic, assign) float avgInferenceTimeMs;
@property (nonatomic, assign) int maxBatchSize;
@property (nonatomic, assign) BOOL useVision;
@property (nonatomic, assign) BOOL useCache;
@property (nonatomic, strong) NSCache* predictionCache;
@property (nonatomic, strong) NSString* lastErrorMessage;

// Initialize with model path
- (instancetype)initWithModelPath:(NSString*)path configuration:(MLModelConfiguration*)config;

// Load the model
- (BOOL)loadModel:(NSError**)error;

// Run inference with a frame buffer (game screen)
- (NSDictionary*)inferWithFrameBuffer:(void*)buffer 
                               width:(int)width 
                              height:(int)height
                               error:(NSError**)error;

// Run inference with pre-processed features
- (NSDictionary*)inferWithFeatures:(NSDictionary*)features error:(NSError**)error;

// Run batch inference
- (NSArray*)batchInferWithFrameBuffers:(NSArray*)buffers error:(NSError**)error;

// Set the cache size
- (void)setCacheSize:(int)maxItems;

// Get performance stats
- (NSDictionary*)getPerformanceStats;

// Memory management
- (void)releaseResources;

@end

@implementation FBNeoCoreMLInferenceManager

- (instancetype)initWithModelPath:(NSString*)path configuration:(MLModelConfiguration*)config {
    self = [super init];
    if (self) {
        _modelPath = path;
        _configuration = config ?: [[MLModelConfiguration alloc] init];
        _isModelLoaded = NO;
        _inferenceCounter = 0;
        _lastInferenceTimeMs = 0.0f;
        _avgInferenceTimeMs = 0.0f;
        _maxBatchSize = 1;
        _useVision = NO;
        _useCache = NO;
        _predictionCache = [[NSCache alloc] init];
        _predictionCache.countLimit = 100; // Default cache size
        _lastErrorMessage = @"";
        
        // Set default configuration
        if (!config) {
            _configuration.computeUnits = MLComputeUnitsAll;
        }
    }
    return self;
}

- (void)dealloc {
    [self releaseResources];
    [_model release];
    [_visionModel release];
    [_modelPath release];
    [_configuration release];
    [_predictionCache release];
    [_lastErrorMessage release];
    [super dealloc];
}

- (BOOL)loadModel:(NSError**)error {
    // Check if model path exists
    if (![[NSFileManager defaultManager] fileExistsAtPath:_modelPath]) {
        if (error) {
            *error = [NSError errorWithDomain:@"com.fbneo.coreml" 
                                         code:404 
                                     userInfo:@{NSLocalizedDescriptionKey: @"Model file not found"}];
        }
        _lastErrorMessage = @"Model file not found";
        return NO;
    }
    
    // Load the model
    NSURL* modelURL = [NSURL fileURLWithPath:_modelPath];
    _model = [[MLModel modelWithContentsOfURL:modelURL configuration:_configuration error:error] retain];
    
    if (!_model) {
        _lastErrorMessage = [NSString stringWithFormat:@"Failed to load model: %@", 
                            *error ? [*error localizedDescription] : @"Unknown error"];
        return NO;
    }
    
    // Try to create a Vision model for image input
    NSError* visionError = nil;
    _visionModel = [[VNCoreMLModel modelForMLModel:_model error:&visionError] retain];
    
    if (!visionError && _visionModel) {
        _useVision = YES;
        NSLog(@"Using Vision API for inference");
    } else {
        _useVision = NO;
        NSLog(@"Vision API not available for this model, using direct CoreML inference");
    }
    
    // Get max batch size
    if ([_model respondsToSelector:@selector(maximumBatchSize)]) {
        _maxBatchSize = [_model maximumBatchSize];
    } else {
        _maxBatchSize = 1;
    }
    
    NSLog(@"Model loaded successfully. Max batch size: %d", _maxBatchSize);
    _isModelLoaded = YES;
    return YES;
}

- (NSDictionary*)inferWithFrameBuffer:(void*)buffer 
                              width:(int)width 
                             height:(int)height
                              error:(NSError**)error {
    if (!_isModelLoaded) {
        if (error) {
            *error = [NSError errorWithDomain:@"com.fbneo.coreml" 
                                         code:400 
                                     userInfo:@{NSLocalizedDescriptionKey: @"Model not loaded"}];
        }
        _lastErrorMessage = @"Model not loaded";
        return nil;
    }
    
    // Start timing the inference
    NSDate* startTime = [NSDate date];
    
    // Create a cache key if caching is enabled
    NSString* cacheKey = nil;
    if (_useCache) {
        // Simple hash-based cache key
        NSUInteger hash = 0;
        const uint8_t* bytes = (const uint8_t*)buffer;
        for (int i = 0; i < width * height * 4; i += 16) {
            hash = hash * 31 + bytes[i];
        }
        cacheKey = [NSString stringWithFormat:@"%lu", (unsigned long)hash];
        
        // Check cache
        NSDictionary* cachedResult = [_predictionCache objectForKey:cacheKey];
        if (cachedResult) {
            return cachedResult;
        }
    }
    
    // Create CVPixelBuffer from the frame buffer
    CVPixelBufferRef pixelBuffer = NULL;
    CVReturn status = CVPixelBufferCreate(kCFAllocatorDefault,
                                         width, height,
                                         kCVPixelFormatType_32BGRA,
                                         NULL, &pixelBuffer);
    
    if (status != kCVReturnSuccess) {
        if (error) {
            *error = [NSError errorWithDomain:@"com.fbneo.coreml" 
                                         code:500 
                                     userInfo:@{NSLocalizedDescriptionKey: @"Failed to create pixel buffer"}];
        }
        _lastErrorMessage = @"Failed to create pixel buffer";
        return nil;
    }
    
    // Copy frame data to pixel buffer
    CVPixelBufferLockBaseAddress(pixelBuffer, 0);
    void* pixelData = CVPixelBufferGetBaseAddress(pixelBuffer);
    size_t bytesPerRow = CVPixelBufferGetBytesPerRow(pixelBuffer);
    
    // Copy data row by row
    uint8_t* srcRow = (uint8_t*)buffer;
    uint8_t* dstRow = (uint8_t*)pixelData;
    for (int y = 0; y < height; y++) {
        memcpy(dstRow, srcRow, width * 4);
        srcRow += width * 4;
        dstRow += bytesPerRow;
    }
    
    CVPixelBufferUnlockBaseAddress(pixelBuffer, 0);
    
    // Run inference
    NSDictionary* result = nil;
    
    if (_useVision) {
        // Use Vision API for image processing
        result = [self runVisionInference:pixelBuffer error:error];
    } else {
        // Use direct CoreML inference
        result = [self runCoreMLInference:pixelBuffer error:error];
    }
    
    // Release the pixel buffer
    CVPixelBufferRelease(pixelBuffer);
    
    // Calculate inference time
    NSTimeInterval inferenceTime = [[NSDate date] timeIntervalSinceDate:startTime] * 1000.0; // ms
    _lastInferenceTimeMs = inferenceTime;
    
    // Update average inference time
    if (_inferenceCounter == 0) {
        _avgInferenceTimeMs = inferenceTime;
    } else {
        // Exponential moving average with alpha=0.1
        _avgInferenceTimeMs = 0.9 * _avgInferenceTimeMs + 0.1 * inferenceTime;
    }
    
    // Increment counter
    _inferenceCounter++;
    
    // Cache the result if caching is enabled
    if (_useCache && cacheKey && result) {
        [_predictionCache setObject:result forKey:cacheKey];
    }
    
    return result;
}

- (NSDictionary*)runVisionInference:(CVPixelBufferRef)pixelBuffer error:(NSError**)error {
    // Create a Vision request handler
    VNImageRequestHandler* handler = [[VNImageRequestHandler alloc] 
                                     initWithCVPixelBuffer:pixelBuffer 
                                                  options:@{}];
    
    // Create a CoreML request
    __block NSMutableDictionary* results = [NSMutableDictionary dictionary];
    __block NSError* requestError = nil;
    
    VNCoreMLRequest* request = [[VNCoreMLRequest alloc] 
                               initWithModel:_visionModel 
                               completionHandler:^(VNRequest* request, NSError* error) {
        if (error) {
            requestError = error;
            return;
        }
        
        // Extract results
        for (VNObservation* observation in request.results) {
            if ([observation isKindOfClass:[VNCoreMLFeatureValueObservation class]]) {
                VNCoreMLFeatureValueObservation* featureObs = (VNCoreMLFeatureValueObservation*)observation;
                results[featureObs.featureName] = featureObs.featureValue;
            }
        }
    }];
    
    // Set image crop and scale option
    request.imageCropAndScaleOption = VNImageCropAndScaleOptionCenterCrop;
    
    // Perform the request
    [handler performRequests:@[request] error:&requestError];
    
    [handler release];
    [request release];
    
    // Handle any errors
    if (requestError) {
        if (error) {
            *error = requestError;
        }
        _lastErrorMessage = [NSString stringWithFormat:@"Vision inference failed: %@", 
                            [requestError localizedDescription]];
        return nil;
    }
    
    return results;
}

- (NSDictionary*)runCoreMLInference:(CVPixelBufferRef)pixelBuffer error:(NSError**)error {
    // Create input features dictionary
    MLFeatureValue* inputValue = [MLFeatureValue featureValueWithPixelBuffer:pixelBuffer];
    
    // Find the input feature name
    NSString* inputName = [[[_model modelDescription] inputDescriptionsByName] allKeys].firstObject;
    if (!inputName) {
        if (error) {
            *error = [NSError errorWithDomain:@"com.fbneo.coreml" 
                                         code:400 
                                     userInfo:@{NSLocalizedDescriptionKey: @"Model has no input features"}];
        }
        _lastErrorMessage = @"Model has no input features";
        return nil;
    }
    
    // Create feature provider
    NSDictionary* inputFeatures = @{inputName: inputValue};
    MLDictionaryFeatureProvider* provider = [[MLDictionaryFeatureProvider alloc] 
                                            initWithDictionary:inputFeatures 
                                                        error:error];
    
    if (!provider) {
        _lastErrorMessage = [NSString stringWithFormat:@"Failed to create feature provider: %@", 
                            *error ? [*error localizedDescription] : @"Unknown error"];
        return nil;
    }
    
    // Run prediction
    id<MLFeatureProvider> outputFeatures = [_model predictionFromFeatures:provider error:error];
    [provider release];
    
    if (!outputFeatures) {
        _lastErrorMessage = [NSString stringWithFormat:@"Prediction failed: %@", 
                            *error ? [*error localizedDescription] : @"Unknown error"];
        return nil;
    }
    
    // Convert output to dictionary
    NSMutableDictionary* results = [NSMutableDictionary dictionary];
    for (NSString* featureName in [outputFeatures featureNames]) {
        MLFeatureValue* value = [outputFeatures featureValueForName:featureName];
        if (value) {
            results[featureName] = value;
        }
    }
    
    return results;
}

- (NSDictionary*)inferWithFeatures:(NSDictionary*)features error:(NSError**)error {
    if (!_isModelLoaded) {
        if (error) {
            *error = [NSError errorWithDomain:@"com.fbneo.coreml" 
                                         code:400 
                                     userInfo:@{NSLocalizedDescriptionKey: @"Model not loaded"}];
        }
        _lastErrorMessage = @"Model not loaded";
        return nil;
    }
    
    // Start timing the inference
    NSDate* startTime = [NSDate date];
    
    // Create feature provider
    MLDictionaryFeatureProvider* provider = [[MLDictionaryFeatureProvider alloc] 
                                            initWithDictionary:features 
                                                        error:error];
    
    if (!provider) {
        _lastErrorMessage = [NSString stringWithFormat:@"Failed to create feature provider: %@", 
                            *error ? [*error localizedDescription] : @"Unknown error"];
        return nil;
    }
    
    // Run prediction
    id<MLFeatureProvider> outputFeatures = [_model predictionFromFeatures:provider error:error];
    [provider release];
    
    if (!outputFeatures) {
        _lastErrorMessage = [NSString stringWithFormat:@"Prediction failed: %@", 
                            *error ? [*error localizedDescription] : @"Unknown error"];
        return nil;
    }
    
    // Convert output to dictionary
    NSMutableDictionary* results = [NSMutableDictionary dictionary];
    for (NSString* featureName in [outputFeatures featureNames]) {
        MLFeatureValue* value = [outputFeatures featureValueForName:featureName];
        if (value) {
            results[featureName] = value;
        }
    }
    
    // Calculate inference time
    NSTimeInterval inferenceTime = [[NSDate date] timeIntervalSinceDate:startTime] * 1000.0; // ms
    _lastInferenceTimeMs = inferenceTime;
    
    // Update average inference time
    if (_inferenceCounter == 0) {
        _avgInferenceTimeMs = inferenceTime;
    } else {
        // Exponential moving average with alpha=0.1
        _avgInferenceTimeMs = 0.9 * _avgInferenceTimeMs + 0.1 * inferenceTime;
    }
    
    // Increment counter
    _inferenceCounter++;
    
    return results;
}

- (NSArray*)batchInferWithFrameBuffers:(NSArray*)buffers error:(NSError**)error {
    if (!_isModelLoaded) {
        if (error) {
            *error = [NSError errorWithDomain:@"com.fbneo.coreml" 
                                         code:400 
                                     userInfo:@{NSLocalizedDescriptionKey: @"Model not loaded"}];
        }
        _lastErrorMessage = @"Model not loaded";
        return nil;
    }
    
    // Limit batch size to maximum supported by the model
    NSUInteger batchSize = MIN(buffers.count, _maxBatchSize);
    
    // If batch size is 1 or Vision API is used, fall back to individual inference
    if (batchSize == 1 || _useVision) {
        NSMutableArray* results = [NSMutableArray arrayWithCapacity:buffers.count];
        
        for (NSDictionary* bufferInfo in buffers) {
            void* buffer = (__bridge void*)(bufferInfo[@"buffer"]);
            int width = [bufferInfo[@"width"] intValue];
            int height = [bufferInfo[@"height"] intValue];
            
            NSError* singleError = nil;
            NSDictionary* result = [self inferWithFrameBuffer:buffer 
                                                      width:width 
                                                     height:height 
                                                      error:&singleError];
            
            if (result) {
                [results addObject:result];
            } else {
                if (error && !*error) {
                    *error = singleError;
                }
            }
        }
        
        return results;
    }
    
    // Start timing the inference
    NSDate* startTime = [NSDate date];
    
    // Implement batch inference directly with CoreML
    // This would require creating a batched input feature and running prediction
    
    // For now, this is a placeholder showing what would be done
    NSMutableArray* results = [NSMutableArray arrayWithCapacity:buffers.count];
    
    if (error) {
        *error = [NSError errorWithDomain:@"com.fbneo.coreml" 
                                     code:501 
                                 userInfo:@{NSLocalizedDescriptionKey: @"Batch inference not fully implemented"}];
    }
    _lastErrorMessage = @"Batch inference not fully implemented";
    
    // Calculate inference time
    NSTimeInterval inferenceTime = [[NSDate date] timeIntervalSinceDate:startTime] * 1000.0; // ms
    _lastInferenceTimeMs = inferenceTime;
    
    return results;
}

- (void)setCacheSize:(int)maxItems {
    _predictionCache.countLimit = maxItems;
    _useCache = (maxItems > 0);
}

- (NSDictionary*)getPerformanceStats {
    return @{
        @"inferenceCount": @(_inferenceCounter),
        @"lastInferenceTimeMs": @(_lastInferenceTimeMs),
        @"avgInferenceTimeMs": @(_avgInferenceTimeMs),
        @"maxBatchSize": @(_maxBatchSize),
        @"useVision": @(_useVision),
        @"useCache": @(_useCache),
        @"cacheSize": @(_predictionCache.countLimit),
        @"cacheHits": @(_predictionCache.totalHitCount),
        @"cacheMisses": @(_predictionCache.totalMissCount)
    };
}

- (void)releaseResources {
    // Clear the cache
    [_predictionCache removeAllObjects];
}

@end

// C interface for the CoreML Inference Manager
extern "C" {
    // Create a new inference manager
    void* CoreMLInference_Create(const char* modelPath) {
        if (!modelPath) return NULL;
        
        NSString* path = [NSString stringWithUTF8String:modelPath];
        return (void*)CFBridgingRetain([[FBNeoCoreMLInferenceManager alloc] 
                                       initWithModelPath:path configuration:nil]);
    }
    
    // Destroy an inference manager
    void CoreMLInference_Destroy(void* handle) {
        if (handle) {
            CFRelease(handle);
        }
    }
    
    // Load a model
    int CoreMLInference_LoadModel(void* handle) {
        if (!handle) return 0;
        
        FBNeoCoreMLInferenceManager* manager = (__bridge FBNeoCoreMLInferenceManager*)handle;
        NSError* error = nil;
        BOOL success = [manager loadModel:&error];
        
        if (!success && error) {
            NSLog(@"Failed to load model: %@", error);
        }
        
        return success ? 1 : 0;
    }
    
    // Run inference with a frame buffer
    int CoreMLInference_InferWithFrameBuffer(void* handle, void* buffer, int width, int height, 
                                            float* outputBuffer, int* outputSize) {
        if (!handle || !buffer || !outputBuffer || !outputSize) return 0;
        
        FBNeoCoreMLInferenceManager* manager = (__bridge FBNeoCoreMLInferenceManager*)handle;
        NSError* error = nil;
        NSDictionary* result = [manager inferWithFrameBuffer:buffer width:width height:height error:&error];
        
        if (!result) {
            NSLog(@"Inference failed: %@", error);
            return 0;
        }
        
        // Find the first MLMultiArray in the results
        for (NSString* key in result) {
            MLFeatureValue* value = result[key];
            if (value.type == MLFeatureTypeMultiArray) {
                MLMultiArray* multiArray = value.multiArrayValue;
                
                // Copy at most outputSize values
                int count = MIN(*outputSize, (int)multiArray.count);
                
                for (int i = 0; i < count; i++) {
                    outputBuffer[i] = [[multiArray objectAtIndexedSubscript:i] floatValue];
                }
                
                // Update the actual output size
                *outputSize = count;
                return 1;
            }
        }
        
        NSLog(@"No MLMultiArray found in inference results");
        return 0;
    }
    
    // Get performance stats as JSON string
    const char* CoreMLInference_GetPerformanceStats(void* handle) {
        if (!handle) return "{}";
        
        FBNeoCoreMLInferenceManager* manager = (__bridge FBNeoCoreMLInferenceManager*)handle;
        NSDictionary* stats = [manager getPerformanceStats];
        
        NSError* error = nil;
        NSData* jsonData = [NSJSONSerialization dataWithJSONObject:stats 
                                                          options:0 
                                                            error:&error];
        
        if (!jsonData) {
            NSLog(@"Failed to serialize performance stats: %@", error);
            return "{}";
        }
        
        static NSString* jsonString = nil;
        jsonString = [[NSString alloc] initWithData:jsonData encoding:NSUTF8StringEncoding];
        return [jsonString UTF8String];
    }
    
    // Set cache size
    void CoreMLInference_SetCacheSize(void* handle, int maxItems) {
        if (!handle) return;
        
        FBNeoCoreMLInferenceManager* manager = (__bridge FBNeoCoreMLInferenceManager*)handle;
        [manager setCacheSize:maxItems];
    }
    
    // Get the last error message
    const char* CoreMLInference_GetLastError(void* handle) {
        if (!handle) return "Invalid handle";
        
        FBNeoCoreMLInferenceManager* manager = (__bridge FBNeoCoreMLInferenceManager*)handle;
        return [manager.lastErrorMessage UTF8String];
    }
    
    // Process game state and return AI action
    int CoreMLInference_ProcessGameState(void* handle, CoreML_GameState* gameState, CoreML_AIAction* action) {
        if (!handle || !gameState || !action) return 0;
        
        FBNeoCoreMLInferenceManager* manager = (__bridge FBNeoCoreMLInferenceManager*)handle;
        
        // Convert game state to frame buffer for inference
        void* buffer = gameState->screenBuffer;
        int width = gameState->screenWidth;
        int height = gameState->screenHeight;
        
        // Allocate output buffer
        const int MAX_OUTPUTS = 20;  // More than enough for all buttons
        float outputs[MAX_OUTPUTS];
        int outputSize = MAX_OUTPUTS;
        
        // Run inference
        int success = CoreMLInference_InferWithFrameBuffer(handle, buffer, width, height, outputs, &outputSize);
        if (!success) {
            return 0;
        }
        
        // Clear action struct
        memset(action, 0, sizeof(CoreML_AIAction));
        
        // Apply thresholding to outputs to determine button presses
        // We assume a simple mapping: outputs 0-11 correspond to the buttons defined in CoreML_AIAction
        const float THRESHOLD = 0.5f;
        
        // Process each output
        if (outputSize >= 1) action->buttonUp = outputs[0] > THRESHOLD ? 1 : 0;
        if (outputSize >= 2) action->buttonDown = outputs[1] > THRESHOLD ? 1 : 0;
        if (outputSize >= 3) action->buttonLeft = outputs[2] > THRESHOLD ? 1 : 0;
        if (outputSize >= 4) action->buttonRight = outputs[3] > THRESHOLD ? 1 : 0;
        if (outputSize >= 5) action->button1 = outputs[4] > THRESHOLD ? 1 : 0;
        if (outputSize >= 6) action->button2 = outputs[5] > THRESHOLD ? 1 : 0;
        if (outputSize >= 7) action->button3 = outputs[6] > THRESHOLD ? 1 : 0;
        if (outputSize >= 8) action->button4 = outputs[7] > THRESHOLD ? 1 : 0;
        if (outputSize >= 9) action->button5 = outputs[8] > THRESHOLD ? 1 : 0;
        if (outputSize >= 10) action->button6 = outputs[9] > THRESHOLD ? 1 : 0;
        if (outputSize >= 11) action->buttonStart = outputs[10] > THRESHOLD ? 1 : 0;
        if (outputSize >= 12) action->buttonCoin = outputs[11] > THRESHOLD ? 1 : 0;
        
        // Calculate average confidence
        float totalConfidence = 0.0f;
        int buttonCount = std::min(outputSize, 12);
        for (int i = 0; i < buttonCount; i++) {
            totalConfidence += outputs[i];
        }
        action->confidenceLevel = buttonCount > 0 ? totalConfidence / buttonCount : 0.0f;
        
        // Special move ID and hold frames could be inferred from additional outputs if available
        if (outputSize >= 13) action->specialMoveId = (int)(outputs[12] * 10.0f);
        if (outputSize >= 14) action->suggestedHoldFrames = (int)(outputs[13] * 10.0f);
        
        return 1;
    }
} 