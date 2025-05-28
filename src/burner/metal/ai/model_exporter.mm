#import <Foundation/Foundation.h>
#import <CoreML/CoreML.h>
#import <Metal/Metal.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h>

#include <string>
#include <vector>
#include <cstdint>
#include "ai_definitions.h"

// Model exporter class for CoreML models
class ModelExporter {
private:
    id<MTLDevice> device;
    std::string lastError;
    
    // Convert a CoreML model to a Metal-optimized format
    bool optimizeForMetal(const std::string& inputPath, const std::string& outputPath);
    
    // Log error message
    void setError(const std::string& error);
    
public:
    ModelExporter();
    ~ModelExporter();
    
    // Initialize the exporter
    bool initialize();
    
    // Export a CoreML model to a quantized format
    bool exportQuantizedModel(const std::string& inputPath, 
                             const std::string& outputPath,
                             FBNeoAIPrecision precision);
    
    // Export a CoreML model to a Metal-optimized format
    bool exportMetalOptimizedModel(const std::string& inputPath,
                                  const std::string& outputPath);
    
    // Get the last error message
    std::string getLastError() const;
};

// Implementation
ModelExporter::ModelExporter() : device(nil) {
}

ModelExporter::~ModelExporter() {
    if (device) {
        [device release];
        device = nil;
    }
}

bool ModelExporter::initialize() {
    // Get the default Metal device
    device = MTLCreateSystemDefaultDevice();
    if (!device) {
        setError("Failed to create Metal device");
        return false;
    }
    
    return true;
}

void ModelExporter::setError(const std::string& error) {
    lastError = error;
    printf("ModelExporter error: %s\n", error.c_str());
}

std::string ModelExporter::getLastError() const {
    return lastError;
}

bool ModelExporter::optimizeForMetal(const std::string& inputPath, const std::string& outputPath) {
    @autoreleasepool {
        NSError* error = nil;
        NSURL* inputURL = [NSURL fileURLWithPath:[NSString stringWithUTF8String:inputPath.c_str()]];
        NSURL* outputURL = [NSURL fileURLWithPath:[NSString stringWithUTF8String:outputPath.c_str()]];
        
        // Load the model
        MLModel* model = [MLModel modelWithContentsOfURL:inputURL error:&error];
        if (!model) {
            setError(std::string("Failed to load model: ") + 
                    std::string([[error localizedDescription] UTF8String]));
            return false;
        }
        
        // Configure for Metal optimization
        MLModelConfiguration* config = [[MLModelConfiguration alloc] init];
        config.computeUnits = MLComputeUnitsAll;
        
        // Set Metal device
        if (@available(macOS 12.0, *)) {
            config.preferredMetalDevice = device;
        }
        
        // Create a semaphore to wait for async compilation
        dispatch_semaphore_t semaphore = dispatch_semaphore_create(0);
        __block bool success = false;
        
        // Compile and save optimized model
        [MLModel compileModelAtURL:inputURL 
                    configuration:config 
              completionHandler:^(NSURL* resultURL, NSError* compileError) {
            if (compileError) {
                NSString* errorMsg = [NSString stringWithFormat:@"Failed to compile model: %@", 
                                     [compileError localizedDescription]];
                NSLog(@"%@", errorMsg);
                self->setError([errorMsg UTF8String]);
            } else {
                // Copy the compiled model to the output path
                NSFileManager* fileManager = [NSFileManager defaultManager];
                NSError* copyError = nil;
                
                // Remove existing file if it exists
                if ([fileManager fileExistsAtPath:[outputURL path]]) {
                    [fileManager removeItemAtURL:outputURL error:&copyError];
                    if (copyError) {
                        NSString* errorMsg = [NSString stringWithFormat:@"Failed to remove existing file: %@", 
                                             [copyError localizedDescription]];
                        NSLog(@"%@", errorMsg);
                        self->setError([errorMsg UTF8String]);
                        dispatch_semaphore_signal(semaphore);
                        return;
                    }
                }
                
                // Copy the file
                [fileManager copyItemAtURL:resultURL toURL:outputURL error:&copyError];
                if (copyError) {
                    NSString* errorMsg = [NSString stringWithFormat:@"Failed to copy compiled model: %@", 
                                         [copyError localizedDescription]];
                    NSLog(@"%@", errorMsg);
                    self->setError([errorMsg UTF8String]);
                } else {
                    NSLog(@"Successfully exported Metal-optimized model to: %@", outputURL);
                    success = true;
                }
            }
            
            dispatch_semaphore_signal(semaphore);
        }];
        
        // Wait for the async operation to complete
        dispatch_semaphore_wait(semaphore, DISPATCH_TIME_FOREVER);
        dispatch_release(semaphore);
        
        [config release];
        
        return success;
    }
}

