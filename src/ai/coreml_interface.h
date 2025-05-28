#pragma once

#include <string>
#include <memory>
#include <vector>

// Forward declarations for CoreML
#ifdef __APPLE__
// CoreML is only available on Apple platforms
namespace CoreML {
    class MLModel;
    class MLFeatureProvider;
}
#endif

namespace AI {

// Forward declarations
struct AIInputFrame;
struct AIOutputAction;

// Class for CoreML model inference
class CoreMLInterface {
public:
    // Constructor
    CoreMLInterface();
    
    // Destructor
    ~CoreMLInterface();
    
    // Check if CoreML is available on this platform
    static bool isAvailable();
    
    // Load a CoreML model from file
    bool loadModel(const std::string& model_path);
    
    // Predict action given an input frame
    AIOutputAction predict(const AIInputFrame& frame);
    
    // Predict action given a vector of features
    AIOutputAction predict(const std::vector<float>& features);
    
    // Check if model is loaded
    bool isLoaded() const;
    
    // Get the size of the input features the model expects
    size_t getInputSize() const;
    
    // Get the number of possible output actions
    size_t getOutputSize() const;
    
    // Get model metadata
    std::string getModelDescription() const;
    
private:
#ifdef __APPLE__
    // CoreML model
    std::shared_ptr<CoreML::MLModel> model;
#endif
    
    // Model metadata
    size_t input_size;
    size_t output_size;
    std::string model_path;
    
    // Helper methods
    std::vector<float> preprocess(const AIInputFrame& frame) const;
    AIOutputAction postprocess(const std::vector<float>& output) const;
    
#ifdef __APPLE__
    // CoreML-specific methods
    AIOutputAction predictWithCoreML(const std::vector<float>& features);
    std::vector<float> getOutputFromFeatures(CoreML::MLFeatureProvider* outputProvider) const;
#endif
};

} // namespace AI 