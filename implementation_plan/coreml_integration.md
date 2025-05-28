# CoreML Integration Implementation

This document details the implementation plan for replacing stub functions in the CoreML integration components of FBNeo Metal.

## CoreML Model Loading

```objective-c
// Implementation for CoreML model loading
@implementation CoreMLManager

+ (instancetype)sharedManager {
    static CoreMLManager *instance = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        instance = [[CoreMLManager alloc] init];
    });
    return instance;
}

- (instancetype)init {
    self = [super init];
    if (self) {
        _models = [NSMutableDictionary dictionary];
        _activeModel = nil;
        _modelConfigs = [NSMutableDictionary dictionary];
        _isInitialized = NO;
    }
    return self;
}

- (BOOL)initialize {
    if (_isInitialized) {
        return YES;
    }
    
    // Initialize CoreML
    NSError *error = nil;
    _mlModelConfiguration = [[MLModelConfiguration alloc] init];
    
    // Enable all available optimizations
    _mlModelConfiguration.computeUnits = MLComputeUnitsAll;
    
    // Check for Neural Engine support
    if (@available(macOS 12.0, *)) {
        // Use the Neural Engine if available
        if ([MLComputeUnits.allComputeUnits containsObject:MLComputeUnitsCPUAndNeuralEngine]) {
            _mlModelConfiguration.computeUnits = MLComputeUnitsCPUAndNeuralEngine;
            NSLog(@"Neural Engine is available and will be used");
        }
    }
    
    // Enable low-precision accumulation for performance
    if (@available(macOS 13.0, *)) {
        _mlModelConfiguration.allowLowPrecisionAccumulationOnGPU = YES;
    }
    
    // Look for models in standard directories
    [self scanForModels];
    
    _isInitialized = YES;
    NSLog(@"CoreML manager initialized successfully");
    return YES;
}

- (void)scanForModels {
    // Standard locations for models
    NSArray<NSURL *> *searchPaths = @[
        [[NSBundle mainBundle] bundleURL],
        [[[NSBundle mainBundle] bundleURL] URLByAppendingPathComponent:@"Models"],
        [[[NSFileManager defaultManager] URLsForDirectory:NSApplicationSupportDirectory
                                               inDomains:NSUserDomainMask] firstObject]
    ];
    
    // Add the "models" directory in the application directory
    NSString *appPath = [[[NSBundle mainBundle] bundlePath] stringByDeletingLastPathComponent];
    NSURL *modelsDir = [NSURL fileURLWithPath:[appPath stringByAppendingPathComponent:@"models"]];
    searchPaths = [searchPaths arrayByAddingObject:modelsDir];
    
    // Extensions to look for
    NSArray<NSString *> *extensions = @[@"mlmodel", @"mlpackage", @"mlmodelc"];
    
    // Scan each directory
    for (NSURL *directory in searchPaths) {
        if (![directory isFileURL]) continue;
        
        NSError *error = nil;
        NSArray<NSURL *> *contents = [[NSFileManager defaultManager] contentsOfDirectoryAtURL:directory
                                                                   includingPropertiesForKeys:nil
                                                                                      options:NSDirectoryEnumerationSkipsHiddenFiles
                                                                                        error:&error];
        if (error) {
            NSLog(@"Error scanning directory %@: %@", directory, error);
            continue;
        }
        
        // Look for model files
        for (NSURL *item in contents) {
            NSString *extension = [item pathExtension];
            if ([extensions containsObject:extension]) {
                [self registerModelWithURL:item];
            }
        }
    }
}

- (BOOL)registerModelWithURL:(NSURL *)url {
    NSString *modelID = [url lastPathComponent];
    NSLog(@"Registering model: %@", modelID);
    
    // Check if we already have this model
    if (_models[modelID]) {
        NSLog(@"Model %@ already registered", modelID);
        return YES;
    }
    
    // Load the model
    NSError *error = nil;
    MLModel *model = [MLModel modelWithContentsOfURL:url
                                         configuration:_mlModelConfiguration
                                                error:&error];
    if (error) {
        NSLog(@"Error loading model %@: %@", modelID, error);
        return NO;
    }
    
    // Store the model
    _models[modelID] = model;
    
    // Store model metadata
    NSMutableDictionary *metadata = [NSMutableDictionary dictionary];
    metadata[@"name"] = modelID;
    metadata[@"path"] = [url path];
    
    // Extract model description
    MLModelDescription *modelDescription = model.modelDescription;
    metadata[@"description"] = [modelDescription description];
    
    // Store input/output information
    NSMutableArray *inputs = [NSMutableArray array];
    for (NSString *key in modelDescription.inputDescriptionsByName.allKeys) {
        MLFeatureDescription *desc = modelDescription.inputDescriptionsByName[key];
        [inputs addObject:@{
            @"name": key,
            @"type": [self stringForType:desc.type],
            @"shape": [self shapeForFeatureDescription:desc]
        }];
    }
    metadata[@"inputs"] = inputs;
    
    NSMutableArray *outputs = [NSMutableArray array];
    for (NSString *key in modelDescription.outputDescriptionsByName.allKeys) {
        MLFeatureDescription *desc = modelDescription.outputDescriptionsByName[key];
        [outputs addObject:@{
            @"name": key,
            @"type": [self stringForType:desc.type],
            @"shape": [self shapeForFeatureDescription:desc]
        }];
    }
    metadata[@"outputs"] = outputs;
    
    // Store the metadata
    _modelConfigs[modelID] = metadata;
    
    NSLog(@"Model %@ registered successfully", modelID);
    return YES;
}

// Helper method to get string representation of feature type
- (NSString *)stringForType:(MLFeatureType)type {
    switch (type) {
        case MLFeatureTypeImage:
            return @"image";
        case MLFeatureTypeMultiArray:
            return @"multiArray";
        case MLFeatureTypeDictionary:
            return @"dictionary";
        case MLFeatureTypeSequence:
            return @"sequence";
        default:
            return @"unknown";
    }
}

// Helper method to get shape information
- (NSArray *)shapeForFeatureDescription:(MLFeatureDescription *)desc {
    if (desc.type == MLFeatureTypeMultiArray) {
        MLMultiArrayConstraint *constraint = desc.multiArrayConstraint;
        NSMutableArray *shape = [NSMutableArray array];
        for (NSNumber *dim in constraint.shape) {
            [shape addObject:dim];
        }
        return shape;
    } else if (desc.type == MLFeatureTypeImage) {
        MLImageConstraint *constraint = desc.imageConstraint;
        return @[@(constraint.pixelsWide), @(constraint.pixelsHigh)];
    }
    return @[];
}

- (BOOL)setActiveModel:(NSString *)modelID {
    if (!_models[modelID]) {
        NSLog(@"Model %@ not found", modelID);
        return NO;
    }
    
    _activeModel = _models[modelID];
    _activeModelID = modelID;
    NSLog(@"Active model set to %@", modelID);
    return YES;
}

- (BOOL)predict:(NSDictionary *)inputFeatures outputs:(NSDictionary **)outputFeatures {
    if (!_activeModel) {
        NSLog(@"No active model");
        return NO;
    }
    
    // Convert input features
    NSError *error = nil;
    MLDictionaryFeatureProvider *inputProvider = 
        [[MLDictionaryFeatureProvider alloc] initWithDictionary:inputFeatures error:&error];
    if (error) {
        NSLog(@"Error creating input features: %@", error);
        return NO;
    }
    
    // Run prediction
    id<MLFeatureProvider> outputProvider = 
        [_activeModel predictionFromFeatures:inputProvider error:&error];
    if (error) {
        NSLog(@"Error during prediction: %@", error);
        return NO;
    }
    
    // Convert outputs to a dictionary
    NSMutableDictionary *results = [NSMutableDictionary dictionary];
    for (NSString *name in outputProvider.featureNames) {
        MLFeatureValue *value = [outputProvider featureValueForName:name];
        
        // Convert MLFeatureValue to appropriate Objective-C type
        if (value.type == MLFeatureTypeMultiArray) {
            MLMultiArray *multiArray = value.multiArrayValue;
            // Convert to NSData for easier interoperability
            NSData *data = [NSData dataWithBytes:multiArray.dataPointer 
                                          length:multiArray.count * sizeof(float)];
            results[name] = data;
        } else if (value.type == MLFeatureTypeDictionary) {
            results[name] = value.dictionaryValue;
        } else if (value.type == MLFeatureTypeImage) {
            // Convert to CGImage
            #if TARGET_OS_OSX
            results[name] = value.imageBufferValue;
            #else
            CGImageRef cgImage = [value.imageBufferValue CGImage];
            results[name] = (__bridge id)cgImage;
            #endif
        } else {
            results[name] = value.value;
        }
    }
    
    if (outputFeatures) {
        *outputFeatures = results;
    }
    
    return YES;
}

@end
```

