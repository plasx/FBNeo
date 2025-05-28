#import <Foundation/Foundation.h>
#import <CoreML/CoreML.h>
#import <Vision/Vision.h>

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstdio>
#include <cstring>

#include "ai_definitions.h"

// Class for converting PyTorch models to CoreML format
class TorchToCoreMLConverter {
public:
    // Convert PyTorch model to CoreML format
    bool convertModel(const std::string& torchModelPath,
                      const std::string& coremlOutputPath,
                      const std::string& gameType = "");
    
    // Optimize CoreML model for specific device
    bool optimizeModel(const std::string& inputModelPath,
                       const std::string& outputModelPath,
                       const std::string& deviceType = "ALL");
    
    // Enhanced conversion with optimizations
    bool enhancedConversion(const std::string& torchModelPath,
                           const std::string& coremlOutputPath,
                           const std::string& gameType = "",
                           const std::string& deviceType = "ALL");
    
    TorchToCoreMLConverter();
    ~TorchToCoreMLConverter();
    
private:
    // Run Python script for the actual conversion
    bool runPythonConversion(const std::string& scriptPath,
                            const std::vector<std::string>& args);
    
    // Validate the converted model
    bool validateModel(const std::string& modelPath);
    
    // Find Python executable path
    std::string findPythonPath();
    
    // Convert model type string to enum
    AIModelType modelTypeFromString(const std::string& typeStr);
    
    // Check if file exists
    bool fileExists(const std::string& path);
    
    // Create temporary directory
    std::string createTempDir();
    
    // Clean up temporary files
    void cleanupTempFiles(const std::vector<std::string>& files);
};

// Implementation of TorchToCoreMLConverter
TorchToCoreMLConverter::TorchToCoreMLConverter() {
    // Constructor implementation
}

TorchToCoreMLConverter::~TorchToCoreMLConverter() {
    // Destructor implementation
}

bool TorchToCoreMLConverter::convertModel(const std::string& torchModelPath,
                                        const std::string& coremlOutputPath,
                                        const std::string& gameType) {
    // Check if input file exists
    if (!fileExists(torchModelPath)) {
        NSLog(@"Error: PyTorch model not found at %s", torchModelPath.c_str());
        return false;
    }
    
    // In a real implementation, this would use a Python script with torch.jit and coremltools
    // For this prototype, we'll use a conversion script if available
    
    // Find the conversion script path
    NSString *resourcePath = [[NSBundle mainBundle] resourcePath];
    NSString *conversionScriptPath = [resourcePath stringByAppendingPathComponent:@"scripts/torch_to_coreml.py"];
    
    if (![[NSFileManager defaultManager] fileExistsAtPath:conversionScriptPath]) {
        NSLog(@"Error: Conversion script not found");
        return false;
    }
    
    // Build arguments for the conversion script
    std::vector<std::string> args;
    args.push_back([conversionScriptPath UTF8String]);
    args.push_back("--input");
    args.push_back(torchModelPath);
    args.push_back("--output");
    args.push_back(coremlOutputPath);
    
    if (!gameType.empty()) {
        args.push_back("--game-type");
        args.push_back(gameType);
    }
    
    // Run the conversion script
    bool success = runPythonConversion([conversionScriptPath UTF8String], args);
    
    if (success) {
        // Validate the converted model
        success = validateModel(coremlOutputPath);
    }
    
    return success;
}

