#import <Foundation/Foundation.h>
#import <CoreML/CoreML.h>
#import <Vision/Vision.h>

#include "ai_definitions.h"

// Forward declarations
@class FBNeoMLPredictor;

// Model metadata class to store additional information about models
@interface FBNeoMLModelMetadata : NSObject
@property (nonatomic, strong) NSString* name;
@property (nonatomic, strong) NSString* version;
@property (nonatomic, strong) NSString* author;
@property (nonatomic, strong) NSString* description;
@property (nonatomic, strong) NSString* license;
@property (nonatomic, strong) NSString* driverName;
@property (nonatomic, assign) int gameGenre;
@property (nonatomic, assign) int supportedFeatures;
@property (nonatomic, assign) float avgInferenceTimeMs;
@property (nonatomic, assign) int memoryUsageBytes;
@property (nonatomic, assign) BOOL isQuantized;
@property (nonatomic, assign) int quantizationBits;

// Initialize with default values
- (instancetype)init;

// Create from dictionary
- (instancetype)initWithDictionary:(NSDictionary*)dict;

// Convert to dictionary
- (NSDictionary*)toDictionary;
@end

// Implementation of model metadata
@implementation FBNeoMLModelMetadata

- (instancetype)init {
    if (self = [super init]) {
        _name = @"Unnamed Model";
        _version = @"1.0";
        _author = @"Unknown";
        _description = @"No description";
        _license = @"Proprietary";
        _driverName = @"";
        _gameGenre = FBNEO_AI_GENRE_OTHER;
        _supportedFeatures = FBNEO_AI_FEATURE_PLAYER_ASSIST;
        _avgInferenceTimeMs = 0.0f;
        _memoryUsageBytes = 0;
        _isQuantized = NO;
        _quantizationBits = 32;
    }
    return self;
}

- (instancetype)initWithDictionary:(NSDictionary*)dict {
    if (self = [self init]) {
        if (dict[@"name"]) _name = dict[@"name"];
        if (dict[@"version"]) _version = dict[@"version"];
        if (dict[@"author"]) _author = dict[@"author"];
        if (dict[@"description"]) _description = dict[@"description"];
        if (dict[@"license"]) _license = dict[@"license"];
        if (dict[@"driverName"]) _driverName = dict[@"driverName"];
        if (dict[@"gameGenre"]) _gameGenre = [dict[@"gameGenre"] intValue];
        if (dict[@"supportedFeatures"]) _supportedFeatures = [dict[@"supportedFeatures"] intValue];
        if (dict[@"avgInferenceTimeMs"]) _avgInferenceTimeMs = [dict[@"avgInferenceTimeMs"] floatValue];
        if (dict[@"memoryUsageBytes"]) _memoryUsageBytes = [dict[@"memoryUsageBytes"] intValue];
        if (dict[@"isQuantized"]) _isQuantized = [dict[@"isQuantized"] boolValue];
        if (dict[@"quantizationBits"]) _quantizationBits = [dict[@"quantizationBits"] intValue];
    }
    return self;
}

- (NSDictionary*)toDictionary {
    return @{
        @"name": _name,
        @"version": _version,
        @"author": _author,
        @"description": _description,
        @"license": _license,
        @"driverName": _driverName,
        @"gameGenre": @(_gameGenre),
        @"supportedFeatures": @(_supportedFeatures),
        @"avgInferenceTimeMs": @(_avgInferenceTimeMs),
        @"memoryUsageBytes": @(_memoryUsageBytes),
        @"isQuantized": @(_isQuantized),
        @"quantizationBits": @(_quantizationBits)
    };
}

@end