bool ModelExporter::exportQuantizedModel(const std::string& inputPath, 
                                        const std::string& outputPath,
                                        FBNeoAIPrecision precision) {
    @autoreleasepool {
        NSError* error = nil;
        NSURL* inputURL = [NSURL fileURLWithPath:[NSString stringWithUTF8String:inputPath.c_str()]];
        NSURL* outputURL = [NSURL fileURLWithPath:[NSString stringWithUTF8String:outputPath.c_str()]];
        
        // Load the model
        MLModel* model = [MLModel modelWithContentsOfURL:inputURL error:&error];
        if (!model) {
            setError(std::string("Failed to load model: ") + 
                    std::string([[error localizedDescription] UTF8String]));
            return false;
        }
        
        // Configure for quantization
        MLModelConfiguration* config = [[MLModelConfiguration alloc] init];
        config.computeUnits = MLComputeUnitsAll;
        
        // Set Metal device
        if (@available(macOS 12.0, *)) {
            config.preferredMetalDevice = device;
        }
        
        // Set quantization parameters based on desired precision
        NSMutableDictionary* params = [NSMutableDictionary dictionary];
        switch (precision) {
            case FBNEO_AI_PRECISION_FP16:
                params[@"type"] = @"float16";
                break;
            case FBNEO_AI_PRECISION_INT8:
                params[@"type"] = @"int8";
                params[@"nbits"] = @8;
                break;
            case FBNEO_AI_PRECISION_INT4:
                params[@"type"] = @"int4";
                params[@"nbits"] = @4;
                break;
            default: // FP32 or MIXED
                params[@"type"] = @"float32";
                break;
        }
        
        // Create a semaphore to wait for async compilation
        dispatch_semaphore_t semaphore = dispatch_semaphore_create(0);
        __block bool success = false;
        
        // Create a model collection for more advanced operations
        if (@available(macOS 13.0, *)) {
            MLModelCollection* collection = [[MLModelCollection alloc] init];
            NSProgress* progress = [collection optimizeContentsOfURL:inputURL 
                                                   outputURL:outputURL 
                                                configuration:config 
                                                     options:params 
                                                       error:&error];
            
            if (error) {
                NSString* errorMsg = [NSString stringWithFormat:@"Failed to optimize model: %@", 
                                     [error localizedDescription]];
                NSLog(@"%@", errorMsg);
                setError([errorMsg UTF8String]);
                [collection release];
                dispatch_release(semaphore);
                return false;
            }
            
            // Observe progress
            [progress addObserver:NSObject.new 
                      forKeyPath:@"fractionCompleted" 
                         options:NSKeyValueObservingOptionNew 
                         context:NULL];
            
            // Wait for completion
            [progress waitUntilFinished];
            
            if (progress.completed) {
                NSLog(@"Successfully exported quantized model to: %@", outputURL);
                success = true;
            } else {
                setError("Quantization failed or was cancelled");
            }
            
            [collection release];
        } else {
            // Fallback for older macOS versions - just use Metal optimization
            success = optimizeForMetal(inputPath, outputPath);
        }
        
        [config release];
        dispatch_release(semaphore);
        
        return success;
    }
}

bool ModelExporter::exportMetalOptimizedModel(const std::string& inputPath,
                                            const std::string& outputPath) {
    return optimizeForMetal(inputPath, outputPath);
}

// C wrapper functions for the ModelExporter
extern "C" {
    // Create a new exporter
    void* ModelExporter_Create() {
        ModelExporter* exporter = new ModelExporter();
        if (!exporter->initialize()) {
            delete exporter;
            return NULL;
        }
        return exporter;
    }
    
    // Destroy an exporter
    void ModelExporter_Destroy(void* handle) {
        if (handle) {
            delete static_cast<ModelExporter*>(handle);
        }
    }
    
    // Export a quantized model
    int ModelExporter_ExportQuantizedModel(void* handle, 
                                          const char* inputPath,
                                          const char* outputPath,
                                          int precision) {
        if (!handle || !inputPath || !outputPath) {
            return 0;
        }
        
        ModelExporter* exporter = static_cast<ModelExporter*>(handle);
        return exporter->exportQuantizedModel(inputPath, 
                                             outputPath, 
                                             static_cast<FBNeoAIPrecision>(precision)) ? 1 : 0;
    }
    
    // Export a Metal-optimized model
    int ModelExporter_ExportMetalOptimizedModel(void* handle,
                                               const char* inputPath,
                                               const char* outputPath) {
        if (!handle || !inputPath || !outputPath) {
            return 0;
        }
        
        ModelExporter* exporter = static_cast<ModelExporter*>(handle);
        return exporter->exportMetalOptimizedModel(inputPath, outputPath) ? 1 : 0;
    }
    
    // Get the last error message
    const char* ModelExporter_GetLastError(void* handle) {
        if (!handle) {
            return "Invalid handle";
        }
        
        static std::string errorMsg;
        ModelExporter* exporter = static_cast<ModelExporter*>(handle);
        errorMsg = exporter->getLastError();
        return errorMsg.c_str();
    }
} 