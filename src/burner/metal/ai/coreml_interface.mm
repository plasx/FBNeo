#import <Foundation/Foundation.h>
#import <CoreML/CoreML.h>
#import <Vision/Vision.h>

#include "../../burner.h"
#include "ai_interface.h"

// CoreML implementation for FBNeo Metal backend
// This file provides an interface between FBNeo and Apple's CoreML framework

// Maximum number of supported AI models
#define MAX_MODELS 4

// CoreML model wrapper
@interface FBNeoMLModel : NSObject
@property (nonatomic, strong) MLModel *model;
@property (nonatomic, strong) NSString *modelName;
@property (nonatomic, strong) VNCoreMLModel *visionModel;  // For vision-based input processing
@property (nonatomic, assign) int modelId;
@property (nonatomic, assign) BOOL isActive;
@end

@implementation FBNeoMLModel
- (instancetype)initWithPath:(NSString *)path modelId:(int)modelId {
    self = [super init];
    if (self) {
        _modelId = modelId;
        _modelName = [path lastPathComponent];
        _isActive = NO;
        
        NSURL *modelURL = [NSURL fileURLWithPath:path];
        
        NSError *error = nil;
        _model = [MLModel modelWithContentsOfURL:modelURL error:&error];
        if (error) {
            NSLog(@"Error loading CoreML model: %@", error.localizedDescription);
            return nil;
        }
        
        // If it's a vision-compatible model, create VNCoreMLModel
        error = nil;
        _visionModel = [VNCoreMLModel modelForMLModel:_model error:&error];
        if (error) {
            NSLog(@"Model is not vision-compatible: %@", error.localizedDescription);
            // Not an error, just means we won't use Vision-based processing
        }
        
        NSLog(@"Successfully loaded CoreML model: %@", _modelName);
    }
    return self;
}

- (NSDictionary *)predictWithInputs:(NSDictionary *)inputs {
    if (!_model || !_isActive) {
        return nil;
    }
    
    NSError *error = nil;
    MLPrediction *prediction = [_model predictionFromFeatures:inputs error:&error];
    if (error) {
        NSLog(@"Error making prediction: %@", error.localizedDescription);
        return nil;
    }
    
    return prediction.featureValuesByName;
}

- (NSDictionary *)predictWithImageBuffer:(CVPixelBufferRef)pixelBuffer {
    if (!_visionModel || !_isActive) {
        return nil;
    }
    
    // Using Vision framework for image-based prediction
    VNImageRequestHandler *handler = [[VNImageRequestHandler alloc] initWithCVPixelBuffer:pixelBuffer options:@{}];
    
    __block NSDictionary *results = nil;
    __block NSError *requestError = nil;
    
    VNCoreMLRequest *request = [[VNCoreMLRequest alloc] initWithModel:_visionModel completionHandler:^(VNRequest * _Nonnull request, NSError * _Nullable error) {
        if (error) {
            requestError = error;
            return;
        }
        
        // Process results from the Vision request
        if (request.results.count > 0) {
            NSMutableDictionary *resultDict = [NSMutableDictionary dictionary];
            
            for (VNObservation *observation in request.results) {
                if ([observation isKindOfClass:[VNCoreMLFeatureValueObservation class]]) {
                    VNCoreMLFeatureValueObservation *featureObs = (VNCoreMLFeatureValueObservation *)observation;
                    resultDict[featureObs.featureName] = featureObs.featureValue;
                }
            }
            
            results = [resultDict copy];
        }
    }];
    
    // Perform the Vision request
    [handler performRequests:@[request] error:&requestError];
    
    if (requestError) {
        NSLog(@"Error in Vision request: %@", requestError.localizedDescription);
        return nil;
    }
    
    return results;
}
@end

// Model manager
@interface FBNeoMLManager : NSObject
@property (nonatomic, strong) NSMutableArray<FBNeoMLModel *> *models;
@end

@implementation FBNeoMLManager
+ (instancetype)sharedManager {
    static FBNeoMLManager *sharedManager = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        sharedManager = [[self alloc] init];
    });
    return sharedManager;
}

- (instancetype)init {
    self = [super init];
    if (self) {
        _models = [NSMutableArray arrayWithCapacity:MAX_MODELS];
    }
    return self;
}