// Main model container class
@interface FBNeoAIModelContainer : NSObject
@property (nonatomic, strong) MLModel* model;
@property (nonatomic, strong) NSString* modelPath;
@property (nonatomic, strong) FBNeoMLModelMetadata* metadata;
@property (nonatomic, strong) MLModelConfiguration* configuration;
@property (nonatomic, strong) VNCoreMLModel* visionModel;
@property (nonatomic, strong) FBNeoMLPredictor* predictor;
@property (nonatomic, assign) BOOL isLoaded;
@property (nonatomic, assign) int modelId;

// Initialize with a model path
- (instancetype)initWithPath:(NSString *)path modelId:(int)modelId;

// Load model with configuration
- (BOOL)loadWithConfiguration:(MLModelConfiguration*)config error:(NSError**)error;

// Get model information
- (NSDictionary*)getModelInfo;

// Create a Vision model if supported
- (BOOL)createVisionModel;

// Predict with input data
- (id<MLFeatureProvider>)predictWithFeatures:(id<MLFeatureProvider>)input error:(NSError**)error;

// Predict with image
- (NSDictionary*)predictWithImage:(CVPixelBufferRef)pixelBuffer error:(NSError**)error;

// Create a copy of the model with quantization
- (BOOL)exportQuantizedModelToPath:(NSString*)path 
                       withPrecision:(FBNeoAIPrecision)precision 
                              error:(NSError**)error;
@end

// Predictor class for efficient batch prediction
@interface FBNeoMLPredictor : NSObject
@property (nonatomic, strong) MLModel* model;
@property (nonatomic, strong) MLPredictionOptions* options;
@property (nonatomic, strong) id<MLBatchProvider> batchProvider;
@property (nonatomic, assign) int batchSize;
@property (nonatomic, assign) float avgPredictionTimeMs;

// Initialize with an MLModel
- (instancetype)initWithModel:(MLModel*)model;

// Configure batch size
- (void)setBatchSize:(int)batchSize;

// Run a batch prediction
- (NSArray<id<MLFeatureProvider>>*)predictWithBatch:(NSArray<id<MLFeatureProvider>>*)inputs 
                                            error:(NSError**)error;

// Get average prediction time
- (float)getAveragePredictionTimeMs;
@end

// Implementation of predictor
@implementation FBNeoMLPredictor

- (instancetype)initWithModel:(MLModel*)model {
    if (self = [super init]) {
        _model = model;
        _options = [[MLPredictionOptions alloc] init];
        _batchSize = 1;
        _avgPredictionTimeMs = 0.0f;
    }
    return self;
}

- (void)setBatchSize:(int)batchSize {
    _batchSize = batchSize;
}

- (NSArray<id<MLFeatureProvider>>*)predictWithBatch:(NSArray<id<MLFeatureProvider>>*)inputs 
                                            error:(NSError**)error {
    if (!_model) {
        if (error) {
            *error = [NSError errorWithDomain:@"com.fbneo.coreml" 
                                         code:400 
                                     userInfo:@{NSLocalizedDescriptionKey: @"No model loaded"}];
        }
        return nil;
    }
    
    // Start time for performance measurement
    NSDate* startTime = [NSDate date];
    
    // Perform batch prediction
    NSArray<id<MLFeatureProvider>>* results = nil;
    
    if (@available(macOS 12.0, *)) {
        // Use batch prediction on macOS 12+
        _batchProvider = [[MLArrayBatchProvider alloc] initWithFeatureProviderArray:inputs];
        results = [_model predictionsFromBatch:_batchProvider options:_options error:error];
    } else {
        // Fallback for older macOS - do predictions one by one
        NSMutableArray* individualResults = [NSMutableArray arrayWithCapacity:inputs.count];
        
        for (id<MLFeatureProvider> input in inputs) {
            id<MLFeatureProvider> result = [_model predictionFromFeatures:input options:_options error:error];
            if (result) {
                [individualResults addObject:result];
            } else {
                return nil; // Error occurred
            }
        }
        
        results = individualResults;
    }
    
    // Calculate prediction time
    NSTimeInterval predictionTime = [[NSDate date] timeIntervalSinceDate:startTime] * 1000.0; // ms
    
    // Update average prediction time
    if (_avgPredictionTimeMs == 0.0f) {
        _avgPredictionTimeMs = predictionTime;
    } else {
        _avgPredictionTimeMs = (_avgPredictionTimeMs * 0.9f) + (predictionTime * 0.1f); // Exponential moving average
    }
    
    return results;
}

