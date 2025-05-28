#include "coreml_interface.h"
#include "ai_input_frame.h"
#include "ai_output_action.h"
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

// Include CoreML headers only on Apple platforms
#ifdef __APPLE__
#include <CoreML/CoreML.h>
#endif

namespace AI {

CoreMLInterface::CoreMLInterface()
    : input_size(0), output_size(0) {
    // Initialize without a model
}

CoreMLInterface::~CoreMLInterface() {
    // CoreML handles cleanup via shared_ptr
}

bool CoreMLInterface::isAvailable() {
#ifdef __APPLE__
    return true;
#else
    return false;
#endif
}

bool CoreMLInterface::loadModel(const std::string& path) {
    if (!isAvailable()) {
        std::cerr << "CoreML is not available on this platform" << std::endl;
        return false;
    }
    
#ifdef __APPLE__
    try {
        // Check if file exists
        if (!fs::exists(path)) {
            std::cerr << "Model file not found: " << path << std::endl;
            return false;
        }
        
        // Create an NSURL from the path
        NSURL* modelURL = [NSURL fileURLWithPath:[NSString stringWithUTF8String:path.c_str()]];
        
        // Load the compiled model
        NSError* error = nil;
        MLModel* mlModel = [MLModel modelWithContentsOfURL:modelURL error:&error];
        
        if (error || !mlModel) {
            std::cerr << "Error loading CoreML model: " 
                      << [[error localizedDescription] UTF8String] << std::endl;
            return false;
        }
        
        // Store the model in our shared_ptr wrapper
        model = std::shared_ptr<CoreML::MLModel>((__bridge_retained CoreML::MLModel*)mlModel);
        model_path = path;
        
        // Determine input and output sizes from model description
        MLModelDescription* modelDesc = mlModel.modelDescription;
        
        // Extract input size from the model's expected inputs
        MLFeatureDescription* inputDesc = modelDesc.inputDescriptionsByName[@"input"];
        if (inputDesc && inputDesc.type == MLFeatureTypeMultiArray) {
            NSArray<NSNumber*>* shape = inputDesc.multiArrayConstraint.shape;
            if (shape.count > 0) {
                input_size = static_cast<size_t>([shape[0] intValue]);
            }
        }
        
        // Extract output size from the model's expected outputs
        MLFeatureDescription* outputDesc = modelDesc.outputDescriptionsByName[@"output"];
        if (outputDesc && outputDesc.type == MLFeatureTypeMultiArray) {
            NSArray<NSNumber*>* shape = outputDesc.multiArrayConstraint.shape;
            if (shape.count > 0) {
                output_size = static_cast<size_t>([shape[0] intValue]);
            }
        }
        
        // If we couldn't determine sizes from model, use defaults
        if (input_size == 0) {
            input_size = 20; // Match AIInputFrame::toVector size
        }
        
        if (output_size == 0) {
            output_size = static_cast<size_t>(AIAction::ACTION_COUNT);
        }
        
        std::cout << "CoreML model loaded from: " << path << std::endl;
        std::cout << "Input size: " << input_size << ", Output size: " << output_size << std::endl;
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error loading CoreML model: " << e.what() << std::endl;
        model.reset();
        return false;
    }
#else
    std::cerr << "CoreML is not available on this platform" << std::endl;
    return false;
#endif
}

AIOutputAction CoreMLInterface::predict(const AIInputFrame& frame) {
    // Preprocess frame into features
    std::vector<float> features = preprocess(frame);
    
    // Forward through model
    return predict(features);
}

AIOutputAction CoreMLInterface::predict(const std::vector<float>& features) {
    if (!isLoaded()) {
        std::cerr << "CoreML model not loaded" << std::endl;
        return AIOutputAction(); // Return default action
    }
    
#ifdef __APPLE__
    return predictWithCoreML(features);
#else
    std::cerr << "CoreML is not available on this platform" << std::endl;
    return AIOutputAction(); // Return default action
#endif
}

bool CoreMLInterface::isLoaded() const {
#ifdef __APPLE__
    return model != nullptr;
#else
    return false;
#endif
}

size_t CoreMLInterface::getInputSize() const {
    return input_size;
}

size_t CoreMLInterface::getOutputSize() const {
    return output_size;
}

std::string CoreMLInterface::getModelDescription() const {
    if (!isLoaded()) {
        return "No model loaded";
    }
    
#ifdef __APPLE__
    MLModel* mlModel = (__bridge MLModel*)model.get();
    MLModelDescription* modelDesc = mlModel.modelDescription;
    
    std::string description = [[modelDesc.predictedFeatureName stringByAppendingString:@" "] UTF8String];
    description += [[modelDesc.predictedProbabilitiesName stringByAppendingString:@" "] UTF8String];
    
    // Add metadata if available
    NSDictionary* metadata = modelDesc.metadata;
    if (metadata) {
        for (NSString* key in metadata) {
            description += std::string([[key stringByAppendingString:@": "] UTF8String]);
            description += std::string([[metadata[key] description] UTF8String]);
            description += " ";
        }
    }
    
    return description;
#else
    return "CoreML not available on this platform";
#endif
}

std::vector<float> CoreMLInterface::preprocess(const AIInputFrame& frame) const {
    // Convert frame to feature vector
    return frame.toVector();
}

AIOutputAction CoreMLInterface::postprocess(const std::vector<float>& output) const {
    // Assuming output is a probability distribution over actions
    
    if (output.empty() || output.size() != output_size) {
        std::cerr << "Invalid output size" << std::endl;
        return AIOutputAction(); // Return default action
    }
    
    // Find the action with highest probability
    size_t best_action_idx = 0;
    float best_prob = output[0];
    
    for (size_t i = 1; i < output.size(); ++i) {
        if (output[i] > best_prob) {
            best_prob = output[i];
            best_action_idx = i;
        }
    }
    
    // Convert to AIAction
    AIAction action = static_cast<AIAction>(best_action_idx);
    
    // Create and return action with confidence
    return AIOutputAction(action, best_prob);
}

#ifdef __APPLE__
AIOutputAction CoreMLInterface::predictWithCoreML(const std::vector<float>& features) {
    try {
        // Check input size
        if (features.size() != input_size) {
            std::cerr << "Input feature size mismatch. Expected: " 
                      << input_size << ", Got: " << features.size() << std::endl;
            return AIOutputAction(); // Return default action
        }
        
        // Get the underlying MLModel
        MLModel* mlModel = (__bridge MLModel*)model.get();
        
        // Create MLMultiArray for input
        NSError* error = nil;
        MLMultiArray* inputArray = [[MLMultiArray alloc] 
                                   initWithShape:@[@(features.size())]
                                   dataType:MLMultiArrayDataTypeFloat32
                                   error:&error];
        
        if (error || !inputArray) {
            std::cerr << "Error creating input array: " 
                      << [[error localizedDescription] UTF8String] << std::endl;
            return AIOutputAction();
        }
        
        // Fill the input array with features
        for (size_t i = 0; i < features.size(); ++i) {
            inputArray[[NSNumber numberWithUnsignedLong:i]] = [NSNumber numberWithFloat:features[i]];
        }
        
        // Create input dictionary
        NSDictionary* inputDict = @{@"input": inputArray};
        
        // Create MLFeatureProvider
        MLDictionaryFeatureProvider* inputFeatures = 
            [[MLDictionaryFeatureProvider alloc] initWithDictionary:inputDict error:&error];
        
        if (error || !inputFeatures) {
            std::cerr << "Error creating input features: " 
                      << [[error localizedDescription] UTF8String] << std::endl;
            return AIOutputAction();
        }
        
        // Perform prediction
        id<MLFeatureProvider> outputFeatures = [mlModel predictionFromFeatures:inputFeatures
                                                                         error:&error];
        
        if (error || !outputFeatures) {
            std::cerr << "Error in prediction: " 
                      << [[error localizedDescription] UTF8String] << std::endl;
            return AIOutputAction();
        }
        
        // Extract output
        std::vector<float> output = getOutputFromFeatures((__bridge CoreML::MLFeatureProvider*)outputFeatures);
        
        // Postprocess to get AIOutputAction
        return postprocess(output);
    } catch (const std::exception& e) {
        std::cerr << "Error in CoreML prediction: " << e.what() << std::endl;
        return AIOutputAction();
    }
}

std::vector<float> CoreMLInterface::getOutputFromFeatures(CoreML::MLFeatureProvider* outputProvider) const {
    std::vector<float> result;
    
    @autoreleasepool {
        id<MLFeatureProvider> features = (__bridge id<MLFeatureProvider>)outputProvider;
        MLFeatureValue* outputValue = [features featureValueForName:@"output"];
        
        if (outputValue && outputValue.type == MLFeatureTypeMultiArray) {
            MLMultiArray* outputArray = outputValue.multiArrayValue;
            NSUInteger count = outputArray.count;
            result.reserve(count);
            
            // Extract values from the MLMultiArray
            for (NSUInteger i = 0; i < count; ++i) {
                result.push_back([outputArray[i] floatValue]);
            }
        }
    }
    
    return result;
}
#endif

} // namespace AI 