#import <Foundation/Foundation.h>
#import <CoreML/CoreML.h>
#include "ai_coreml_policy.h"
#include "ai_input_frame.h"
#include <iostream>
#include <vector>

// Private Objective-C++ interface for CoreML operations
@interface AICoreMLWrapper : NSObject

@property (nonatomic, strong) MLModel* model;
@property (nonatomic, strong) NSString* modelPath;
@property (nonatomic, assign) bool isLoaded;
@property (nonatomic, assign) int inputSize;

- (instancetype)init;
- (bool)loadModel:(NSString*)path;
- (NSArray<NSNumber*>*)predictWithInputs:(const std::vector<float>&)inputs;
- (NSString*)modelDescription;

@end

@implementation AICoreMLWrapper

- (instancetype)init {
    self = [super init];
    if (self) {
        _model = nil;
        _modelPath = nil;
        _isLoaded = false;
        _inputSize = 0;
    }
    return self;
}

- (bool)loadModel:(NSString*)path {
    NSError* error = nil;
    
    // Check if the path is a directory (compiled model) or a file (.mlmodel)
    BOOL isDir;
    if (![[NSFileManager defaultManager] fileExistsAtPath:path isDirectory:&isDir]) {
        NSLog(@"Model file not found: %@", path);
        return false;
    }
    
    NSURL* modelURL = [NSURL fileURLWithPath:path];
    
    // If it's a .mlmodel file, we need to compile it first
    if (!isDir && [path.pathExtension isEqualToString:@"mlmodel"]) {
        NSLog(@"Compiling .mlmodel file");
        
        // Compile the model to .mlmodelc
        NSURL* compiledModelURL = [MLModel compileModelAtURL:modelURL error:&error];
        if (error) {
            NSLog(@"Error compiling model: %@", error.localizedDescription);
            return false;
        }
        
        modelURL = compiledModelURL;
    }
    
    // Load the compiled model
    _model = [MLModel modelWithContentsOfURL:modelURL error:&error];
    if (error) {
        NSLog(@"Error loading model: %@", error.localizedDescription);
        return false;
    }
    
    // Get model description to determine input size
    MLModelDescription* description = _model.modelDescription;
    MLFeatureDescription* inputDesc = description.inputDescriptionsByName.allValues.firstObject;
    
    if (inputDesc && inputDesc.type == MLFeatureTypeMultiArray) {
        MLMultiArrayConstraint* constraint = inputDesc.multiArrayConstraint;
        _inputSize = 1;
        for (NSNumber* dim in constraint.shape) {
            _inputSize *= dim.intValue;
        }
        NSLog(@"Model loaded with input size: %d", _inputSize);
    } else {
        NSLog(@"Model does not have the expected input type");
        return false;
    }
    
    _modelPath = path;
    _isLoaded = true;
    return true;
}

- (NSArray<NSNumber*>*)predictWithInputs:(const std::vector<float>&)inputs {
    if (!_isLoaded || !_model) {
        NSLog(@"Cannot predict: no model loaded");
        return @[];
    }
    
    NSError* error = nil;
    
    // Convert input vector to MLMultiArray
    MLMultiArrayConstraint* constraint = _model.modelDescription.inputDescriptionsByName.allValues.firstObject.multiArrayConstraint;
    MLMultiArray* inputArray = [[MLMultiArray alloc] initWithShape:constraint.shape
                                                           dataType:MLMultiArrayDataTypeFloat32
                                                              error:&error];
    if (error) {
        NSLog(@"Error creating MLMultiArray: %@", error.localizedDescription);
        return @[];
    }
    
    // Fill the MLMultiArray with input data
    for (size_t i = 0; i < inputs.size() && i < (size_t)_inputSize; i++) {
        inputArray[(NSInteger)i] = inputs[i];
    }
    
    // Create input dictionary
    NSString* inputName = _model.modelDescription.inputDescriptionsByName.allKeys.firstObject;
    if (!inputName) {
        NSLog(@"Cannot determine model input name");
        return @[];
    }
    
    NSDictionary* inputDict = @{inputName: inputArray};
    
    // Run prediction
    MLPrediction* prediction = [_model predictionFromFeatures:inputDict error:&error];
    if (error) {
        NSLog(@"Error running prediction: %@", error.localizedDescription);
        return @[];
    }
    
    // Extract output
    NSString* outputName = _model.modelDescription.outputDescriptionsByName.allKeys.firstObject;
    if (!outputName) {
        NSLog(@"Cannot determine model output name");
        return @[];
    }
    
    id outputFeature = prediction.featureValueForName(outputName);
    
    // Handle different output types
    if ([outputFeature isKindOfClass:[MLFeatureValue class]]) {
        MLFeatureValue* featureValue = (MLFeatureValue*)outputFeature;
        
        if (featureValue.type == MLFeatureTypeMultiArray) {
            // For multi-array output (logits)
            MLMultiArray* outputArray = featureValue.multiArrayValue;
            NSMutableArray<NSNumber*>* result = [NSMutableArray arrayWithCapacity:outputArray.count];
            
            for (NSInteger i = 0; i < outputArray.count; i++) {
                [result addObject:@(outputArray[i].floatValue)];
            }
            
            return result;
        } 
        else if (featureValue.type == MLFeatureTypeDictionary) {
            // For dictionary output (class probabilities)
            NSDictionary* dict = featureValue.dictionaryValue;
            NSMutableArray<NSNumber*>* result = [NSMutableArray arrayWithCapacity:dict.count];
            
            // Convert dictionary values to array
            for (NSInteger i = 0; i < dict.count; i++) {
                NSString* key = [NSString stringWithFormat:@"%ld", (long)i];
                NSNumber* value = dict[key];
                if (value) {
                    [result addObject:value];
                } else {
                    [result addObject:@(0.0)];  // Default to 0 if missing
                }
            }
            
            return result;
        }
    }
    
    NSLog(@"Unexpected output format from model");
    return @[];
}