- (float)getAveragePredictionTimeMs {
    return _avgPredictionTimeMs;
}

@end

// Implementation of model container
@implementation FBNeoAIModelContainer

- (instancetype)initWithPath:(NSString *)path modelId:(int)modelId {
    self = [super init];
    if (self) {
        _modelPath = path;
        _modelId = modelId;
        _metadata = [[FBNeoMLModelMetadata alloc] init];
        _configuration = [[MLModelConfiguration alloc] init];
        _isLoaded = NO;
        
        // Set default configuration
        _configuration.computeUnits = MLComputeUnitsAll;
    }
    return self;
}

- (BOOL)loadWithConfiguration:(MLModelConfiguration*)config error:(NSError**)error {
    if (config) {
        _configuration = config;
    }
    
    // Check if file exists
    if (![[NSFileManager defaultManager] fileExistsAtPath:_modelPath]) {
        if (error) {
            *error = [NSError errorWithDomain:@"com.fbneo.coreml" 
                                         code:404 
                                     userInfo:@{NSLocalizedDescriptionKey: @"Model file not found"}];
        }
        return NO;
    }
    
    // Load the model
    NSURL* modelURL = [NSURL fileURLWithPath:_modelPath];
    _model = [MLModel modelWithContentsOfURL:modelURL configuration:_configuration error:error];
    
    if (!_model) {
        return NO;
    }
    
    // Extract metadata from model
    [self extractMetadataFromModel];
    
    // Create predictor
    _predictor = [[FBNeoMLPredictor alloc] initWithModel:_model];
    
    // Create Vision model if possible
    [self createVisionModel];
    
    _isLoaded = YES;
    return YES;
}

- (void)extractMetadataFromModel {
    // Extract basic metadata from the model description
    MLModelDescription* desc = _model.modelDescription;
    
    // Update name from path if not set
    if ([_metadata.name isEqualToString:@"Unnamed Model"]) {
        _metadata.name = [[_modelPath lastPathComponent] stringByDeletingPathExtension];
    }
    
    // Extract metadata from model description
    if (desc.metadata[@"author"]) {
        _metadata.author = desc.metadata[@"author"];
    }
    
    if (desc.metadata[@"description"]) {
        _metadata.description = desc.metadata[@"description"];
    }
    
    if (desc.metadata[@"license"]) {
        _metadata.license = desc.metadata[@"license"];
    }
    
    if (desc.metadata[@"version"]) {
        _metadata.version = desc.metadata[@"version"];
    }
    
    // Get user-defined metadata which might include our custom properties
    NSDictionary* userDefinedMetadata = _model.modelDescription.metadata;
    
    if (userDefinedMetadata[@"fbneo_driver"]) {
        _metadata.driverName = userDefinedMetadata[@"fbneo_driver"];
    }
    
    if (userDefinedMetadata[@"fbneo_genre"]) {
        _metadata.gameGenre = [userDefinedMetadata[@"fbneo_genre"] intValue];
    }
    
    if (userDefinedMetadata[@"fbneo_features"]) {
        _metadata.supportedFeatures = [userDefinedMetadata[@"fbneo_features"] intValue];
    }
    
    if (userDefinedMetadata[@"is_quantized"]) {
        _metadata.isQuantized = [userDefinedMetadata[@"is_quantized"] boolValue];
    }
    
    if (userDefinedMetadata[@"quantization_bits"]) {
        _metadata.quantizationBits = [userDefinedMetadata[@"quantization_bits"] intValue];
    }
}

