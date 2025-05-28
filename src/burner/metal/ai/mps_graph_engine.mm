#import <Foundation/Foundation.h>
#import <MetalPerformanceShadersGraph/MetalPerformanceShadersGraph.h>
#import <Metal/Metal.h>

#include <vector>
#include <string>
#include <unordered_map>
#include "../metal_declarations.h"
#include "ai_input_frame.h"
#include "ai_output_action.h"

// MPS Graph Engine for AI acceleration on Apple platforms
class MPSGraphEngine {
private:
    id<MTLDevice> device;
    id<MTLCommandQueue> commandQueue;
    MPSGraph* graph;
    MPSGraphExecutionDescriptor* executionDesc;
    
    // Inputs and outputs tensors
    std::unordered_map<std::string, MPSGraphTensor*> inputTensors;
    std::unordered_map<std::string, MPSGraphTensor*> outputTensors;
    
    // Operation placeholders
    std::unordered_map<std::string, MPSGraphOperation*> operations;
    
    // Input and output buffers
    std::unordered_map<std::string, id<MTLBuffer>> inputBuffers;
    std::unordered_map<std::string, id<MTLBuffer>> outputBuffers;
    
    // Model details
    bool modelLoaded;
    std::string modelPath;
    
    // Convert AIInputFrame to input tensor data
    void prepareInputData(const AIInputFrame& input);
    
    // Extract output data and convert to AIOutputAction
    void extractOutputData(AIOutputAction& output);
    
public:
    MPSGraphEngine();
    ~MPSGraphEngine();
    
    // Initialize the engine
    bool initialize();
    
    // Load model from file
    bool loadModel(const std::string& path);
    
    // Run inference on input frame
    bool runInference(const AIInputFrame& input, AIOutputAction& output);
    
    // Check if model is loaded
    bool isModelLoaded() const { return modelLoaded; }
};

// Implementation of MPSGraphEngine
MPSGraphEngine::MPSGraphEngine() 
    : device(nil), commandQueue(nil), graph(nil), executionDesc(nil), modelLoaded(false) {
}

MPSGraphEngine::~MPSGraphEngine() {
    // MPSGraph objects are reference counted, they'll be automatically released
    inputBuffers.clear();
    outputBuffers.clear();
    inputTensors.clear();
    outputTensors.clear();
    operations.clear();
}

bool MPSGraphEngine::initialize() {
    @autoreleasepool {
        // Get the default Metal device
        device = MTLCreateSystemDefaultDevice();
        if (!device) {
            NSLog(@"Error: Could not create Metal device");
            return false;
        }
        
        // Create a command queue
        commandQueue = [device newCommandQueue];
        if (!commandQueue) {
            NSLog(@"Error: Could not create command queue");
            return false;
        }
        
        // Create a new graph
        graph = [[MPSGraph alloc] init];
        if (!graph) {
            NSLog(@"Error: Could not create MPSGraph");
            return false;
        }
        
        // Create execution descriptor with options
        executionDesc = [[MPSGraphExecutionDescriptor alloc] init];
        executionDesc.waitUntilCompleted = YES; // Synchronous execution
        
        NSLog(@"MPSGraphEngine initialized successfully");
        return true;
    }
}