- (NSString*)modelDescription {
    if (!_isLoaded || !_model) {
        return @"No model loaded";
    }
    
    MLModelDescription* description = _model.modelDescription;
    NSMutableString* result = [NSMutableString string];
    
    [result appendFormat:@"Model: %@\n", _modelPath];
    [result appendFormat:@"Description: %@\n", description.description];
    
    // Input description
    [result appendString:@"Inputs:\n"];
    for (NSString* key in description.inputDescriptionsByName) {
        MLFeatureDescription* inputDesc = description.inputDescriptionsByName[key];
        [result appendFormat:@"  %@: %@\n", key, inputDesc.description];
    }
    
    // Output description
    [result appendString:@"Outputs:\n"];
    for (NSString* key in description.outputDescriptionsByName) {
        MLFeatureDescription* outputDesc = description.outputDescriptionsByName[key];
        [result appendFormat:@"  %@: %@\n", key, outputDesc.description];
    }
    
    return result;
}

@end

// Implementation of C++ interface
namespace AI {

AICoreMLPolicyModel::AICoreMLPolicyModel() 
    : m_model(nullptr), m_isLoaded(false), m_inputSize(0) {
    // Create Objective-C++ wrapper
    m_model = (__bridge_retained void*)[[AICoreMLWrapper alloc] init];
}

AICoreMLPolicyModel::~AICoreMLPolicyModel() {
    // Release Objective-C++ wrapper
    if (m_model) {
        AICoreMLWrapper* wrapper = (__bridge_transfer AICoreMLWrapper*)m_model;
        wrapper = nil;
        m_model = nullptr;
    }
}

bool AICoreMLPolicyModel::loadModel(const std::string& modelPath) {
    if (!m_model) {
        std::cerr << "CoreML wrapper not initialized" << std::endl;
        return false;
    }
    
    m_modelPath = modelPath;
    
    @autoreleasepool {
        AICoreMLWrapper* wrapper = (__bridge AICoreMLWrapper*)m_model;
        NSString* path = [NSString stringWithUTF8String:modelPath.c_str()];
        
        bool success = [wrapper loadModel:path];
        if (success) {
            m_isLoaded = true;
            m_inputSize = wrapper.inputSize;
            
            // Log model description
            NSString* description = [wrapper modelDescription];
            std::cout << "Loaded CoreML model:\n" << [description UTF8String] << std::endl;
        }
        
        return success;
    }
}

bool AICoreMLPolicyModel::isModelLoaded() const {
    return m_isLoaded;
}

AIOutputAction AICoreMLPolicyModel::predict(const AIInputFrame& input) {
    if (!m_isLoaded || !m_model) {
        std::cerr << "Cannot run inference: no model loaded" << std::endl;
        AIOutputAction defaultAction;
        defaultAction.action = AIAction::IDLE;
        defaultAction.confidence = 0.5f;
        return defaultAction;
    }
    
    @autoreleasepool {
        // Convert AIInputFrame to vector of floats
        std::vector<float> inputVector = input.toVector();
        
        // Get prediction from CoreML model
        AICoreMLWrapper* wrapper = (__bridge AICoreMLWrapper*)m_model;
        NSArray<NSNumber*>* outputs = [wrapper predictWithInputs:inputVector];
        
        if (outputs.count == 0) {
            std::cerr << "No output from CoreML model" << std::endl;
            AIOutputAction defaultAction;
            defaultAction.action = AIAction::IDLE;
            defaultAction.confidence = 0.5f;
            return defaultAction;
        }
        
        // Find the action with highest probability
        int bestActionIdx = 0;
        float bestProb = 0.0f;
        
        for (NSUInteger i = 0; i < outputs.count; i++) {
            float prob = [outputs[i] floatValue];
            if (prob > bestProb) {
                bestProb = prob;
                bestActionIdx = (int)i;
            }
        }
        
        // Ensure the action index is valid
        if (bestActionIdx < 0 || bestActionIdx >= static_cast<int>(AIAction::ACTION_COUNT)) {
            bestActionIdx = static_cast<int>(AIAction::IDLE);
        }
        
        // Create output action
        AIOutputAction action;
        action.action = static_cast<AIAction>(bestActionIdx);
        action.confidence = bestProb;
        
        return action;
    }
}

} // namespace AI 