- (BOOL)createVisionModel {
    NSError* error = nil;
    _visionModel = [VNCoreMLModel modelForMLModel:_model error:&error];
    
    if (error) {
        NSLog(@"Failed to create Vision model: %@", error);
        _visionModel = nil;
        return NO;
    }
    
    return (_visionModel != nil);
}

- (NSDictionary*)getModelInfo {
    NSMutableDictionary* info = [NSMutableDictionary dictionary];
    
    // Basic info
    info[@"id"] = @(_modelId);
    info[@"path"] = _modelPath;
    info[@"loaded"] = @(_isLoaded);
    
    // Add metadata
    info[@"metadata"] = [_metadata toDictionary];
    
    // Add performance stats if available
    if (_predictor) {
        info[@"avgInferenceTimeMs"] = @([_predictor getAveragePredictionTimeMs]);
    }
    
    // Add model description
    if (_model) {
        MLModelDescription* desc = _model.modelDescription;
        
        // Input info
        NSMutableArray* inputs = [NSMutableArray array];
        for (NSString* key in desc.inputDescriptionsByName) {
            MLFeatureDescription* featureDesc = desc.inputDescriptionsByName[key];
            [inputs addObject:@{
                @"name": key,
                @"type": [self stringForFeatureType:featureDesc.type]
            }];
        }
        info[@"inputs"] = inputs;
        
        // Output info
        NSMutableArray* outputs = [NSMutableArray array];
        for (NSString* key in desc.outputDescriptionsByName) {
            MLFeatureDescription* featureDesc = desc.outputDescriptionsByName[key];
            [outputs addObject:@{
                @"name": key,
                @"type": [self stringForFeatureType:featureDesc.type]
            }];
        }
        info[@"outputs"] = outputs;
        
        // Additional details
        info[@"modelDescription"] = [desc modelDescription] ?: @"No description";
        info[@"predictedFeatureName"] = [desc predictedFeatureName] ?: @"Unknown";
        info[@"predictionOptions"] = desc.predictionOptions ?: @{};
    }
    
    return info;
}

- (NSString*)stringForFeatureType:(MLFeatureType)type {
    switch (type) {
        case MLFeatureTypeInt64:
            return @"Int64";
        case MLFeatureTypeDouble:
            return @"Double";
        case MLFeatureTypeString:
            return @"String";
        case MLFeatureTypeImage:
            return @"Image";
        case MLFeatureTypeMultiArray:
            return @"MultiArray";
        case MLFeatureTypeDictionary:
            return @"Dictionary";
        case MLFeatureTypeSequence:
            return @"Sequence";
        default:
            return @"Unknown";
    }
}

- (id<MLFeatureProvider>)predictWithFeatures:(id<MLFeatureProvider>)input error:(NSError**)error {
    if (!_isLoaded || !_model) {
        if (error) {
            *error = [NSError errorWithDomain:@"com.fbneo.coreml" 
                                         code:400 
                                     userInfo:@{NSLocalizedDescriptionKey: @"Model not loaded"}];
        }
        return nil;
    }
    
    return [_model predictionFromFeatures:input options:[[MLPredictionOptions alloc] init] error:error];
}