bool MPSGraphEngine::loadModel(const std::string& path) {
    @autoreleasepool {
        NSLog(@"Loading model from: %s", path.c_str());
        modelPath = path;
        
        // Convert path to NSString
        NSString* nsPath = [NSString stringWithUTF8String:path.c_str()];
        
        // Check if the file exists
        NSFileManager* fileManager = [NSFileManager defaultManager];
        if (![fileManager fileExistsAtPath:nsPath]) {
            NSLog(@"Error: Model file does not exist: %@", nsPath);
            return false;
        }
        
        // Try to load the model
        NSError* error = nil;
        
        // Different loading methods depending on file extension
        if ([nsPath hasSuffix:@".mlpackage"] || [nsPath hasSuffix:@".mlmodel"]) {
            // This is a CoreML model, convert to MPSGraph
            NSLog(@"Loading CoreML model");
            
            // CoreML integration would go here
            // For now, just log that we need to implement this
            NSLog(@"CoreML to MPSGraph conversion not implemented yet");
            return false;
            
        } else if ([nsPath hasSuffix:@".mpsgraph"]) {
            // This is a serialized MPSGraph
            NSLog(@"Loading serialized MPSGraph");
            
            // Load the graph from file
            NSData* data = [NSData dataWithContentsOfFile:nsPath options:0 error:&error];
            if (error || !data) {
                NSLog(@"Error loading graph data: %@", error);
                return false;
            }
            
            // Deserialize the graph
            NSKeyedUnarchiver* unarchiver = [[NSKeyedUnarchiver alloc] initForReadingFromData:data error:&error];
            if (error) {
                NSLog(@"Error creating unarchiver: %@", error);
                return false;
            }
            
            unarchiver.requiresSecureCoding = NO;
            graph = [unarchiver decodeObjectForKey:@"graph"];
            
            if (!graph) {
                NSLog(@"Error deserializing graph");
                return false;
            }
            
            // Setup input and output tensors
            // This would depend on the specific graph structure
            // For now, we'll just create placeholder tensors
            MPSGraphTensor* inputTensor = [graph placeholderWithShape:@[@1, @84, @84, @4] 
                                                 name:@"input"];
            inputTensors["input"] = inputTensor;
            
            MPSGraphTensor* outputTensor = [graph placeholderWithShape:@[@1, @16] 
                                                  name:@"output"];
            outputTensors["output"] = outputTensor;
            
            // Allocate buffers for input and output
            inputBuffers["input"] = [device newBufferWithLength:1 * 84 * 84 * 4 * sizeof(float) 
                                                      options:MTLResourceStorageModeShared];
            outputBuffers["output"] = [device newBufferWithLength:1 * 16 * sizeof(float) 
                                                       options:MTLResourceStorageModeShared];
            
            modelLoaded = true;
            NSLog(@"Model loaded successfully");
            return true;
            
        } else if ([nsPath hasSuffix:@".pt"] || [nsPath hasSuffix:@".pth"]) {
            // This is a PyTorch model, we need to convert it
            NSLog(@"Loading PyTorch model");
            
            // PyTorch to MPSGraph conversion would go here
            // For now, just log that we need to implement this
            NSLog(@"PyTorch to MPSGraph conversion not implemented yet");
            return false;
            
        } else {
            NSLog(@"Unsupported model format: %@", nsPath);
            return false;
        }
    }
}

void MPSGraphEngine::prepareInputData(const AIInputFrame& input) {
    @autoreleasepool {
        // For now, we'll just create a simple placeholder implementation
        if (!inputBuffers.count("input")) {
            NSLog(@"Error: Input buffer not allocated");
            return;
        }
        
        // Example: Copy input data to input buffer
        // In a real implementation, this would extract features from the AIInputFrame
        float* buffer = (float*)[inputBuffers["input"] contents];
        
        // Simple example: Convert a small portion of the frame to floating point
        // In practice, this would be a more sophisticated feature extraction
        if (input.width > 0 && input.height > 0 && input.frameBuffer) {
            const uint8_t* frameData = static_cast<const uint8_t*>(input.frameBuffer);
            
            // Simple downsampling to 84x84 for input to neural network
            for (int y = 0; y < 84; y++) {
                for (int x = 0; x < 84; x++) {
                    // Sample from the original frame with simple scaling
                    int srcX = x * input.width / 84;
                    int srcY = y * input.height / 84;
                    
                    // Get pixel data (assuming RGBA format with 4 bytes per pixel)
                    int pixelOffset = (srcY * input.width + srcX) * 4;
                    
                    // Convert to grayscale and normalize to [0, 1]
                    uint8_t r = frameData[pixelOffset];
                    uint8_t g = frameData[pixelOffset + 1];
                    uint8_t b = frameData[pixelOffset + 2];
                    
                    // Standard RGB to grayscale conversion
                    float gray = (0.299f * r + 0.587f * g + 0.114f * b) / 255.0f;
                    
                    // Store in the input buffer (assuming NHWC format for the first channel)
                    buffer[y * 84 * 4 + x * 4 + 0] = gray;
                    buffer[y * 84 * 4 + x * 4 + 1] = gray;
                    buffer[y * 84 * 4 + x * 4 + 2] = gray;
                    buffer[y * 84 * 4 + x * 4 + 3] = gray;
                }
            }
        }
    }
}

void MPSGraphEngine::extractOutputData(AIOutputAction& output) {
    @autoreleasepool {
        // For now, we'll just create a simple placeholder implementation
        if (!outputBuffers.count("output")) {
            NSLog(@"Error: Output buffer not allocated");
            return;
        }
        
        // Get output buffer contents
        float* buffer = (float*)[outputBuffers["output"] contents];
        
        // Clear the output action
        output.up = 0;
        output.down = 0;
        output.left = 0;
        output.right = 0;
        for (int i = 0; i < 6; i++) {
            output.buttons[i] = 0;
        }
        
        // Example: Convert first 4 outputs to directional inputs
        if (buffer[0] > 0.5f) output.up = 1;
        if (buffer[1] > 0.5f) output.down = 1;
        if (buffer[2] > 0.5f) output.left = 1;
        if (buffer[3] > 0.5f) output.right = 1;
        
        // Example: Convert next 6 outputs to button inputs
        for (int i = 0; i < 6; i++) {
            if (buffer[4 + i] > 0.5f) {
                output.buttons[i] = 1;
            }
        }
    }
}