## C API for CoreML Integration

```objective-c
// Implementation for C-compatible CoreML API

#include "ai_stub_types.h"

// Get the shared CoreML manager
static CoreMLManager* GetSharedManager() {
    return [CoreMLManager sharedManager];
}

// Initialize CoreML system
bool CoreML_Initialize() {
    @autoreleasepool {
        CoreMLManager* manager = GetSharedManager();
        return [manager initialize];
    }
}

// Load a CoreML model
int CoreML_LoadModel(const char* path) {
    @autoreleasepool {
        if (!path) return -1;
        
        NSURL* url = [NSURL fileURLWithPath:[NSString stringWithUTF8String:path]];
        
        CoreMLManager* manager = GetSharedManager();
        if ([manager registerModelWithURL:url]) {
            // Set it as the active model
            NSString* modelID = [url lastPathComponent];
            if ([manager setActiveModel:modelID]) {
                // Return model ID (using pointer as integer)
                return (int)[modelID hash];
            }
        }
        
        return -1;
    }
}

// Activate a previously loaded model
bool CoreML_ActivateModel(int modelId) {
    @autoreleasepool {
        CoreMLManager* manager = GetSharedManager();
        NSArray* modelIDs = [manager.models allKeys];
        
        for (NSString* modelID in modelIDs) {
            if ((int)[modelID hash] == modelId) {
                return [manager setActiveModel:modelID];
            }
        }
        
        return false;
    }
}

// Predict using raw features
bool CoreML_PredictWithFeatures(int modelId, void* inputFeatures, void* outputFeatures, int* outputSize) {
    @autoreleasepool {
        CoreMLManager* manager = GetSharedManager();
        
        // Convert C structures to Objective-C
        NSDictionary* inputs = (__bridge NSDictionary*)inputFeatures;
        
        // Run prediction
        NSDictionary* outputs = nil;
        BOOL success = [manager predict:inputs outputs:&outputs];
        
        if (success && outputs) {
            // Copy outputs to C structure
            *(void**)outputFeatures = (__bridge_retained void*)outputs;
            if (outputSize) {
                *outputSize = (int)[outputs count];
            }
            return true;
        }
        
        return false;
    }
}

// Predict using a screen buffer
bool CoreML_PredictWithScreenBuffer(int modelId, void* screenBuffer, int width, int height, void* outputActions, int* outputSize) {
    @autoreleasepool {
        // Get the CoreML manager
        CoreMLManager* manager = GetSharedManager();
        
        // Create a CGImage from the screen buffer
        CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
        CGContextRef context = CGBitmapContextCreate(
            screenBuffer,
            width,
            height,
            8,
            width * 4,
            colorSpace,
            kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big
        );
        
        CGImageRef image = CGBitmapContextCreateImage(context);
        CGContextRelease(context);
        CGColorSpaceRelease(colorSpace);
        
        // Create an MLFeatureValue for the image
        CVPixelBufferRef pixelBuffer = NULL;
        CVReturn status = CVPixelBufferCreate(
            kCFAllocatorDefault,
            width,
            height,
            kCVPixelFormatType_32BGRA,
            NULL,
            &pixelBuffer
        );
        
        if (status != kCVReturnSuccess) {
            CGImageRelease(image);
            return false;
        }
        
        // Lock the pixel buffer and copy the image data
        CVPixelBufferLockBaseAddress(pixelBuffer, 0);
        
        void* pixelData = CVPixelBufferGetBaseAddress(pixelBuffer);
        CGColorSpaceRef colorspace = CGColorSpaceCreateDeviceRGB();
        CGContextRef cgContext = CGBitmapContextCreate(
            pixelData,
            width,
            height,
            8,
            CVPixelBufferGetBytesPerRow(pixelBuffer),
            colorspace,
            kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big
        );
        
        CGContextDrawImage(cgContext, CGRectMake(0, 0, width, height), image);
        
        CGContextRelease(cgContext);
        CGColorSpaceRelease(colorspace);
        CGImageRelease(image);
        
        CVPixelBufferUnlockBaseAddress(pixelBuffer, 0);
        
        // Create MLFeatureValue from pixel buffer
        MLFeatureValue* imageFeature = [MLFeatureValue featureValueWithPixelBuffer:pixelBuffer];
        CVPixelBufferRelease(pixelBuffer);
        
        // Create dictionary of inputs
        NSDictionary* inputs = @{@"image": imageFeature};
        
        // Run prediction
        NSDictionary* outputs = nil;
        BOOL success = [manager predict:inputs outputs:&outputs];
        
        if (success && outputs) {
            // Convert outputs to AIActions
            AIActions* actions = (AIActions*)outputActions;
            actions->action_count = 0;
            
            // Parse the outputs based on model format
            MLFeatureValue* actionsFeature = outputs[@"actions"];
            if (actionsFeature && actionsFeature.type == MLFeatureTypeMultiArray) {
                MLMultiArray* actionsArray = actionsFeature.multiArrayValue;
                
                // Assume the model outputs action probabilities
                for (NSUInteger i = 0; i < actionsArray.count && i < MAX_ACTION_COUNT; i++) {
                    float value = [(NSNumber*)[actionsArray objectAtIndexedSubscript:i] floatValue];
                    
                    // Only include actions with sufficient probability
                    if (value > 0.5f) {
                        actions->actions[actions->action_count].type = AI_ACTION_BUTTON;
                        actions->actions[actions->action_count].input_id = (uint32_t)i;
                        actions->actions[actions->action_count].value = value;
                        actions->action_count++;
                    }
                }
            }
            
            if (outputSize) {
                *outputSize = actions->action_count;
            }
            
            return true;
        }
        
        return false;
    }
}
```