- (NSDictionary*)predictWithImage:(CVPixelBufferRef)pixelBuffer error:(NSError**)error {
    if (!_isLoaded || !_visionModel) {
        if (error) {
            *error = [NSError errorWithDomain:@"com.fbneo.coreml" 
                                         code:400 
                                     userInfo:@{NSLocalizedDescriptionKey: @"Vision model not available"}];
        }
        return nil;
    }
    
    // Create a Vision request with the model
    __block NSDictionary* results = nil;
    __block NSError* requestError = nil;
    
    // Create Vision request
    VNCoreMLRequest* request = [[VNCoreMLRequest alloc] initWithModel:_visionModel completionHandler:^(VNRequest* request, NSError* error) {
        if (error) {
            requestError = error;
            return;
        }
        
        // Process the results
        NSMutableDictionary* resultDict = [NSMutableDictionary dictionary];
        
        for (VNObservation* observation in request.results) {
            if ([observation isKindOfClass:[VNCoreMLFeatureValueObservation class]]) {
                // This is a CoreML feature value observation
                VNCoreMLFeatureValueObservation* featureObs = (VNCoreMLFeatureValueObservation*)observation;
                resultDict[featureObs.featureName] = featureObs.featureValue;
            }
        }
        
        results = [resultDict copy];
    }];
    
    // Configure request
    request.imageCropAndScaleOption = VNImageCropAndScaleOptionCenterCrop;
    
    // Create a request handler
    VNImageRequestHandler* handler = [[VNImageRequestHandler alloc] initWithCVPixelBuffer:pixelBuffer options:@{}];
    
    // Execute the request
    [handler performRequests:@[request] error:&requestError];
    
    // Check for errors
    if (requestError) {
        if (error) {
            *error = requestError;
        }
        return nil;
    }
    
    return results;
}

- (BOOL)exportQuantizedModelToPath:(NSString*)path 
                     withPrecision:(FBNeoAIPrecision)precision 
                            error:(NSError**)error {
    if (!_isLoaded || !_model) {
        if (error) {
            *error = [NSError errorWithDomain:@"com.fbneo.coreml" 
                                         code:400 
                                     userInfo:@{NSLocalizedDescriptionKey: @"Model not loaded"}];
        }
        return NO;
    }
    
    NSURL* outputURL = [NSURL fileURLWithPath:path];
    
    // Use model collection for quantization if available
    if (@available(macOS 13.0, *)) {
        MLModelCollection* collection = [[MLModelCollection alloc] init];
        
        // Set up quantization options
        NSMutableDictionary* options = [NSMutableDictionary dictionary];
        
        // Set precision
        switch (precision) {
            case FBNEO_AI_PRECISION_FP16:
                options[@"precision"] = @"fp16";
                break;
            case FBNEO_AI_PRECISION_INT8:
                options[@"precision"] = @"int8";
                break;
            case FBNEO_AI_PRECISION_INT4:
                options[@"precision"] = @"int4";
                break;
            default:
                options[@"precision"] = @"fp32";
                break;
        }
        
        // Perform quantization
        NSProgress* progress = [collection optimizeContentsOfURL:[NSURL fileURLWithPath:_modelPath]
                                                       outputURL:outputURL
                                                   configuration:_configuration
                                                        options:options
                                                          error:error];
        
        if (!progress) {
            return NO;
        }
        
        // Wait for completion
        [progress waitUntilFinished];
        
        // Check if successful
        return progress.completed;
    } else {
        // Fallback for older macOS - just create a copy
        NSFileManager* fileManager = [NSFileManager defaultManager];
        return [fileManager copyItemAtPath:_modelPath toPath:path error:error];
    }
}

@end

