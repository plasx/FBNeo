#import <Foundation/Foundation.h>
#import <CoreML/CoreML.h>
#import <Metal/Metal.h>
#include "quantization_engine.h"

// Implementation of QuantizationEngine
@implementation QuantizationEngine {
    MLModel* _originalModel;
    MLModel* _quantizedModel;
    MLQuantizationSpecification* _quantizationSpec;
    BOOL _isInt4Enabled;
    BOOL _isHybridPrecision;
}

- (instancetype)initWithModelPath:(NSString*)modelPath {
    self = [super init];
    if (self) {
        _isInt4Enabled = NO;
        _isHybridPrecision = NO;
        _quantizationSpec = [[MLQuantizationSpecification alloc] init];
        
        NSError* error = nil;
        NSURL* modelURL = [NSURL fileURLWithPath:modelPath];
        
        // Load the original model
        MLModelConfiguration* config = [[MLModelConfiguration alloc] init];
        config.computeUnits = MLComputeUnitsAll;
        
        _originalModel = [MLModel modelWithContentsOfURL:modelURL
                                          configuration:config
                                                  error:&error];
        
        if (!_originalModel) {
            NSLog(@"Failed to load original model: %@", error);
            return nil;
        }
        
        // Initialize with default quantization configuration
        [self setupDefaultQuantizationSpec];
    }
    return self;
}

- (void)setupDefaultQuantizationSpec {
    // Default to 8-bit quantization
    _quantizationSpec.nbits = 8;
    _quantizationSpec.symmetric = YES;
    _quantizationSpec.perChannelQuantization = YES;
    _quantizationSpec.dynamicRange = MLQuantizationSpecificationDynamicRangeDefault;
    _quantizationSpec.mode = MLQuantizationModeLinear;
    
    // Set up layer-specific precision config (to be used with hybrid precision)
    _quantizationSpec.layerSpecificPrecision = [[MLQuantizationLayerSpecificPrecisionConfig alloc] init];
}

- (BOOL)quantizeModelWithInt4Precision:(BOOL)useInt4
                       hybridPrecision:(BOOL)useHybrid
                            outputPath:(NSString*)outputPath
                                 error:(NSError**)error {
    // Configure quantization specification
    _isInt4Enabled = useInt4;
    _isHybridPrecision = useHybrid;
    
    // Set bits based on precision level
    _quantizationSpec.nbits = useInt4 ? 4 : 8;
    
    // For hybrid precision models, we use different bit-widths for different layers
    if (useHybrid) {
        [self configureHybridPrecision];
    }
    
    // Create compiler with quantization configuration
    MLModelCompiler* compiler = [[MLModelCompiler alloc] init];
    compiler.optimizationLevel = MLCompilerOptimizationLevelMax;
    compiler.useQuantization = YES;
    compiler.quantizationSpecification = _quantizationSpec;
    
    // New in CoreML 5.0: Enable advanced optimizations
    compiler.enableLayerFusion = YES;
    compiler.enableSpecializedKernels = YES;
    compiler.targetDeviceType = MLCompilerDeviceTypeAppleSilicon;
    
    // Compile the quantized model
    MLModel* quantizedModel = [compiler compileModel:_originalModel error:error];
    if (!quantizedModel) {
        NSLog(@"Failed to compile quantized model: %@", *error);
        return NO;
    }
    
    // Save the quantized model
    NSURL* outputURL = [NSURL fileURLWithPath:outputPath];
    BOOL success = [quantizedModel writeToURL:outputURL error:error];
    
    if (success) {
        _quantizedModel = quantizedModel;
        NSLog(@"Successfully quantized model to %@", outputPath);
        
        // Log memory reduction
        [self logMemoryReduction];
    }
    
    return success;
}

- (void)configureHybridPrecision {
    // In hybrid precision mode, we keep some layers at higher precision
    MLQuantizationLayerSpecificPrecisionConfig* layerConfig = _quantizationSpec.layerSpecificPrecision;
    
    // Configure the critical layers to use higher precision (FP16)
    NSMutableDictionary* layerPrecisions = [NSMutableDictionary dictionary];
    
    // Output layers remain at higher precision
    layerPrecisions[@"actions"] = @(MLQuantizationPrecisionFloat16);
    layerPrecisions[@"value"] = @(MLQuantizationPrecisionFloat16);
    
    // First convolutional layer typically needs higher precision
    layerPrecisions[@"conv1"] = @(MLQuantizationPrecisionFloat16);
    
    // Set layer-specific precisions
    layerConfig.layerPrecisions = layerPrecisions;
    
    // Use automatic precision determination for unlisted layers
    layerConfig.defaultPrecision = _isInt4Enabled ? 
        MLQuantizationPrecisionInt4 : MLQuantizationPrecisionInt8;
}