## Frame Processing for AI Input

```objective-c
// Implementation for frame processing:
@implementation FrameProcessor

- (instancetype)init {
    self = [super init];
    if (self) {
        _configuration = [[MLModelConfiguration alloc] init];
        _configuration.computeUnits = MLComputeUnitsAll;
        
        // Create a capture device
        _queue = dispatch_queue_create("com.fbneo.frameProcessingQueue", DISPATCH_QUEUE_SERIAL);
        
        // Initialize frame processing
        [self setupVisionRequest];
    }
    return self;
}

- (void)setupVisionRequest {
    // Get the shared CoreML manager
    CoreMLManager* manager = [CoreMLManager sharedManager];
    MLModel* model = manager.activeModel;
    
    if (!model) {
        NSLog(@"No active model");
        return;
    }
    
    // Create VNCoreMLModel
    NSError* error = nil;
    _visionModel = [VNCoreMLModel modelForMLModel:model error:&error];
    if (error) {
        NSLog(@"Error creating VNCoreMLModel: %@", error);
        return;
    }
    
    // Create VNCoreMLRequest
    _request = [[VNCoreMLRequest alloc] initWithModel:_visionModel 
                                  completionHandler:^(VNRequest * _Nonnull request, 
                                                   NSError * _Nullable error) {
        if (error) {
            NSLog(@"Error in VNCoreMLRequest: %@", error);
            return;
        }
        
        // Process the results
        [self processResults:request.results];
    }];
    
    // Configure the request
    _request.imageCropAndScaleOption = VNImageCropAndScaleOptionCenterCrop;
}

- (void)processFrame:(CVPixelBufferRef)pixelBuffer {
    if (!_request) {
        [self setupVisionRequest];
        if (!_request) {
            return;
        }
    }
    
    // Create a request handler
    VNImageRequestHandler* handler = [[VNImageRequestHandler alloc] initWithCVPixelBuffer:pixelBuffer 
                                                                               options:@{}];
    
    // Perform the request
    dispatch_async(_queue, ^{
        NSError* error = nil;
        [handler performRequests:@[self->_request] error:&error];
        if (error) {
            NSLog(@"Error performing request: %@", error);
        }
    });
}

- (void)processResults:(NSArray<VNObservation*>*)results {
    for (VNObservation* observation in results) {
        if ([observation isKindOfClass:[VNCoreMLFeatureValueObservation class]]) {
            VNCoreMLFeatureValueObservation* featureObs = (VNCoreMLFeatureValueObservation*)observation;
            
            // Process the feature value based on its type
            MLFeatureValue* featureValue = featureObs.featureValue;
            
            if (featureValue.type == MLFeatureTypeMultiArray) {
                // Extract action probabilities from multi-array
                MLMultiArray* multiArray = featureValue.multiArrayValue;
                [self processActionProbabilities:multiArray];
            } else if (featureValue.type == MLFeatureTypeDictionary) {
                // Extract information from dictionary
                NSDictionary* dict = featureValue.dictionaryValue;
                [self processActionDictionary:dict];
            }
        }
    }
}

- (void)processActionProbabilities:(MLMultiArray*)multiArray {
    // Create an array of actions from the multi-array
    AIActions actions;
    actions.action_count = 0;
    
    // Extract probabilities for each action
    for (NSUInteger i = 0; i < multiArray.count && i < MAX_ACTION_COUNT; i++) {
        float value = [(NSNumber*)[multiArray objectAtIndexedSubscript:i] floatValue];
        
        // Only include actions with sufficient probability
        if (value > 0.5f) {
            actions.actions[actions.action_count].type = AI_ACTION_BUTTON;
            actions.actions[actions.action_count].input_id = (uint32_t)i;
            actions.actions[actions.action_count].value = value;
            actions.action_count++;
        }
    }
    
    // Apply the actions if any were found
    if (actions.action_count > 0) {
        [self applyActions:&actions];
    }
}

- (void)processActionDictionary:(NSDictionary*)dict {
    // Create an array of actions from the dictionary
    AIActions actions;
    actions.action_count = 0;
    
    // Extract actions from the dictionary
    for (NSString* key in dict) {
        NSNumber* value = dict[key];
        float probability = [value floatValue];
        
        // Only include actions with sufficient probability
        if (probability > 0.5f) {
            // Parse the action key (e.g., "button_1", "joystick_x", etc.)
            NSArray* components = [key componentsSeparatedByString:@"_"];
            if (components.count >= 2) {
                NSString* type = components[0];
                NSInteger id = [components[1] integerValue];
                
                if ([type isEqualToString:@"button"]) {
                    actions.actions[actions.action_count].type = AI_ACTION_BUTTON;
                    actions.actions[actions.action_count].input_id = (uint32_t)id;
                    actions.actions[actions.action_count].value = probability;
                    actions.action_count++;
                } else if ([type isEqualToString:@"joystick"]) {
                    actions.actions[actions.action_count].type = AI_ACTION_JOYSTICK;
                    actions.actions[actions.action_count].input_id = (uint32_t)id;
                    actions.actions[actions.action_count].value = probability * 2.0f - 1.0f; // Convert to -1..1
                    actions.action_count++;
                }
            }
        }
    }
    
    // Apply the actions if any were found
    if (actions.action_count > 0) {
        [self applyActions:&actions];
    }
}

- (void)applyActions:(AIActions*)actions {
    // Call the C function to apply the actions
    AI_ApplyActions(actions);
}

@end
```

