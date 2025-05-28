#import <Foundation/Foundation.h>
#import <CoreML/CoreML.h>
#import <Metal/Metal.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h>

#include "ai_definitions.h"
#include "ai_interface.h"

// Forward declarations
@class FBNeoCoreMLManager;

// Model Container Class - Secure storage for model data
@interface FBNeoAIModelContainer : NSObject

// Initialize with file path
- (instancetype)initWithPath:(NSString *)path error:(NSError **)error;

// Initialize with data
- (instancetype)initWithData:(NSData *)data error:(NSError **)error;

// Get file path
- (NSString *)path;

// Get model data
- (NSData *)data;

// Check if data is valid
- (BOOL)isValid;

// Get model type
- (NSString *)modelType;

// Get model metadata
- (NSDictionary *)metadata;

// Model verification
- (BOOL)verifySignature;

@end

// Model Loader Class
@interface FBNeoModelLoader : NSObject

// Singleton access
+ (instancetype)sharedLoader;

// Load a model from path
- (BOOL)loadModelFromPath:(NSString *)path error:(NSError **)error;

// Convert PyTorch model to CoreML
- (BOOL)convertPyTorchModelAtPath:(NSString *)path 
                       toCoreMLAtPath:(NSString *)outputPath 
                                 error:(NSError **)error;

// Check if model is supported
- (BOOL)isModelSupported:(NSString *)path;

// Get model information
- (NSDictionary *)getModelInfo:(NSString *)path;

// Get all available models in default locations
- (NSArray<NSString *> *)availableModels;

// Get currently loaded model
- (NSString *)currentModelPath;

// Create a model container
- (FBNeoAIModelContainer *)createModelContainerWithPath:(NSString *)path error:(NSError **)error;

@end

// Model Container implementation
@implementation FBNeoAIModelContainer {
    NSString *_path;
    NSData *_data;
    NSString *_modelType;
    NSDictionary *_metadata;
    BOOL _isValid;
}

- (instancetype)initWithPath:(NSString *)path error:(NSError **)error {
    if (self = [super init]) {
        _path = [path copy];
        _isValid = NO;
        
        // Load data from file
        _data = [NSData dataWithContentsOfFile:path options:NSDataReadingMappedIfSafe error:error];
        if (!_data) {
            NSLog(@"FBNeoAIModelContainer: Failed to load data from %@", path);
            return self;
        }
        
        // Detect model type and validate
        [self detectModelType];
        [self extractMetadata];
        _isValid = [self validateModel];
        
        NSLog(@"FBNeoAIModelContainer: Loaded model from %@, type: %@, valid: %d", 
              path, _modelType, _isValid);
    }
    return self;
}

- (instancetype)initWithData:(NSData *)data error:(NSError **)error {
    if (self = [super init]) {
        _data = [data copy];
        _path = nil;
        _isValid = NO;
        
        // Detect model type and validate
        [self detectModelType];
        [self extractMetadata];
        _isValid = [self validateModel];
        
        NSLog(@"FBNeoAIModelContainer: Loaded model from data, type: %@, valid: %d", 
              _modelType, _isValid);
    }
    return self;
}

- (void)detectModelType {
    // Simple detection based on file extension and header bytes
    if (_path) {
        NSString *extension = [_path pathExtension];
        if ([extension isEqualToString:@"mlmodel"] || [extension isEqualToString:@"mlpackage"]) {
            _modelType = @"CoreML";
        } else if ([extension isEqualToString:@"pt"] || [extension isEqualToString:@"pth"]) {
            _modelType = @"PyTorch";
        } else if ([extension isEqualToString:@"onnx"]) {
            _modelType = @"ONNX";
        } else if ([extension isEqualToString:@"tflite"]) {
            _modelType = @"TensorFlowLite";
        } else {
            _modelType = @"Unknown";
        }
    } else {
        // Try to determine from data - simplified for now
        // In a real implementation, this would examine header bytes
        _modelType = @"Unknown";
    }
}

- (void)extractMetadata {
    _metadata = @{
        @"type": _modelType ?: @"Unknown",
        @"path": _path ?: @"<memory>",
        @"size": @(_data.length)
    };
    
    // In a real implementation, we would extract more detailed metadata
    // based on the model type
}

- (BOOL)validateModel {
    // Basic validation
    if (!_data || _data.length == 0) {
        return NO;
    }
    
    // Model-specific validation
    if ([_modelType isEqualToString:@"CoreML"]) {
        // For CoreML, we can try to compile the model to validate it
        // This is a simplified validation
        return _data.length > 1024; // Just check if it's not too small
    } else if ([_modelType isEqualToString:@"PyTorch"]) {
        // For PyTorch, check for magic numbers or specific headers
        return _data.length > 1024; // Simplified check
    } else if ([_modelType isEqualToString:@"ONNX"]) {
        // For ONNX, check for ONNX magic number
        return _data.length > 1024; // Simplified check
    }
    
    return NO;
}

