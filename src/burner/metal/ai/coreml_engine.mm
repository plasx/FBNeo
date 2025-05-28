#import <Foundation/Foundation.h>
#import <CoreML/CoreML.h>
#import <Vision/Vision.h>
#import <Metal/Metal.h>

#include <vector>
#include <string>
#include <unordered_map>
#include "../metal_declarations.h"
#include "ai_input_frame.h"
#include "ai_output_action.h"

// CoreML Engine for AI acceleration using Apple Neural Engine
class CoreMLEngine {
private:
    // CoreML model
    MLModel* model;
    VNCoreMLModel* visionModel;
    VNCoreMLRequest* request;
    
    // Input and output dictionaries
    NSMutableDictionary<NSString*, id>* inputDict;
    NSDictionary<NSString*, id>* outputDict;
    
    // Model details
    bool modelLoaded;
    std::string modelPath;
    MLModelConfiguration* configuration;
    
    // Input and output feature names
    NSString* inputFeatureName;
    NSString* outputFeatureName;
    
    // Convert AIInputFrame to MLMultiArray
    MLMultiArray* prepareInputData(const AIInputFrame& input);
    
    // Extract output data and convert to AIOutputAction
    void extractOutputData(AIOutputAction& output, MLMultiArray* outputArray);
    
    // Create a CIImage from frame buffer
    CIImage* createImageFromFrameBuffer(const AIInputFrame& input);
    
public:
    CoreMLEngine();
    ~CoreMLEngine();
    
    // Initialize the engine
    bool initialize();
    
    // Load model from file
    bool loadModel(const std::string& path);
    
    // Run inference on input frame
    bool runInference(const AIInputFrame& input, AIOutputAction& output);
    
    // Check if model is loaded
    bool isModelLoaded() const { return modelLoaded; }
    
    // Get model information
    std::string getModelInfo() const;
};

// Implementation of CoreMLEngine
CoreMLEngine::CoreMLEngine() 
    : model(nil), visionModel(nil), request(nil), 
      inputDict(nil), outputDict(nil), modelLoaded(false),
      inputFeatureName(nil), outputFeatureName(nil) {
    
    // Create input dictionary
    inputDict = [[NSMutableDictionary alloc] init];
    
    // Create model configuration
    configuration = [[MLModelConfiguration alloc] init];
    
    // Enable compute optimizations
    configuration.computeUnits = MLComputeUnitsAll;
    
    // Use Neural Engine on Apple Silicon
    if (@available(macOS 12.0, *)) {
        configuration.preferredMetalDevice = MTLCreateSystemDefaultDevice();
    }
}

CoreMLEngine::~CoreMLEngine() {
    // Release CoreML objects
    model = nil;
    visionModel = nil;
    request = nil;
    inputDict = nil;
    outputDict = nil;
}

bool CoreMLEngine::initialize() {
    NSLog(@"Initializing CoreML Engine");
    return true;
}

bool CoreMLEngine::loadModel(const std::string& path) {
    @autoreleasepool {
        NSLog(@"Loading CoreML model from: %s", path.c_str());
        modelPath = path;
        
        // Convert path to NSURL
        NSURL* modelURL = [NSURL fileURLWithPath:[NSString stringWithUTF8String:path.c_str()]];
        
        // Check if the file exists
        NSFileManager* fileManager = [NSFileManager defaultManager];
        if (![fileManager fileExistsAtPath:[modelURL path]]) {
            NSLog(@"Error: Model file does not exist: %@", [modelURL path]);
            return false;
        }
        
        // Try to load the model
        NSError* error = nil;
        model = [MLModel modelWithContentsOfURL:modelURL configuration:configuration error:&error];
        
        if (error || !model) {
            NSLog(@"Error loading CoreML model: %@", error);
            return false;
        }
        
        // Display model information
        NSLog(@"Model loaded successfully: %@", [model modelDescription]);
        NSLog(@"Input features: %@", [model modelDescription].inputDescriptionsByName);
        NSLog(@"Output features: %@", [model modelDescription].outputDescriptionsByName);
        
        // Get input and output feature names
        if ([model modelDescription].inputDescriptionsByName.count > 0) {
            inputFeatureName = [[model modelDescription].inputDescriptionsByName allKeys][0];
        } else {
            NSLog(@"Error: Model has no input features");
            return false;
        }
        
        if ([model modelDescription].outputDescriptionsByName.count > 0) {
            outputFeatureName = [[model modelDescription].outputDescriptionsByName allKeys][0];
        } else {
            NSLog(@"Error: Model has no output features");
            return false;
        }
        
        // Create Vision model for image input models
        if ([[[model modelDescription].inputDescriptionsByName[inputFeatureName] type] isEqualToString:@"Image"]) {
            NSLog(@"Model requires image input, creating Vision model");
            visionModel = [VNCoreMLModel modelForMLModel:model error:&error];
            
            if (error || !visionModel) {
                NSLog(@"Error creating Vision model: %@", error);
                return false;
            }
            
            // Create Vision request
            request = [[VNCoreMLRequest alloc] initWithModel:visionModel completionHandler:nil];
            request.imageCropAndScaleOption = VNImageCropAndScaleOptionCenterCrop;
        }
        
        modelLoaded = true;
        NSLog(@"CoreML model loaded successfully");
        return true;
    }
}

