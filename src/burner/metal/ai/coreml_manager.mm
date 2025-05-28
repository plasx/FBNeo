#import <Foundation/Foundation.h>
#import <CoreML/CoreML.h>
#import <Metal/Metal.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h>
#import <Vision/Vision.h>

#include "ai_definitions.h"
#include "ai_interface.h"

// Forward declarations
@class FBNeoAIModelContainer;

// CoreML Manager Class
@interface FBNeoCoreMLManager : NSObject

// Singleton access
+ (instancetype)sharedManager;

// Model loading with secure verification
- (BOOL)loadModelFromPath:(NSString *)path 
              withPrivacy:(BOOL)enablePrivacy 
              privacyLevel:(float)noiseScale
              error:(NSError **)error;

// Check if model is loaded and ready
- (BOOL)isModelReady;

// Get information about the loaded model
- (NSDictionary *)modelInformation;

// Batch prediction with hardware acceleration
- (NSArray<NSNumber *> *)predictWithInputFrame:(NSArray<NSNumber *> *)inputFrame
                                  batchSize:(NSInteger)batchSize
                                      error:(NSError **)error;

// Neural Engine optimization methods
- (void)enableHardwareAcceleration:(BOOL)enable;
- (void)setComputeUnits:(MLComputeUnits)units;
- (void)enableLowPrecisionAccumulation:(BOOL)enable;

// Model security and privacy
- (void)setDifferentialPrivacyLevel:(float)noiseScale;
- (void)enableModelEncryption:(BOOL)enable;

// Get underlying MLModel
- (MLModel *)currentModel;

// Model state serialization/deserialization
- (NSData *)serializeModelState;
- (BOOL)deserializeModelState:(NSData *)stateData;

@end

// Implementation of the CoreML Manager
@implementation FBNeoCoreMLManager {
    MLModel *_model;
    FBNeoAIModelContainer *_modelContainer;
    MLModelConfiguration *_modelConfig;
    BOOL _isModelEncrypted;
    BOOL _isDifferentialPrivacyEnabled;
    float _differentialPrivacyNoiseScale;
    MLComputeUnits _computeUnits;
    NSString *_currentModelPath;
    BOOL _useHardwareAcceleration;
    BOOL _useLowPrecisionAccumulation;
}

#pragma mark - Initialization

+ (instancetype)sharedManager {
    static FBNeoCoreMLManager *sharedInstance = nil;
    static dispatch_once_t onceToken;
    
    dispatch_once(&onceToken, ^{
        sharedInstance = [[self alloc] init];
    });
    
    return sharedInstance;
}

- (instancetype)init {
    if (self = [super init]) {
        _modelConfig = [[MLModelConfiguration alloc] init];
        _computeUnits = MLComputeUnitsAll;
        _modelConfig.computeUnits = _computeUnits;
        _isDifferentialPrivacyEnabled = YES;
        _differentialPrivacyNoiseScale = 0.1f;
        _useHardwareAcceleration = YES;
        _useLowPrecisionAccumulation = YES;
        _isModelEncrypted = NO;
        
        // Set up initial configuration
        [self updateModelConfiguration];
        
        NSLog(@"FBNeoCoreMLManager: Initialized with default configuration");
    }
    return self;
}

#pragma mark - Configuration Management

- (void)updateModelConfiguration {
    // Set differential privacy parameters if enabled
    if (_isDifferentialPrivacyEnabled) {
        _modelConfig.parameters = @{
            MLModelParametersDifferentialPrivacyNoiseScale: @(_differentialPrivacyNoiseScale),
            MLModelParametersDifferentialPrivacyNoiseEnabled: @YES
        };
        
        // Add low precision accumulation if enabled
        if (_useLowPrecisionAccumulation) {
            NSMutableDictionary *params = [NSMutableDictionary dictionaryWithDictionary:_modelConfig.parameters];
            params[MLModelParametersAllowLowPrecisionAccumulationOnGPU] = @YES;
            _modelConfig.parameters = params;
        }
    } else {
        // Just set low precision without differential privacy
        if (_useLowPrecisionAccumulation) {
            _modelConfig.parameters = @{
                MLModelParametersAllowLowPrecisionAccumulationOnGPU: @YES
            };
        } else {
            _modelConfig.parameters = @{};
        }
    }
    
    // Update compute units based on current settings
    _modelConfig.computeUnits = _useHardwareAcceleration ? _computeUnits : MLComputeUnitsCPUOnly;
    
    NSLog(@"FBNeoCoreMLManager: Configuration updated with privacy=%d, noise=%.2f, hardware=%d, precision=%d",
          _isDifferentialPrivacyEnabled, _differentialPrivacyNoiseScale, 
          _useHardwareAcceleration, _useLowPrecisionAccumulation);
}