- (NSString *)path {
    return _path;
}

- (NSData *)data {
    return _data;
}

- (BOOL)isValid {
    return _isValid;
}

- (NSString *)modelType {
    return _modelType;
}

- (NSDictionary *)metadata {
    return _metadata;
}

- (BOOL)verifySignature {
    // In a real implementation, this would check code signing or
    // other authentication mechanisms
    // For now, this is just a placeholder
    return _isValid;
}

@end

// Model Loader implementation
@implementation FBNeoModelLoader {
    NSString *_currentModelPath;
    FBNeoAIModelContainer *_currentModelContainer;
    NSArray<NSString *> *_modelSearchPaths;
}

+ (instancetype)sharedLoader {
    static FBNeoModelLoader *sharedInstance = nil;
    static dispatch_once_t onceToken;
    
    dispatch_once(&onceToken, ^{
        sharedInstance = [[self alloc] init];
    });
    
    return sharedInstance;
}

- (instancetype)init {
    if (self = [super init]) {
        // Setup default model search paths
        NSString *userModelsDir = [NSHomeDirectory() stringByAppendingPathComponent:@"Documents/FBNeo/models"];
        NSString *appModelsDir = [[NSBundle mainBundle] pathForResource:@"models" ofType:nil];
        
        _modelSearchPaths = @[
            userModelsDir,
            appModelsDir ?: @"",
            @"/Users/Shared/FBNeo/models"
        ];
        
        NSLog(@"FBNeoModelLoader: Initialized with search paths: %@", _modelSearchPaths);
    }
    return self;
}

- (BOOL)loadModelFromPath:(NSString *)path error:(NSError **)error {
    // Create a model container
    FBNeoAIModelContainer *container = [self createModelContainerWithPath:path error:error];
    if (!container || !container.isValid) {
        NSLog(@"FBNeoModelLoader: Failed to create model container for %@", path);
        return NO;
    }
    
    _currentModelContainer = container;
    _currentModelPath = path;
    
    // Handle model loading based on type
    if ([container.modelType isEqualToString:@"CoreML"]) {
        // CoreML models can be loaded directly by the CoreML manager
        return YES;
    } else if ([container.modelType isEqualToString:@"PyTorch"]) {
        // For PyTorch models, we need to convert to CoreML or process differently
        NSLog(@"FBNeoModelLoader: PyTorch model loaded, but requires conversion for CoreML use");
        return YES;
    } else if ([container.modelType isEqualToString:@"ONNX"]) {
        // For ONNX models, we need to handle differently
        NSLog(@"FBNeoModelLoader: ONNX model loaded, but requires conversion for CoreML use");
        return YES;
    }
    
    // Unsupported model type
    if (error) {
        *error = [NSError errorWithDomain:@"com.fbneo.modelloader" 
                                     code:400 
                                 userInfo:@{NSLocalizedDescriptionKey: 
                                             [NSString stringWithFormat:@"Unsupported model type: %@", container.modelType]}];
    }
    
    return NO;
}

- (FBNeoAIModelContainer *)createModelContainerWithPath:(NSString *)path error:(NSError **)error {
    return [[FBNeoAIModelContainer alloc] initWithPath:path error:error];
}

- (BOOL)convertPyTorchModelAtPath:(NSString *)path 
                   toCoreMLAtPath:(NSString *)outputPath 
                             error:(NSError **)error {
    // This is a placeholder for the actual conversion logic
    // In a real implementation, this would use PyTorch -> CoreML conversion tools
    
    NSLog(@"FBNeoModelLoader: PyTorch to CoreML conversion not fully implemented");
    
    if (error) {
        *error = [NSError errorWithDomain:@"com.fbneo.modelloader" 
                                     code:501 
                                 userInfo:@{NSLocalizedDescriptionKey: @"PyTorch to CoreML conversion not implemented"}];
    }
    
    return NO;
}

- (BOOL)isModelSupported:(NSString *)path {
    NSError *error = nil;
    FBNeoAIModelContainer *container = [self createModelContainerWithPath:path error:&error];
    
    if (error || !container) {
        return NO;
    }
    
    NSString *modelType = container.modelType;
    return [modelType isEqualToString:@"CoreML"] || 
           [modelType isEqualToString:@"PyTorch"] || 
           [modelType isEqualToString:@"ONNX"];
}