MLMultiArray* CoreMLEngine::prepareInputData(const AIInputFrame& input) {
    @autoreleasepool {
        NSError* error = nil;
        
        // Determine input type
        MLFeatureDescription* inputDesc = [model modelDescription].inputDescriptionsByName[inputFeatureName];
        NSString* inputType = [inputDesc type];
        
        if ([inputType isEqualToString:@"MultiArray"]) {
            // Get expected input shape
            MLMultiArrayConstraint* constraint = (MLMultiArrayConstraint*)[inputDesc multiArrayConstraint];
            NSArray<NSNumber*>* shape = constraint.shape;
            
            // Create a multi-array with the appropriate shape
            MLMultiArray* inputArray = [[MLMultiArray alloc] initWithShape:shape
                                                               dataType:MLMultiArrayDataTypeFloat32
                                                                  error:&error];
            
            if (error || !inputArray) {
                NSLog(@"Error creating input array: %@", error);
                return nil;
            }
            
            // Fill the array with data from the input frame
            if (input.frameBuffer) {
                const uint8_t* frameData = static_cast<const uint8_t*>(input.frameBuffer);
                
                // Adapt this code based on your actual input requirements
                // Here's a simple example for a 4D input [1, 3, H, W] - batch, channels, height, width
                int batchSize = [shape[0] intValue];
                int channels = [shape[1] intValue];
                int height = [shape[2] intValue];
                int width = [shape[3] intValue];
                
                // Simple preprocessing: resize and normalize to [0,1]
                for (int c = 0; c < channels && c < 3; c++) {
                    for (int h = 0; h < height; h++) {
                        for (int w = 0; w < width; w++) {
                            // Sample from the original frame
                            int srcX = w * input.width / width;
                            int srcY = h * input.height / height;
                            
                            // Calculate pixel offset (assuming RGBA format)
                            int pixelOffset = (srcY * input.width + srcX) * 4;
                            
                            // Get pixel value (R,G,B) and normalize
                            float pixelValue = frameData[pixelOffset + c] / 255.0f;
                            
                            // Calculate index in the MLMultiArray
                            NSArray<NSNumber*>* indices = @[@0, @(c), @(h), @(w)];
                            NSUInteger index = [inputArray indexWithSubscript:indices];
                            
                            // Set value
                            inputArray.dataPointer[index] = pixelValue;
                        }
                    }
                }
            }
            
            return inputArray;
            
        } else if ([inputType isEqualToString:@"Image"]) {
            // For image input, just return nil as we'll use Vision API
            return nil;
        } else {
            NSLog(@"Unsupported input type: %@", inputType);
            return nil;
        }
    }
}

CIImage* CoreMLEngine::createImageFromFrameBuffer(const AIInputFrame& input) {
    @autoreleasepool {
        if (!input.frameBuffer || input.width <= 0 || input.height <= 0) {
            return nil;
        }
        
        // Create a CGImage from frameBuffer (assuming RGBA format)
        CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
        CGContextRef cgContext = CGBitmapContextCreate(
            (void*)input.frameBuffer,  // Data pointer
            input.width,               // Width
            input.height,              // Height
            8,                         // Bits per component
            input.width * 4,           // Bytes per row
            colorSpace,                // Color space
            kCGImageAlphaPremultipliedLast  // Bitmap info
        );
        
        CGImageRef cgImage = CGBitmapContextCreateImage(cgContext);
        CIImage* ciImage = [CIImage imageWithCGImage:cgImage];
        
        // Clean up
        CGImageRelease(cgImage);
        CGContextRelease(cgContext);
        CGColorSpaceRelease(colorSpace);
        
        return ciImage;
    }
}