bool TorchToCoreMLConverter::optimizeModel(const std::string& inputModelPath,
                                         const std::string& outputModelPath,
                                         const std::string& deviceType) {
    // Check if input model exists
    if (!fileExists(inputModelPath)) {
        NSLog(@"Error: CoreML model not found at %s", inputModelPath.c_str());
        return false;
    }
    
    // For macOS 10.15+, we can use MLModel compiler directly
    if (@available(macOS 10.15, *)) {
        NSURL *modelURL = [NSURL fileURLWithPath:[NSString stringWithUTF8String:inputModelPath.c_str()]];
        
        NSError *error = nil;
        MLModel *model = [MLModel modelWithContentsOfURL:modelURL error:&error];
        
        if (error || !model) {
            NSLog(@"Error loading CoreML model: %@", error.localizedDescription);
            return false;
        }
        
        // Configure compilation options
        MLModelCompilerOptions *options = [[MLModelCompilerOptions alloc] init];
        
        // Set compute units based on device type
        if ([deviceType compare:"CPU" options:NSCaseInsensitiveSearch] == NSOrderedSame) {
            options.computeUnits = MLComputeUnitsCPUOnly;
        } else if ([deviceType compare:"GPU" options:NSCaseInsensitiveSearch] == NSOrderedSame) {
            options.computeUnits = MLComputeUnitsGPUOnly;
        } else if ([deviceType compare:"ANE" options:NSCaseInsensitiveSearch] == NSOrderedSame) {
            if (@available(macOS 11.0, *)) {
                options.computeUnits = MLComputeUnitsNeuralEngine;
            } else {
                options.computeUnits = MLComputeUnitsCPUAndGPU;
            }
        } else if ([deviceType compare:"ALL" options:NSCaseInsensitiveSearch] == NSOrderedSame) {
            options.computeUnits = MLComputeUnitsAll;
        } else {
            options.computeUnits = MLComputeUnitsAll;
        }
        
        // Compile the model
        NSURL *outputURL = [NSURL fileURLWithPath:[NSString stringWithUTF8String:outputModelPath.c_str()]];
        
        error = nil;
        [MLModel compileModelAtURL:modelURL 
                          toDirectory:[outputURL URLByDeletingLastPathComponent] 
                             withOptions:options 
                          configuration:[[MLModelConfiguration alloc] init]
                                 error:&error];
        
        if (error) {
            NSLog(@"Error compiling CoreML model: %@", error.localizedDescription);
            return false;
        }
        
        return true;
    } else {
        NSLog(@"CoreML optimization requires macOS 10.15 or later");
        
        // On older systems, just copy the model
        NSError *error = nil;
        [[NSFileManager defaultManager] copyItemAtPath:[NSString stringWithUTF8String:inputModelPath.c_str()]
                                                 toPath:[NSString stringWithUTF8String:outputModelPath.c_str()]
                                                  error:&error];
        
        if (error) {
            NSLog(@"Error copying model: %@", error.localizedDescription);
            return false;
        }
        
        return true;
    }
}

bool TorchToCoreMLConverter::enhancedConversion(const std::string& torchModelPath,
                                               const std::string& coremlOutputPath,
                                               const std::string& gameType,
                                               const std::string& deviceType) {
    // Create a temporary directory for intermediate files
    std::string tempDir = createTempDir();
    if (tempDir.empty()) {
        NSLog(@"Error creating temporary directory");
        return false;
    }
    
    // Intermediate model path
    std::string intermediatePath = tempDir + "/intermediate.mlmodel";
    
    // First convert from PyTorch to CoreML
    bool conversionSuccess = convertModel(torchModelPath, intermediatePath, gameType);
    if (!conversionSuccess) {
        NSLog(@"Enhanced conversion failed: Could not convert PyTorch model");
        return false;
    }
    
    // Then optimize the CoreML model
    bool optimizationSuccess = optimizeModel(intermediatePath, coremlOutputPath, deviceType);
    if (!optimizationSuccess) {
        NSLog(@"Enhanced conversion failed: Could not optimize CoreML model");
        return false;
    }
    
    // Clean up temporary files
    cleanupTempFiles({intermediatePath});
    
    return true;
}