## AI_Predict Implementation

```c
// Implementation of AI_Predict:
bool AI_Predict(const struct AIFrameData* frame_data, struct AIActions* actions) {
    if (!frame_data || !actions || !frame_data->data || frame_data->size == 0) {
        return false;
    }
    
    // Clear the actions structure
    memset(actions, 0, sizeof(struct AIActions));
    
    // Process the frame using CoreML
    bool predict_success = CoreML_PredictWithScreenBuffer(
        0, // Use active model
        frame_data->data,
        frame_data->width,
        frame_data->height,
        actions,
        NULL
    );
    
    if (!predict_success) {
        return false;
    }
    
    // Apply confidence threshold filter
    AIConfig config;
    AI_GetConfiguration(&config);
    
    uint32_t original_count = actions->action_count;
    uint32_t new_count = 0;
    
    for (uint32_t i = 0; i < original_count; i++) {
        if (actions->actions[i].value >= config.confidence_threshold) {
            if (i != new_count) {
                // Move the action to the new position
                actions->actions[new_count] = actions->actions[i];
            }
            new_count++;
        }
    }
    
    actions->action_count = new_count;
    
    return new_count > 0;
}
```

## Implementation Steps

1. **CoreML Manager Implementation**
   - Create singleton manager class
   - Implement model loading and management
   - Add model metadata extraction
   - Support dynamic model switching

2. **Frame Processing Pipeline**
   - Implement frame capture and processing
   - Create Vision integration for image analysis
   - Add support for different input formats
   - Optimize processing for real-time performance

3. **C API Implementation**
   - Create C-compatible function interfaces
   - Implement Objective-C to C bridges
   - Add error handling and logging
   - Create memory management utilities

4. **Action Mapping**
   - Implement mapping from model outputs to game actions
   - Add support for different action types
   - Create confidence filtering for actions
   - Add debugging and visualization tools

5. **Optimization**
   - Add support for model quantization
   - Implement batch processing for efficiency
   - Add caching for frequent predictions
   - Optimize memory usage for mobile devices

## Testing Strategy

For each component:
1. Create test models with known outputs
2. Validate prediction accuracy with test data
3. Measure performance and optimize bottlenecks
4. Test integration with game input system
5. Validate memory usage and potential leaks

## Dependencies

- CoreML framework
- Vision framework
- MPS framework
- FBNeo core emulation
- Metal rendering pipeline 