// C interface for the model container
extern "C" {
    // Create a new model container
    void* ModelContainer_Create(const char* path, int modelId) {
        if (!path) return NULL;
        
        NSString* modelPath = [NSString stringWithUTF8String:path];
        return (void*)CFBridgingRetain([[FBNeoAIModelContainer alloc] initWithPath:modelPath modelId:modelId]);
    }
    
    // Destroy a model container
    void ModelContainer_Destroy(void* handle) {
        if (handle) {
            CFRelease(handle);
        }
    }
    
    // Load a model
    int ModelContainer_LoadModel(void* handle) {
        if (!handle) return 0;
        
        FBNeoAIModelContainer* container = (__bridge FBNeoAIModelContainer*)handle;
        NSError* error = nil;
        BOOL success = [container loadWithConfiguration:nil error:&error];
        
        if (!success && error) {
            NSLog(@"Failed to load model: %@", error);
        }
        
        return success ? 1 : 0;
    }
    
    // Get model information
    const char* ModelContainer_GetModelInfo(void* handle) {
        if (!handle) return "Invalid handle";
        
        static NSString* infoString = nil;
        @autoreleasepool {
            FBNeoAIModelContainer* container = (__bridge FBNeoAIModelContainer*)handle;
            NSDictionary* info = [container getModelInfo];
            
            // Convert to JSON
            NSError* error = nil;
            NSData* jsonData = [NSJSONSerialization dataWithJSONObject:info options:0 error:&error];
            
            if (error || !jsonData) {
                infoString = @"{\"error\":\"Failed to serialize model info\"}";
            } else {
                infoString = [[NSString alloc] initWithData:jsonData encoding:NSUTF8StringEncoding];
            }
        }
        
        return [infoString UTF8String];
    }
    
    // Run inference with an image buffer
    int ModelContainer_PredictWithImage(void* handle, void* pixelBuffer, int width, int height, void* output, int* outputSize) {
        if (!handle || !pixelBuffer || !output || !outputSize) return 0;
        
        FBNeoAIModelContainer* container = (__bridge FBNeoAIModelContainer*)handle;
        
        // Create a CVPixelBuffer from the raw buffer
        CVPixelBufferRef cvPixelBuffer = NULL;
        CVReturn status = CVPixelBufferCreate(kCFAllocatorDefault, width, height,
                                             kCVPixelFormatType_32BGRA,
                                             NULL, &cvPixelBuffer);
        
        if (status != kCVReturnSuccess) {
            NSLog(@"Failed to create pixel buffer");
            return 0;
        }
        
        // Copy the data into the pixel buffer
        CVPixelBufferLockBaseAddress(cvPixelBuffer, 0);
        void* baseAddress = CVPixelBufferGetBaseAddress(cvPixelBuffer);
        memcpy(baseAddress, pixelBuffer, width * height * 4);
        CVPixelBufferUnlockBaseAddress(cvPixelBuffer, 0);
        
        // Run prediction
        NSError* error = nil;
        NSDictionary* results = [container predictWithImage:cvPixelBuffer error:&error];
        
        // Release the pixel buffer
        CVPixelBufferRelease(cvPixelBuffer);
        
        if (error || !results) {
            NSLog(@"Prediction failed: %@", error);
            return 0;
        }
        
        // Process the results - this is a simplified example
        // In a real implementation, you would convert the results to the expected output format
        
        // For this example, we assume the output is a multiarray of floats
        // representing button presses for the game controller
        MLFeatureValue* outputValue = results.allValues.firstObject;
        if (outputValue && outputValue.type == MLFeatureTypeMultiArray) {
            MLMultiArray* multiArray = outputValue.multiArrayValue;
            
            // Assuming the output contains at most *outputSize float values
            int count = MIN(*outputSize, (int)multiArray.count);
            
            // Copy values to output buffer (assumed to be float*)
            float* outputBuffer = (float*)output;
            for (int i = 0; i < count; i++) {
                outputBuffer[i] = [multiArray objectAtIndexedSubscript:i].floatValue;
            }
            
            // Update the actual output size
            *outputSize = count;
            return 1;
        }
        
        NSLog(@"Unexpected output format");
        return 0;
    }
    
    // Export a quantized model
    int ModelContainer_ExportQuantizedModel(void* handle, const char* outputPath, int precision) {
        if (!handle || !outputPath) return 0;
        
        FBNeoAIModelContainer* container = (__bridge FBNeoAIModelContainer*)handle;
        NSError* error = nil;
        BOOL success = [container exportQuantizedModelToPath:[NSString stringWithUTF8String:outputPath]
                                             withPrecision:(FBNeoAIPrecision)precision
                                                    error:&error];
        
        if (!success && error) {
            NSLog(@"Failed to export quantized model: %@", error);
        }
        
        return success ? 1 : 0;
    }
} 