bool TorchToCoreMLConverter::runPythonConversion(const std::string& scriptPath,
                                               const std::vector<std::string>& args) {
    // Find Python executable
    std::string pythonPath = findPythonPath();
    if (pythonPath.empty()) {
        NSLog(@"Error: Python not found");
        return false;
    }
    
    // Build the command
    std::string command = pythonPath + " ";
    for (const auto& arg : args) {
        command += "\"" + arg + "\" ";
    }
    
    // Run the command
    NSLog(@"Running conversion command: %s", command.c_str());
    int result = std::system(command.c_str());
    
    return (result == 0);
}

bool TorchToCoreMLConverter::validateModel(const std::string& modelPath) {
    // Check if file exists
    if (!fileExists(modelPath)) {
        NSLog(@"Error: CoreML model not found at %s", modelPath.c_str());
        return false;
    }
    
    // Try to load the model to validate it
    NSURL *modelURL = [NSURL fileURLWithPath:[NSString stringWithUTF8String:modelPath.c_str()]];
    
    NSError *error = nil;
    MLModel *model = [MLModel modelWithContentsOfURL:modelURL error:&error];
    
    if (error || !model) {
        NSLog(@"Error validating CoreML model: %@", error.localizedDescription);
        return false;
    }
    
    // Check model description
    MLModelDescription *description = model.modelDescription;
    if (!description) {
        NSLog(@"Error: CoreML model has no description");
        return false;
    }
    
    // Check for expected inputs and outputs
    if (description.inputDescriptionsByName.count == 0) {
        NSLog(@"Error: CoreML model has no inputs");
        return false;
    }
    
    if (description.outputDescriptionsByName.count == 0) {
        NSLog(@"Error: CoreML model has no outputs");
        return false;
    }
    
    NSLog(@"CoreML model validation successful");
    return true;
}

std::string TorchToCoreMLConverter::findPythonPath() {
    // Try different Python locations
    std::vector<std::string> pythonPaths = {
        "/usr/bin/python3",
        "/usr/local/bin/python3",
        "/opt/homebrew/bin/python3",
        "/usr/bin/python",
        "/usr/local/bin/python"
    };
    
    for (const auto& path : pythonPaths) {
        if (fileExists(path)) {
            return path;
        }
    }
    
    // If not found in common locations, try using which
    FILE* pipe = popen("which python3", "r");
    if (pipe) {
        char buffer[256];
        if (fgets(buffer, sizeof(buffer), pipe) != NULL) {
            // Remove trailing newline
            buffer[strcspn(buffer, "\n")] = 0;
            pclose(pipe);
            if (fileExists(buffer)) {
                return buffer;
            }
        }
        pclose(pipe);
    }
    
    pipe = popen("which python", "r");
    if (pipe) {
        char buffer[256];
        if (fgets(buffer, sizeof(buffer), pipe) != NULL) {
            // Remove trailing newline
            buffer[strcspn(buffer, "\n")] = 0;
            pclose(pipe);
            if (fileExists(buffer)) {
                return buffer;
            }
        }
        pclose(pipe);
    }
    
    return "";
}

AIModelType TorchToCoreMLConverter::modelTypeFromString(const std::string& typeStr) {
    if (typeStr == "coreml") {
        return FBNEO_AI_MODEL_TYPE_COREML;
    } else if (typeStr == "pytorch") {
        return FBNEO_AI_MODEL_TYPE_PYTORCH;
    } else if (typeStr == "onnx") {
        return FBNEO_AI_MODEL_TYPE_ONNX;
    } else if (typeStr == "tflite") {
        return FBNEO_AI_MODEL_TYPE_TENSORFLOW_LITE;
    } else if (typeStr == "metal") {
        return FBNEO_AI_MODEL_TYPE_METAL_GRAPH;
    } else {
        return FBNEO_AI_MODEL_TYPE_UNKNOWN;
    }
}

bool TorchToCoreMLConverter::fileExists(const std::string& path) {
    return [[NSFileManager defaultManager] fileExistsAtPath:[NSString stringWithUTF8String:path.c_str()]];
}