bool MPSGraphEngine::runInference(const AIInputFrame& input, AIOutputAction& output) {
    if (!modelLoaded || !graph) {
        NSLog(@"Error: Model not loaded");
        return false;
    }
    
    @autoreleasepool {
        // Prepare input data
        prepareInputData(input);
        
        // Set up feed dictionary for inputs
        NSMutableDictionary* feeds = [NSMutableDictionary dictionary];
        for (auto& [name, tensor] : inputTensors) {
            MPSGraphTensorData* tensorData = [[MPSGraphTensorData alloc] 
                                             initWithMTLBuffer:inputBuffers[name]
                                             shape:tensor.shape
                                             dataType:MPSDataTypeFloat32];
            [feeds setObject:tensorData forKey:tensor];
        }
        
        // Set up results dictionary for outputs
        NSMutableDictionary* results = [NSMutableDictionary dictionary];
        for (auto& [name, tensor] : outputTensors) {
            [results setObject:tensor forKey:tensor];
        }
        
        // Run inference
        NSLog(@"Running inference...");
        NSDictionary* resultDict = [graph runWithFeeds:feeds 
                                      targetOperations:nil
                                  targetTensors:results
                                  executionDescriptor:executionDesc];
        
        // Check if inference succeeded
        if (resultDict.count != outputTensors.size()) {
            NSLog(@"Error: Inference failed");
            return false;
        }
        
        // Process results
        for (auto& [name, tensor] : outputTensors) {
            MPSGraphTensorData* tensorData = resultDict[tensor];
            if (!tensorData) {
                NSLog(@"Error: Missing output tensor data for %s", name.c_str());
                continue;
            }
            
            // Copy tensor data to output buffer
            [tensorData synchronizeWithDevice];
            
            // Ensure the output buffer exists and has the right size
            if (!outputBuffers.count(name) || 
                [outputBuffers[name] length] < tensorData.tensor.shape.count * sizeof(float)) {
                
                size_t size = 1;
                for (NSNumber* dim in tensorData.shape) {
                    size *= [dim unsignedIntegerValue];
                }
                
                outputBuffers[name] = [device newBufferWithLength:size * sizeof(float) 
                                              options:MTLResourceStorageModeShared];
            }
            
            // Copy data to our output buffer
            memcpy([outputBuffers[name] contents], [tensorData.buffer contents], 
                   [tensorData.buffer length]);
        }
        
        // Extract output data
        extractOutputData(output);
        
        NSLog(@"Inference completed successfully");
        return true;
    }
}

// C wrapper functions for the MPSGraphEngine
extern "C" {
    // Create a new MPSGraphEngine
    void* MPSGraph_Create() {
        MPSGraphEngine* engine = new MPSGraphEngine();
        if (!engine->initialize()) {
            delete engine;
            return NULL;
        }
        return engine;
    }
    
    // Destroy an MPSGraphEngine
    void MPSGraph_Destroy(void* handle) {
        if (handle) {
            delete static_cast<MPSGraphEngine*>(handle);
        }
    }
    
    // Load a model into the MPSGraphEngine
    int MPSGraph_LoadModel(void* handle, const char* path) {
        if (!handle || !path) {
            return 0;
        }
        
        MPSGraphEngine* engine = static_cast<MPSGraphEngine*>(handle);
        return engine->loadModel(path) ? 1 : 0;
    }
    
    // Run inference
    int MPSGraph_RunInference(void* handle, const AIInputFrame* input, AIOutputAction* output) {
        if (!handle || !input || !output) {
            return 0;
        }
        
        MPSGraphEngine* engine = static_cast<MPSGraphEngine*>(handle);
        return engine->runInference(*input, *output) ? 1 : 0;
    }
    
    // Check if model is loaded
    int MPSGraph_IsModelLoaded(void* handle) {
        if (!handle) {
            return 0;
        }
        
        MPSGraphEngine* engine = static_cast<MPSGraphEngine*>(handle);
        return engine->isModelLoaded() ? 1 : 0;
    }
} 