#pragma mark - Model Loading

- (BOOL)loadModelFromPath:(NSString *)path 
              withPrivacy:(BOOL)enablePrivacy 
             privacyLevel:(float)noiseScale
                    error:(NSError **)error {
    
    // Update privacy settings
    _isDifferentialPrivacyEnabled = enablePrivacy;
    _differentialPrivacyNoiseScale = noiseScale;
    [self updateModelConfiguration];
    
    // Verify the model exists
    if (![[NSFileManager defaultManager] fileExistsAtPath:path]) {
        if (error) {
            *error = [NSError errorWithDomain:@"com.fbneo.coreml" 
                                         code:404 
                                     userInfo:@{NSLocalizedDescriptionKey: @"Model file not found"}];
        }
        return NO;
    }
    
    // Verify model signature and integrity (enhanced security)
    if (_isModelEncrypted) {
        BOOL isVerified = [self verifyModelSignatureAtPath:path];
        if (!isVerified) {
            if (error) {
                *error = [NSError errorWithDomain:@"com.fbneo.coreml" 
                                             code:401 
                                         userInfo:@{NSLocalizedDescriptionKey: @"Model signature verification failed"}];
            }
            return NO;
        }
    }
    
    // Load the model with the current configuration
    NSURL *modelURL = [NSURL fileURLWithPath:path];
    
    @try {
        _model = [MLModel modelWithContentsOfURL:modelURL 
                                   configuration:_modelConfig 
                                           error:error];
        
        if (!_model) {
            NSLog(@"FBNeoCoreMLManager: Failed to load model: %@", [*error localizedDescription]);
            return NO;
        }
        
        _currentModelPath = path;
        
        NSLog(@"FBNeoCoreMLManager: Successfully loaded model from %@", path);
        NSLog(@"FBNeoCoreMLManager: Model description: %@", [_model modelDescription]);
        
        return YES;
    } @catch (NSException *exception) {
        NSLog(@"FBNeoCoreMLManager: Exception during model loading: %@", exception);
        if (error) {
            *error = [NSError errorWithDomain:@"com.fbneo.coreml" 
                                        code:500 
                                    userInfo:@{NSLocalizedDescriptionKey: [exception reason]}];
        }
        return NO;
    }
}

- (BOOL)verifyModelSignatureAtPath:(NSString *)path {
    // This would be implemented with Apple's code signing features
    // For now, this is a placeholder for the future implementation
    NSLog(@"FBNeoCoreMLManager: Model signature verification not fully implemented yet");
    
    // Simple file integrity check for now
    NSData *fileData = [NSData dataWithContentsOfFile:path];
    return (fileData != nil && fileData.length > 0);
}

#pragma mark - Model Status

- (BOOL)isModelReady {
    return (_model != nil);
}

- (NSDictionary *)modelInformation {
    if (!_model) {
        return @{@"status": @"Not loaded"};
    }
    
    // Basic model information
    MLModelDescription *desc = _model.modelDescription;
    NSMutableDictionary *info = [NSMutableDictionary dictionary];
    
    info[@"status"] = @"Loaded";
    info[@"path"] = _currentModelPath ?: @"Unknown";
    info[@"author"] = desc.metadata[@"author"] ?: @"Unknown";
    info[@"description"] = desc.metadata[@"description"] ?: @"No description";
    info[@"version"] = desc.metadata[@"versionString"] ?: @"Unknown";
    info[@"license"] = desc.metadata[@"license"] ?: @"Unknown";
    
    // Add input/output information
    NSMutableArray *inputs = [NSMutableArray array];
    for (NSString *key in desc.inputDescriptionsByName) {
        MLFeatureDescription *inputDesc = desc.inputDescriptionsByName[key];
        [inputs addObject:@{
            @"name": key,
            @"type": [self stringForFeatureType:inputDesc.type],
            @"description": inputDesc.metadata[@"description"] ?: @"No description"
        }];
    }
    info[@"inputs"] = inputs;
    
    NSMutableArray *outputs = [NSMutableArray array];
    for (NSString *key in desc.outputDescriptionsByName) {
        MLFeatureDescription *outputDesc = desc.outputDescriptionsByName[key];
        [outputs addObject:@{
            @"name": key,
            @"type": [self stringForFeatureType:outputDesc.type],
            @"description": outputDesc.metadata[@"description"] ?: @"No description"
        }];
    }
    info[@"outputs"] = outputs;
    
    // Add configuration information
    NSMutableDictionary *config = [NSMutableDictionary dictionary];
    config[@"privacyEnabled"] = @(_isDifferentialPrivacyEnabled);
    config[@"noiseScale"] = @(_differentialPrivacyNoiseScale);
    config[@"hardwareAcceleration"] = @(_useHardwareAcceleration);
    config[@"lowPrecision"] = @(_useLowPrecisionAccumulation);
    config[@"encrypted"] = @(_isModelEncrypted);
    config[@"computeUnits"] = [self stringForComputeUnits:_computeUnits];
    
    info[@"configuration"] = config;
    
    return info;
}