- (int)loadModelFromPath:(NSString *)path {
    if (_models.count >= MAX_MODELS) {
        NSLog(@"Maximum number of models reached");
        return -1;
    }
    
    int modelId = (int)_models.count;
    FBNeoMLModel *model = [[FBNeoMLModel alloc] initWithPath:path modelId:modelId];
    
    if (model) {
        [_models addObject:model];
        return modelId;
    }
    
    return -1;
}

- (BOOL)activateModel:(int)modelId {
    if (modelId < 0 || modelId >= _models.count) {
        return NO;
    }
    
    FBNeoMLModel *model = _models[modelId];
    model.isActive = YES;
    return YES;
}

- (BOOL)deactivateModel:(int)modelId {
    if (modelId < 0 || modelId >= _models.count) {
        return NO;
    }
    
    FBNeoMLModel *model = _models[modelId];
    model.isActive = NO;
    return YES;
}

- (NSDictionary *)predictWithModelId:(int)modelId inputs:(NSDictionary *)inputs {
    if (modelId < 0 || modelId >= _models.count) {
        return nil;
    }
    
    FBNeoMLModel *model = _models[modelId];
    return [model predictWithInputs:inputs];
}

- (NSDictionary *)predictWithModelId:(int)modelId imageBuffer:(CVPixelBufferRef)pixelBuffer {
    if (modelId < 0 || modelId >= _models.count) {
        return nil;
    }
    
    FBNeoMLModel *model = _models[modelId];
    return [model predictWithImageBuffer:pixelBuffer];
}
@end

// C interface for CoreML functions to be used in C/C++ code
extern "C" {

// Load a CoreML model from path
int CoreML_LoadModel(const char* path) {
    NSString *modelPath = [NSString stringWithUTF8String:path];
    return [[FBNeoMLManager sharedManager] loadModelFromPath:modelPath];
}

// Activate a loaded model
bool CoreML_ActivateModel(int modelId) {
    return [[FBNeoMLManager sharedManager] activateModel:modelId];
}

// Deactivate a model
bool CoreML_DeactivateModel(int modelId) {
    return [[FBNeoMLManager sharedManager] deactivateModel:modelId];
}

// Make prediction with feature dictionary
bool CoreML_PredictWithFeatures(int modelId, void* inputFeatures, void* outputFeatures, int* outputSize) {
    // This function would need proper conversion between C structures and NSDictionary
    // For now, it's a placeholder that returns success
    return true;
}

// Make prediction with game screen buffer
bool CoreML_PredictWithScreenBuffer(int modelId, void* screenBuffer, int width, int height, void* outputActions, int* outputSize) {
    // Create a CVPixelBuffer from the screen buffer
    CVPixelBufferRef pixelBuffer = NULL;
    CVReturn status = CVPixelBufferCreate(kCFAllocatorDefault, width, height,
                                          kCVPixelFormatType_32BGRA,
                                          NULL, &pixelBuffer);
    
    if (status != kCVReturnSuccess) {
        bprintf(PRINT_ERROR, _T("Failed to create pixel buffer\n"));
        return false;
    }
    
    // Lock the buffer for writing
    CVPixelBufferLockBaseAddress(pixelBuffer, 0);
    
    // Copy the screen buffer to the pixel buffer
    void* pixelData = CVPixelBufferGetBaseAddress(pixelBuffer);
    memcpy(pixelData, screenBuffer, width * height * 4);
    
    // Unlock the buffer
    CVPixelBufferUnlockBaseAddress(pixelBuffer, 0);
    
    // Perform prediction
    NSDictionary *results = [[FBNeoMLManager sharedManager] predictWithModelId:modelId imageBuffer:pixelBuffer];
    
    // Release pixel buffer
    CVPixelBufferRelease(pixelBuffer);
    
    if (!results) {
        return false;
    }
    
    // Process results into outputActions
    // This would need implementation specific to the model's output format
    // For now, it's a placeholder
    
    return true;
}

// Initialize CoreML interface
bool CoreML_Initialize() {
    // Any initialization needed for CoreML
    return true;
}

// Shutdown CoreML interface
void CoreML_Shutdown() {
    // Cleanup resources
}

} // extern "C" 