std::string TorchToCoreMLConverter::createTempDir() {
    NSString *tempDir = NSTemporaryDirectory();
    NSString *tempDirName = [NSString stringWithFormat:@"fbneo_conversion_%@", [[NSUUID UUID] UUIDString]];
    NSString *fullPath = [tempDir stringByAppendingPathComponent:tempDirName];
    
    NSError *error = nil;
    [[NSFileManager defaultManager] createDirectoryAtPath:fullPath
                              withIntermediateDirectories:YES
                                               attributes:nil
                                                    error:&error];
    
    if (error) {
        NSLog(@"Error creating temp directory: %@", error.localizedDescription);
        return "";
    }
    
    return [fullPath UTF8String];
}

void TorchToCoreMLConverter::cleanupTempFiles(const std::vector<std::string>& files) {
    for (const auto& file : files) {
        if (fileExists(file)) {
            [[NSFileManager defaultManager] removeItemAtPath:[NSString stringWithUTF8String:file.c_str()]
                                                       error:nil];
        }
    }
}

#pragma mark - C Interface

// C interface functions for integration with FBNeo
extern "C" {

// Initialize converter system
int FBNEO_PyTorch_ToCoreML_Init() {
    // Nothing to initialize in this implementation
    return 0;
}

// Convert PyTorch model to CoreML
int FBNEO_PyTorch_ToCoreML_Convert(const char* torchModelPath,
                                  const char* coreMLOutputPath,
                                  const char* gameType) {
    if (!torchModelPath || !coreMLOutputPath) {
        return 1;
    }
    
    TorchToCoreMLConverter converter;
    bool success = converter.convertModel(torchModelPath, 
                                         coreMLOutputPath, 
                                         gameType ? gameType : "");
    
    return success ? 0 : 1;
}

// Optimize CoreML model
int FBNEO_PyTorch_ToCoreML_Optimize(const char* coreMLModelPath,
                                   const char* outputModelPath,
                                   const char* deviceType) {
    if (!coreMLModelPath || !outputModelPath) {
        return 1;
    }
    
    TorchToCoreMLConverter converter;
    bool success = converter.optimizeModel(coreMLModelPath, 
                                          outputModelPath, 
                                          deviceType ? deviceType : "ALL");
    
    return success ? 0 : 1;
}

// Enhanced conversion with optimizations
int FBNEO_PyTorch_ToCoreML_Enhanced(const char* torchModelPath,
                                   const char* coreMLOutputPath,
                                   const char* gameType,
                                   const char* deviceType) {
    if (!torchModelPath || !coreMLOutputPath) {
        return 1;
    }
    
    TorchToCoreMLConverter converter;
    bool success = converter.enhancedConversion(torchModelPath, 
                                              coreMLOutputPath, 
                                              gameType ? gameType : "",
                                              deviceType ? deviceType : "ALL");
    
    return success ? 0 : 1;
}

// Batch convert multiple PyTorch models to CoreML format
int FBNEO_PyTorch_ToCoreML_BatchConvert(const char** torchModelPaths,
                                       const char** coreMLOutputPaths,
                                       const char** gameTypes,
                                       int count) {
    if (!torchModelPaths || !coreMLOutputPaths || count <= 0) {
        return 1;
    }
    
    TorchToCoreMLConverter converter;
    int successCount = 0;
    
    for (int i = 0; i < count; i++) {
        if (torchModelPaths[i] && coreMLOutputPaths[i]) {
            bool success = converter.convertModel(torchModelPaths[i], 
                                               coreMLOutputPaths[i], 
                                               gameTypes && gameTypes[i] ? gameTypes[i] : "");
            
            if (success) {
                successCount++;
            }
        }
    }
    
    return (successCount == count) ? 0 : 1;
}

// Validate a CoreML model
int FBNEO_PyTorch_ValidateCoreMLModel(const char* modelPath) {
    if (!modelPath) {
        return 1;
    }
    
    TorchToCoreMLConverter converter;
    bool success = converter.validateModel(modelPath);
    
    return success ? 0 : 1;
}

} // extern "C" 