- (NSString *)stringForFeatureType:(MLFeatureType)type {
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

- (NSString *)stringForComputeUnits:(MLComputeUnits)units {
    switch (units) {
        case MLComputeUnitsCPUOnly:
            return @"CPU Only";
        case MLComputeUnitsCPUAndGPU:
            return @"CPU and GPU";
        case MLComputeUnitsAll:
            return @"All (CPU, GPU, Neural Engine)";
        default:
            return @"Unknown";
    }
}

#pragma mark - Prediction

- (NSArray<NSNumber *> *)predictWithInputFrame:(NSArray<NSNumber *> *)inputFrame
                                  batchSize:(NSInteger)batchSize
                                      error:(NSError **)error {
    if (!_model) {
        if (error) {
            *error = [NSError errorWithDomain:@"com.fbneo.coreml" 
                                        code:400 
                                    userInfo:@{NSLocalizedDescriptionKey: @"No model loaded"}];
        }
        return nil;
    }
    
    // Convert the input to MLMultiArray
    MLModelDescription *modelDesc = _model.modelDescription;
    NSString *inputName = modelDesc.inputDescriptionsByName.allKeys.firstObject;
    
    if (!inputName) {
        if (error) {
            *error = [NSError errorWithDomain:@"com.fbneo.coreml" 
                                        code:400 
                                    userInfo:@{NSLocalizedDescriptionKey: @"Invalid model: no input feature"}];
        }
        return nil;
    }
    
    // Create a MultiArray from the input frame
    MLFeatureDescription *inputDesc = modelDesc.inputDescriptionsByName[inputName];
    if (inputDesc.type != MLFeatureTypeMultiArray) {
        if (error) {
            *error = [NSError errorWithDomain:@"com.fbneo.coreml" 
                                        code:400 
                                    userInfo:@{NSLocalizedDescriptionKey: @"Model input must be MultiArray"}];
        }
        return nil;
    }
    
    NSArray<NSNumber *> *shape = inputDesc.multiArrayConstraint.shape;
    if (shape.count < 1) {
        if (error) {
            *error = [NSError errorWithDomain:@"com.fbneo.coreml" 
                                        code:400 
                                    userInfo:@{NSLocalizedDescriptionKey: @"Invalid model input shape"}];
        }
        return nil;
    }
    
    // Create a multi-array for the batch input
    NSMutableArray<NSNumber *> *actualShape = [shape mutableCopy];
    actualShape[0] = @(batchSize);  // Set the batch size
    
    MLMultiArray *inputMultiArray;
    @try {
        inputMultiArray = [[MLMultiArray alloc] initWithShape:actualShape
                                                    dataType:MLMultiArrayDataTypeFloat32
                                                       error:error];
        if (!inputMultiArray) {
            return nil;
        }
        
        // Copy the input data to the multi-array
        NSUInteger elementCount = inputFrame.count / batchSize;
        for (NSInteger batch = 0; batch < batchSize; batch++) {
            for (NSUInteger i = 0; i < elementCount; i++) {
                NSUInteger index = batch * elementCount + i;
                if (index < inputFrame.count) {
                    [inputMultiArray setSingleValue:inputFrame[index].floatValue
                                          atIndex:@(batch * elementCount + i)];
                }
            }
        }
    } @catch (NSException *exception) {
        NSLog(@"FBNeoCoreMLManager: Exception during input preparation: %@", exception);
        if (error) {
            *error = [NSError errorWithDomain:@"com.fbneo.coreml" 
                                        code:500 
                                    userInfo:@{NSLocalizedDescriptionKey: [exception reason]}];
        }
        return nil;
    }
    
    // Create the input dictionary for the model
    NSDictionary *inputFeatures = @{inputName: [MLFeatureValue featureValueWithMultiArray:inputMultiArray]};
    MLDictionaryFeatureProvider *inputProvider = [[MLDictionaryFeatureProvider alloc] initWithDictionary:inputFeatures error:error];
    if (!inputProvider) {
        return nil;
    }
    
    // Run prediction
    id<MLFeatureProvider> outputProvider;
    @try {
        outputProvider = [_model predictionFromFeatures:inputProvider error:error];
        if (!outputProvider) {
            return nil;
        }
    } @catch (NSException *exception) {
        NSLog(@"FBNeoCoreMLManager: Exception during prediction: %@", exception);
        if (error) {
            *error = [NSError errorWithDomain:@"com.fbneo.coreml" 
                                        code:500 
                                    userInfo:@{NSLocalizedDescriptionKey: [exception reason]}];
        }
        return nil;
    }
    
    // Extract the output
    NSString *outputName = modelDesc.outputDescriptionsByName.allKeys.firstObject;
    MLFeatureValue *outputFeature = [outputProvider featureValueForName:outputName];
    
    if (!outputFeature || outputFeature.type != MLFeatureTypeMultiArray) {
        if (error) {
            *error = [NSError errorWithDomain:@"com.fbneo.coreml" 
                                        code:400 
                                    userInfo:@{NSLocalizedDescriptionKey: @"Invalid model output"}];
        }
        return nil;
    }
    
    // Convert the output to an NSArray of NSNumbers
    MLMultiArray *outputMultiArray = outputFeature.multiArrayValue;
    NSUInteger outputCount = outputMultiArray.count;
    NSMutableArray<NSNumber *> *outputArray = [NSMutableArray arrayWithCapacity:outputCount];
    
    for (NSUInteger i = 0; i < outputCount; i++) {
        [outputArray addObject:@([outputMultiArray objectAtIndexedSubscript:i].floatValue)];
    }
    
    return outputArray;
}

#pragma mark - Configuration Methods

- (void)enableHardwareAcceleration:(BOOL)enable {
    _useHardwareAcceleration = enable;
    [self updateModelConfiguration];
}

- (void)setComputeUnits:(MLComputeUnits)units {
    _computeUnits = units;
    [self updateModelConfiguration];
}

- (void)enableLowPrecisionAccumulation:(BOOL)enable {
    _useLowPrecisionAccumulation = enable;
    [self updateModelConfiguration];
}

- (void)setDifferentialPrivacyLevel:(float)noiseScale {
    _differentialPrivacyNoiseScale = noiseScale;
    [self updateModelConfiguration];
}

- (void)enableModelEncryption:(BOOL)enable {
    _isModelEncrypted = enable;
}

#pragma mark - Model Access

- (MLModel *)currentModel {
    return _model;
}

#pragma mark - State Serialization

- (NSData *)serializeModelState {
    // In a real implementation, we would save the model state
    // This is a placeholder for the actual implementation
    NSMutableDictionary *state = [NSMutableDictionary dictionary];
    state[@"modelPath"] = _currentModelPath ?: @"";
    state[@"privacyEnabled"] = @(_isDifferentialPrivacyEnabled);
    state[@"noiseScale"] = @(_differentialPrivacyNoiseScale);
    state[@"useHardwareAcceleration"] = @(_useHardwareAcceleration);
    state[@"useLowPrecisionAccumulation"] = @(_useLowPrecisionAccumulation);
    state[@"isModelEncrypted"] = @(_isModelEncrypted);
    state[@"computeUnits"] = @(_computeUnits);
    
    return [NSKeyedArchiver archivedDataWithRootObject:state requiringSecureCoding:YES error:nil];
}

- (BOOL)deserializeModelState:(NSData *)stateData {
    NSError *error = nil;
    NSDictionary *state = [NSKeyedUnarchiver unarchivedDictionaryWithKeysOfClass:[NSString class]
                                                             objectsOfClass:[NSObject class]
                                                                  fromData:stateData
                                                                     error:&error];
    if (error || !state) {
        NSLog(@"FBNeoCoreMLManager: Failed to deserialize state: %@", error);
        return NO;
    }
    
    NSString *modelPath = state[@"modelPath"];
    if (modelPath && [modelPath length] > 0) {
        _isDifferentialPrivacyEnabled = [state[@"privacyEnabled"] boolValue];
        _differentialPrivacyNoiseScale = [state[@"noiseScale"] floatValue];
        _useHardwareAcceleration = [state[@"useHardwareAcceleration"] boolValue];
        _useLowPrecisionAccumulation = [state[@"useLowPrecisionAccumulation"] boolValue];
        _isModelEncrypted = [state[@"isModelEncrypted"] boolValue];
        _computeUnits = [state[@"computeUnits"] integerValue];
        
        [self updateModelConfiguration];
        
        return [self loadModelFromPath:modelPath 
                          withPrivacy:_isDifferentialPrivacyEnabled 
                         privacyLevel:_differentialPrivacyNoiseScale
                                error:&error];
    }
    
    return NO;
}

@end

// C interface functions for FBNeoCoreMLManager
extern "C" {

void* FBNeo_CoreML_Initialize() {
    // Return the shared instance pointer (as an opaque pointer)
    return (void*)CFBridgingRetain([FBNeoCoreMLManager sharedManager]);
}

int FBNeo_CoreML_LoadModel(void* manager, const char* path, int enablePrivacy, float noiseScale) {
    @autoreleasepool {
        FBNeoCoreMLManager *coremlManager = (__bridge FBNeoCoreMLManager*)manager;
        NSString *modelPath = [NSString stringWithUTF8String:path];
        NSError *error = nil;
        
        BOOL success = [coremlManager loadModelFromPath:modelPath 
                                         withPrivacy:(BOOL)enablePrivacy 
                                        privacyLevel:noiseScale 
                                               error:&error];
        
        if (!success && error) {
            NSLog(@"FBNeo_CoreML_LoadModel failed: %@", error);
        }
        
        return success ? 1 : 0;
    }
}

int FBNeo_CoreML_IsModelReady(void* manager) {
    @autoreleasepool {
        FBNeoCoreMLManager *coremlManager = (__bridge FBNeoCoreMLManager*)manager;
        return [coremlManager isModelReady] ? 1 : 0;
    }
}

float* FBNeo_CoreML_PredictBatch(void* manager, float* inputData, int inputSize, int batchSize, int* outputSize) {
    @autoreleasepool {
        FBNeoCoreMLManager *coremlManager = (__bridge FBNeoCoreMLManager*)manager;
        
        // Convert input data to NSArray
        NSMutableArray<NSNumber*> *inputArray = [NSMutableArray arrayWithCapacity:inputSize];
        for (int i = 0; i < inputSize; i++) {
            [inputArray addObject:@(inputData[i])];
        }
        
        NSError *error = nil;
        NSArray<NSNumber*> *outputArray = [coremlManager predictWithInputFrame:inputArray 
                                                                batchSize:batchSize 
                                                                    error:&error];
        
        if (!outputArray) {
            NSLog(@"FBNeo_CoreML_PredictBatch failed: %@", error);
            *outputSize = 0;
            return NULL;
        }
        
        // Convert result to C array
        *outputSize = (int)outputArray.count;
        float* result = (float*)malloc(sizeof(float) * *outputSize);
        if (!result) {
            *outputSize = 0;
            return NULL;
        }
        
        for (int i = 0; i < *outputSize; i++) {
            result[i] = [outputArray[i] floatValue];
        }
        
        return result;
    }
}

void FBNeo_CoreML_FreeResult(float* data) {
    if (data) {
        free(data);
    }
}

void FBNeo_CoreML_EnableHardwareAcceleration(void* manager, int enable) {
    @autoreleasepool {
        FBNeoCoreMLManager *coremlManager = (__bridge FBNeoCoreMLManager*)manager;
        [coremlManager enableHardwareAcceleration:(BOOL)enable];
    }
}

void FBNeo_CoreML_SetComputeUnits(void* manager, int units) {
    @autoreleasepool {
        FBNeoCoreMLManager *coremlManager = (__bridge FBNeoCoreMLManager*)manager;
        [coremlManager setComputeUnits:(MLComputeUnits)units];
    }
}

void FBNeo_CoreML_Release(void* manager) {
    if (manager) {
        CFRelease(manager);
    }
}

} // extern "C" 