- (NSDictionary *)getModelInfo:(NSString *)path {
    NSError *error = nil;
    FBNeoAIModelContainer *container = [self createModelContainerWithPath:path error:&error];
    
    if (error || !container) {
        return @{
            @"status": @"error",
            @"error": error ? [error localizedDescription] : @"Unknown error",
            @"path": path ?: @"<unknown>"
        };
    }
    
    NSMutableDictionary *info = [NSMutableDictionary dictionaryWithDictionary:container.metadata];
    info[@"status"] = container.isValid ? @"valid" : @"invalid";
    info[@"isSupported"] = @([self isModelSupported:path]);
    
    return info;
}

- (NSArray<NSString *> *)availableModels {
    NSMutableArray<NSString *> *modelPaths = [NSMutableArray array];
    NSFileManager *fileManager = [NSFileManager defaultManager];
    
    // Supported extensions
    NSArray<NSString *> *supportedExtensions = @[@"mlmodel", @"mlpackage", @"pt", @"pth", @"onnx", @"tflite"];
    
    // Search in all model paths
    for (NSString *dirPath in _modelSearchPaths) {
        if (![fileManager fileExistsAtPath:dirPath]) {
            continue;
        }
        
        NSError *error = nil;
        NSArray<NSString *> *contents = [fileManager contentsOfDirectoryAtPath:dirPath error:&error];
        
        if (error) {
            NSLog(@"FBNeoModelLoader: Error reading directory %@: %@", dirPath, error);
            continue;
        }
        
        for (NSString *item in contents) {
            NSString *extension = [item pathExtension];
            if ([supportedExtensions containsObject:extension]) {
                NSString *fullPath = [dirPath stringByAppendingPathComponent:item];
                [modelPaths addObject:fullPath];
            }
        }
    }
    
    return modelPaths;
}

- (NSString *)currentModelPath {
    return _currentModelPath;
}

@end

// C interface functions for FBNeoModelLoader
extern "C" {

void* FBNeo_ModelLoader_Initialize() {
    // Return the shared instance pointer (as an opaque pointer)
    return (void*)CFBridgingRetain([FBNeoModelLoader sharedLoader]);
}

int FBNeo_ModelLoader_LoadModel(void* loader, const char* path) {
    @autoreleasepool {
        FBNeoModelLoader *modelLoader = (__bridge FBNeoModelLoader*)loader;
        NSString *modelPath = [NSString stringWithUTF8String:path];
        NSError *error = nil;
        
        BOOL success = [modelLoader loadModelFromPath:modelPath error:&error];
        
        if (!success && error) {
            NSLog(@"FBNeo_ModelLoader_LoadModel failed: %@", error);
        }
        
        return success ? 1 : 0;
    }
}

int FBNeo_ModelLoader_IsModelSupported(void* loader, const char* path) {
    @autoreleasepool {
        FBNeoModelLoader *modelLoader = (__bridge FBNeoModelLoader*)loader;
        NSString *modelPath = [NSString stringWithUTF8String:path];
        
        return [modelLoader isModelSupported:modelPath] ? 1 : 0;
    }
}

char** FBNeo_ModelLoader_GetAvailableModels(void* loader, int* count) {
    @autoreleasepool {
        FBNeoModelLoader *modelLoader = (__bridge FBNeoModelLoader*)loader;
        NSArray<NSString *> *models = [modelLoader availableModels];
        
        *count = (int)models.count;
        if (*count == 0) {
            return NULL;
        }
        
        // Allocate memory for string array
        char** result = (char**)malloc(sizeof(char*) * *count);
        if (!result) {
            *count = 0;
            return NULL;
        }
        
        // Copy each string
        for (int i = 0; i < *count; i++) {
            const char* str = [models[i] UTF8String];
            size_t len = strlen(str) + 1;
            result[i] = (char*)malloc(len);
            if (result[i]) {
                strcpy(result[i], str);
            }
        }
        
        return result;
    }
}

void FBNeo_ModelLoader_FreeModelList(char** list, int count) {
    if (!list) return;
    
    for (int i = 0; i < count; i++) {
        if (list[i]) {
            free(list[i]);
        }
    }
    
    free(list);
}

const char* FBNeo_ModelLoader_GetCurrentModelPath(void* loader) {
    @autoreleasepool {
        FBNeoModelLoader *modelLoader = (__bridge FBNeoModelLoader*)loader;
        NSString *path = [modelLoader currentModelPath];
        
        if (!path) {
            return NULL;
        }
        
        // This is a simplified implementation - in a real app, you should 
        // manage string lifetime more carefully
        return strdup([path UTF8String]);
    }
}

void FBNeo_ModelLoader_FreeString(char* str) {
    if (str) {
        free(str);
    }
}

void FBNeo_ModelLoader_Release(void* loader) {
    if (loader) {
        CFRelease(loader);
    }
}

} // extern "C" 