- (void)logMemoryReduction {
    if (!_originalModel || !_quantizedModel) {
        return;
    }
    
    // Get model sizes
    NSNumber* originalSize = [_originalModel.modelDescription.metadata objectForKey:@"com.apple.coreml.model.preview.size"];
    NSNumber* quantizedSize = [_quantizedModel.modelDescription.metadata objectForKey:@"com.apple.coreml.model.preview.size"];
    
    if (originalSize && quantizedSize) {
        double reductionPercent = (1.0 - ([quantizedSize doubleValue] / [originalSize doubleValue])) * 100.0;
        NSLog(@"Memory reduction: %.2f%% (Original: %.2f MB, Quantized: %.2f MB)",
             reductionPercent,
             [originalSize doubleValue] / (1024 * 1024),
             [quantizedSize doubleValue] / (1024 * 1024));
    }
}

- (NSDictionary*)getQuantizationStats {
    if (!_quantizedModel) {
        return @{
            @"status": @"Not quantized",
            @"memory_reduction": @0
        };
    }
    
    // Get model sizes
    NSNumber* originalSize = [_originalModel.modelDescription.metadata objectForKey:@"com.apple.coreml.model.preview.size"];
    NSNumber* quantizedSize = [_quantizedModel.modelDescription.metadata objectForKey:@"com.apple.coreml.model.preview.size"];
    
    double reductionPercent = 0.0;
    if (originalSize && quantizedSize) {
        reductionPercent = (1.0 - ([quantizedSize doubleValue] / [originalSize doubleValue])) * 100.0;
    }
    
    return @{
        @"status": @"Quantized",
        @"precision": _isInt4Enabled ? @"INT4" : @"INT8",
        @"hybrid": _isHybridPrecision ? @"YES" : @"NO",
        @"memory_reduction": @(reductionPercent),
        @"original_size_mb": @([originalSize doubleValue] / (1024 * 1024)),
        @"quantized_size_mb": @([quantizedSize doubleValue] / (1024 * 1024))
    };
}

// Advanced methods for int4 precision
+ (BOOL)supportsInt4Precision {
    // Check if we're on a device that supports int4 precision
    if (@available(macOS 16.0, *)) {
        MLModelConfiguration* config = [[MLModelConfiguration alloc] init];
        return [MLModelConfiguration deviceSupportsInt4Quantization];
    }
    return NO;
}

+ (BOOL)supportsHybridPrecision {
    if (@available(macOS 16.0, *)) {
        return [MLModelConfiguration deviceSupportsHybridPrecision];
    }
    return NO;
}

@end

// C++ wrapper for QuantizationEngine
bool QuantizationEngineWrapper::initWithModel(const std::string& modelPath) {
    NSString* path = [NSString stringWithUTF8String:modelPath.c_str()];
    quantizationEngine = [[QuantizationEngine alloc] initWithModelPath:path];
    return (quantizationEngine != nil);
}

bool QuantizationEngineWrapper::quantizeModel(bool useInt4, bool useHybrid, const std::string& outputPath) {
    if (!quantizationEngine) {
        return false;
    }
    
    NSString* outPath = [NSString stringWithUTF8String:outputPath.c_str()];
    NSError* error = nil;
    bool success = [quantizationEngine quantizeModelWithInt4Precision:useInt4
                                                     hybridPrecision:useHybrid
                                                          outputPath:outPath
                                                               error:&error];
    
    if (!success && error) {
        std::cerr << "Error quantizing model: " << [error.localizedDescription UTF8String] << std::endl;
    }
    
    return success;
}

bool QuantizationEngineWrapper::supportsInt4Precision() {
    return [QuantizationEngine supportsInt4Precision];
}

bool QuantizationEngineWrapper::supportsHybridPrecision() {
    return [QuantizationEngine supportsHybridPrecision];
}

std::map<std::string, std::string> QuantizationEngineWrapper::getQuantizationStats() {
    std::map<std::string, std::string> stats;
    
    if (!quantizationEngine) {
        stats["status"] = "Not initialized";
        return stats;
    }
    
    NSDictionary* objcStats = [quantizationEngine getQuantizationStats];
    for (NSString* key in objcStats) {
        NSString* value = [NSString stringWithFormat:@"%@", [objcStats objectForKey:key]];
        stats[[key UTF8String]] = [value UTF8String];
    }
    
    return stats;
} 