void CoreMLEngine::extractOutputData(AIOutputAction& output, MLMultiArray* outputArray) {
    @autoreleasepool {
        if (!outputArray) {
            NSLog(@"Error: Null output array");
            return;
        }
        
        // Clear output
        output.up = 0;
        output.down = 0;
        output.left = 0;
        output.right = 0;
        for (int i = 0; i < 6; i++) {
            output.buttons[i] = 0;
        }
        
        // Get output shape
        NSArray<NSNumber*>* shape = outputArray.shape;
        NSUInteger outputSize = 1;
        for (NSNumber* dim in shape) {
            outputSize *= [dim unsignedIntegerValue];
        }
        
        // Ensure we have enough outputs
        if (outputSize < 10) {
            NSLog(@"Warning: Output array too small: %lu elements", (unsigned long)outputSize);
            return;
        }
        
        // Simple interpretation of the first 10 outputs
        // In practice, this would depend on your model's specific output format
        
        // First 4 outputs are directions
        NSArray<NSNumber*>* indices = @[@0, @0];
        output.up = outputArray.dataPointer[[outputArray indexWithSubscript:indices]] > 0.5f ? 1 : 0;
        
        indices = @[@0, @1];
        output.down = outputArray.dataPointer[[outputArray indexWithSubscript:indices]] > 0.5f ? 1 : 0;
        
        indices = @[@0, @2];
        output.left = outputArray.dataPointer[[outputArray indexWithSubscript:indices]] > 0.5f ? 1 : 0;
        
        indices = @[@0, @3];
        output.right = outputArray.dataPointer[[outputArray indexWithSubscript:indices]] > 0.5f ? 1 : 0;
        
        // Next 6 outputs are buttons
        for (int i = 0; i < 6; i++) {
            indices = @[@0, @(i + 4)];
            output.buttons[i] = outputArray.dataPointer[[outputArray indexWithSubscript:indices]] > 0.5f ? 1 : 0;
        }
    }
}

bool CoreMLEngine::runInference(const AIInputFrame& input, AIOutputAction& output) {
    if (!modelLoaded || !model) {
        NSLog(@"Error: Model not loaded");
        return false;
    }
    
    @autoreleasepool {
        NSError* error = nil;
        
        // Different approach depending on input type
        MLFeatureDescription* inputDesc = [model modelDescription].inputDescriptionsByName[inputFeatureName];
        NSString* inputType = [inputDesc type];
        
        if ([inputType isEqualToString:@"MultiArray"]) {
            // Prepare input data
            MLMultiArray* inputArray = prepareInputData(input);
            if (!inputArray) {
                NSLog(@"Error: Failed to prepare input data");
                return false;
            }
            
            // Create input dictionary
            MLFeatureValue* inputFeatureValue = [MLFeatureValue featureValueWithMultiArray:inputArray];
            [inputDict setObject:inputFeatureValue forKey:inputFeatureName];
            
            // Create MLFeatureProvider from dictionary
            MLDictionaryFeatureProvider* inputFeatures = [[MLDictionaryFeatureProvider alloc] 
                                                        initWithDictionary:inputDict error:&error];
            
            if (error || !inputFeatures) {
                NSLog(@"Error creating input features: %@", error);
                return false;
            }
            
            // Run prediction
            id<MLFeatureProvider> outputFeatures = [model predictionFromFeatures:inputFeatures 
                                                        options:[[MLPredictionOptions alloc] init] 
                                                          error:&error];
            
            if (error || !outputFeatures) {
                NSLog(@"Error running prediction: %@", error);
                return false;
            }
            
            // Get output
            MLFeatureValue* outputFeatureValue = [outputFeatures featureValueForName:outputFeatureName];
            
            if (!outputFeatureValue) {
                NSLog(@"Error: No output feature value");
                return false;
            }
            
            // Extract output
            if (outputFeatureValue.type == MLFeatureTypeMultiArray) {
                extractOutputData(output, outputFeatureValue.multiArrayValue);
            } else {
                NSLog(@"Unsupported output type: %ld", (long)outputFeatureValue.type);
                return false;
            }
            
        } else if ([inputType isEqualToString:@"Image"]) {
            // Create image from frame buffer
            CIImage* inputImage = createImageFromFrameBuffer(input);
            if (!inputImage) {
                NSLog(@"Error: Failed to create input image");
                return false;
            }
            
            // Create Vision request handler with the image
            VNImageRequestHandler* handler = [[VNImageRequestHandler alloc] 
                                            initWithCIImage:inputImage options:@{}];
            
            // We need to use a semaphore to wait for the async request to complete
            dispatch_semaphore_t semaphore = dispatch_semaphore_create(0);
            
            // We'll capture the results in these variables
            __block NSArray<VNObservation*>* observations = nil;
            __block NSError* requestError = nil;
            
            // Update the request completion handler to capture results
            request.completionHandler = ^(VNRequest* request, NSError* error) {
                observations = request.results;
                requestError = error;
                dispatch_semaphore_signal(semaphore);
            };
            
            // Perform the request
            [handler performRequests:@[request] error:&error];
            
            if (error) {
                NSLog(@"Error performing Vision request: %@", error);
                return false;
            }
            
            // Wait for the request to complete
            dispatch_semaphore_wait(semaphore, DISPATCH_TIME_FOREVER);
            
            // Check for errors
            if (requestError) {
                NSLog(@"Error in Vision request completion: %@", requestError);
                return false;
            }
            
            // Process results
            if (observations && observations.count > 0) {
                // Look for VNCoreMLFeatureValueObservation
                for (VNObservation* obs in observations) {
                    if ([obs isKindOfClass:[VNCoreMLFeatureValueObservation class]]) {
                        VNCoreMLFeatureValueObservation* featureObs = (VNCoreMLFeatureValueObservation*)obs;
                        MLFeatureValue* featureValue = featureObs.featureValue;
                        
                        // Handle different output types
                        if (featureValue.type == MLFeatureTypeMultiArray) {
                            extractOutputData(output, featureValue.multiArrayValue);
                            return true;
                        } else {
                            NSLog(@"Unsupported output type from Vision: %ld", (long)featureValue.type);
                        }
                    }
                }
                
                NSLog(@"No VNCoreMLFeatureValueObservation found in results");
            } else {
                NSLog(@"No observations returned from Vision request");
            }
            
            return false;
        } else {
            NSLog(@"Unsupported input type: %@", inputType);
            return false;
        }
        
        return true;
    }
}

std::string CoreMLEngine::getModelInfo() const {
    if (!modelLoaded || !model) {
        return "No model loaded";
    }
    
    @autoreleasepool {
        NSString* modelDescription = [NSString stringWithFormat:@"Model: %@\nInputs: %@\nOutputs: %@",
                                     [[model modelDescription] modelDescription],
                                     [[model modelDescription] inputDescriptionsByName],
                                     [[model modelDescription] outputDescriptionsByName]];
        
        return [modelDescription UTF8String];
    }
}

// C wrapper functions for the CoreMLEngine
extern "C" {
    // Create a new CoreMLEngine
    void* CoreML_Create() {
        CoreMLEngine* engine = new CoreMLEngine();
        if (!engine->initialize()) {
            delete engine;
            return NULL;
        }
        return engine;
    }
    
    // Destroy a CoreMLEngine
    void CoreML_Destroy(void* handle) {
        if (handle) {
            delete static_cast<CoreMLEngine*>(handle);
        }
    }
    
    // Load a model into the CoreMLEngine
    int CoreML_LoadModel(void* handle, const char* path) {
        if (!handle || !path) {
            return 0;
        }
        
        CoreMLEngine* engine = static_cast<CoreMLEngine*>(handle);
        return engine->loadModel(path) ? 1 : 0;
    }
    
    // Run inference
    int CoreML_RunInference(void* handle, const AIInputFrame* input, AIOutputAction* output) {
        if (!handle || !input || !output) {
            return 0;
        }
        
        CoreMLEngine* engine = static_cast<CoreMLEngine*>(handle);
        return engine->runInference(*input, *output) ? 1 : 0;
    }
    
    // Check if model is loaded
    int CoreML_IsModelLoaded(void* handle) {
        if (!handle) {
            return 0;
        }
        
        CoreMLEngine* engine = static_cast<CoreMLEngine*>(handle);
        return engine->isModelLoaded() ? 1 : 0;
    }
    
    // Get model information
    const char* CoreML_GetModelInfo(void* handle) {
        if (!handle) {
            return "Invalid handle";
        }
        
        static std::string modelInfo;
        CoreMLEngine* engine = static_cast<CoreMLEngine*>(handle);
        modelInfo = engine->getModelInfo();
        return modelInfo.c_str();
    }
} 