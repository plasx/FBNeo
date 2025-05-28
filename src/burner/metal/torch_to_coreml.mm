#import <Foundation/Foundation.h>
#import <CoreML/CoreML.h>
#include <stdio.h>
#include <string.h>

#include "metal_bridge.h"

// MARK: - PyTorch to CoreML Conversion

/**
 * This file provides bridge functions to convert PyTorch models to CoreML format
 * It will shell out to the Python script that does the actual conversion
 */

// MARK: - C Interface Functions

#ifdef __cplusplus
extern "C" {
#endif

// Initialize the conversion system
int FBNEO_PyTorch_ToCoreML_Init() {
    @autoreleasepool {
        NSLog(@"Initializing PyTorch to CoreML conversion system");
        
        // Check if python3 is available
        NSTask *task = [[NSTask alloc] init];
        [task setLaunchPath:@"/usr/bin/which"];
        [task setArguments:@[@"python3"]];
        
        NSPipe *pipe = [NSPipe pipe];
        [task setStandardOutput:pipe];
        
        NSFileHandle *file = [pipe fileHandleForReading];
        
        @try {
            [task launch];
            [task waitUntilExit];
            
            if ([task terminationStatus] != 0) {
                NSLog(@"Python 3 not found, conversion will not be available");
                return 0;
            }
            
            // Check if coremltools is installed
            task = [[NSTask alloc] init];
            [task setLaunchPath:@"/usr/bin/python3"];
            [task setArguments:@[@"-c", @"import coremltools"]];
            
            [task launch];
            [task waitUntilExit];
            
            if ([task terminationStatus] != 0) {
                NSLog(@"CoreMLTools not installed, trying to install...");
                
                // Try to install coremltools
                task = [[NSTask alloc] init];
                [task setLaunchPath:@"/usr/bin/python3"];
                [task setArguments:@[@"-m", @"pip", @"install", @"--user", @"coremltools"]];
                
                [task launch];
                [task waitUntilExit];
                
                if ([task terminationStatus] != 0) {
                    NSLog(@"Failed to install CoreMLTools");
                    return 0;
                }
            }
            
            return 1;
        }
        @catch (NSException *exception) {
            NSLog(@"Exception during Python check: %@", exception);
            return 0;
        }
    }
}

// Convert a PyTorch model to CoreML format
int FBNEO_PyTorch_ToCoreML_Convert(const char* torchModelPath, const char* coreMLOutputPath, const char* gameType) {
    @autoreleasepool {
        if (!torchModelPath || !coreMLOutputPath) {
            NSLog(@"Invalid parameters for conversion");
            return 0;
        }
        
        NSString *scriptPath = @"src/burner/metal/scripts/torch_to_coreml.py";
        NSString *torchPath = @(torchModelPath);
        NSString *outputPath = @(coreMLOutputPath);
        NSString *game = gameType ? @(gameType) : @"generic";
        
        // Check if the script exists
        NSFileManager *fileManager = [NSFileManager defaultManager];
        if (![fileManager fileExistsAtPath:scriptPath]) {
            NSLog(@"Conversion script not found: %@", scriptPath);
            return 0;
        }
        
        NSLog(@"Converting PyTorch model %@ to CoreML format at %@", torchPath, outputPath);
        
        // Run the conversion script
        NSTask *task = [[NSTask alloc] init];
        [task setLaunchPath:@"/usr/bin/python3"];
        
        NSMutableArray *args = [NSMutableArray arrayWithObjects:
                                scriptPath,
                                @"--input", torchPath,
                                @"--output", outputPath,
                                nil];
        
        if (gameType) {
            [args addObject:@"--game-type"];
            [args addObject:game];
        }
        
        [task setArguments:args];
        
        NSPipe *pipe = [NSPipe pipe];
        [task setStandardOutput:pipe];
        [task setStandardError:pipe];
        
        NSFileHandle *file = [pipe fileHandleForReading];
        
        @try {
            [task launch];
            
            // Read and log output
            NSData *data = [file readDataToEndOfFile];
            NSString *output = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
            NSLog(@"Conversion output:\n%@", output);
            
            [task waitUntilExit];
            
            int status = [task terminationStatus];
            if (status != 0) {
                NSLog(@"Conversion failed with status %d", status);
                return 0;
            }
            
            // Check if the output file was created
            if (![fileManager fileExistsAtPath:outputPath]) {
                NSLog(@"Output file was not created: %@", outputPath);
                return 0;
            }
            
            NSLog(@"Conversion completed successfully");
            return 1;
        }
        @catch (NSException *exception) {
            NSLog(@"Exception during conversion: %@", exception);
            return 0;
        }
    }
}

// Optimize a CoreML model for specific hardware
int FBNEO_PyTorch_ToCoreML_Optimize(const char* coreMLModelPath, const char* outputModelPath, const char* deviceType) {
    @autoreleasepool {
        if (!coreMLModelPath || !outputModelPath) {
            NSLog(@"Invalid parameters for optimization");
            return 0;
        }
        
        NSString *modelPath = @(coreMLModelPath);
        NSString *outputPath = @(outputModelPath);
        NSString *device = deviceType ? @(deviceType) : @"ALL";
        
        NSLog(@"Optimizing CoreML model %@ for device %@", modelPath, device);
        
        // Check if CoreML compiler is available
        NSTask *task = [[NSTask alloc] init];
        [task setLaunchPath:@"/usr/bin/xcrun"];
        [task setArguments:@[@"coremlcompiler", @"--help"]];
        
        NSPipe *pipe = [NSPipe pipe];
        [task setStandardOutput:pipe];
        [task setStandardError:pipe];
        
        @try {
            [task launch];
            [task waitUntilExit];
            
            if ([task terminationStatus] != 0) {
                NSLog(@"CoreML compiler not available, skipping optimization");
                // Just copy the file instead
                NSFileManager *fileManager = [NSFileManager defaultManager];
                NSError *error = nil;
                [fileManager copyItemAtPath:modelPath toPath:outputPath error:&error];
                
                if (error) {
                    NSLog(@"Failed to copy model: %@", error);
                    return 0;
                }
                
                return 1;
            }
            
            // Optimize the model
            task = [[NSTask alloc] init];
            [task setLaunchPath:@"/usr/bin/xcrun"];
            
            NSMutableArray *args = [NSMutableArray arrayWithObjects:
                                    @"coremlcompiler",
                                    @"compile",
                                    modelPath,
                                    outputPath,
                                    nil];
            
            if ([device isEqualToString:@"CPU"]) {
                [args addObject:@"--cpu-only"];
            } else if ([device isEqualToString:@"GPU"]) {
                [args addObject:@"--gpu-only"];
            } else if ([device isEqualToString:@"ANE"]) {
                [args addObject:@"--ane-only"];
            }
            
            [task setArguments:args];
            
            pipe = [NSPipe pipe];
            [task setStandardOutput:pipe];
            [task setStandardError:pipe];
            
            [task launch];
            
            // Read and log output
            NSData *data = [[pipe fileHandleForReading] readDataToEndOfFile];
            NSString *output = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
            NSLog(@"Optimization output:\n%@", output);
            
            [task waitUntilExit];
            
            if ([task terminationStatus] != 0) {
                NSLog(@"Optimization failed");
                return 0;
            }
            
            NSLog(@"Optimization completed successfully");
            return 1;
        }
        @catch (NSException *exception) {
            NSLog(@"Exception during optimization: %@", exception);
            return 0;
        }
    }
}

// Enhanced conversion with optimization in one step
int FBNEO_PyTorch_ToCoreML_Enhanced(const char* torchModelPath, const char* coreMLOutputPath, const char* gameType, const char* deviceType) {
    // First convert the model
    if (!FBNEO_PyTorch_ToCoreML_Convert(torchModelPath, coreMLOutputPath, gameType)) {
        return 0;
    }
    
    // Create a temporary path for optimization
    NSString *tempPath = [NSString stringWithFormat:@"%s.temp", coreMLOutputPath];
    
    // Then optimize it
    if (!FBNEO_PyTorch_ToCoreML_Optimize(coreMLOutputPath, [tempPath UTF8String], deviceType)) {
        return 0;
    }
    
    // Replace the original with the optimized version
    NSFileManager *fileManager = [NSFileManager defaultManager];
    NSError *error = nil;
    
    if ([fileManager fileExistsAtPath:tempPath]) {
        if ([fileManager removeItemAtPath:@(coreMLOutputPath) error:&error]) {
            if (![fileManager moveItemAtPath:tempPath toPath:@(coreMLOutputPath) error:&error]) {
                NSLog(@"Failed to replace model: %@", error);
                return 0;
            }
        } else {
            NSLog(@"Failed to remove original model: %@", error);
            return 0;
        }
    }
    
    return 1;
}

// Validate a CoreML model
int FBNEO_PyTorch_ValidateCoreMLModel(const char* modelPath) {
    @autoreleasepool {
        if (!modelPath) {
            NSLog(@"Invalid model path for validation");
            return 0;
        }
        
        NSLog(@"Validating CoreML model: %s", modelPath);
        
        NSError *error = nil;
        NSURL *modelURL = [NSURL fileURLWithPath:@(modelPath)];
        
        // Try to load the model to validate it
        MLModel *model = [MLModel modelWithContentsOfURL:modelURL error:&error];
        if (!model || error) {
            NSLog(@"Failed to load model: %@", error);
            return 0;
        }
        
        // Validate the model inputs and outputs
        MLModelDescription *description = model.modelDescription;
        if (description.inputDescriptionsByName.count == 0) {
            NSLog(@"Model has no inputs");
            return 0;
        }
        
        if (description.outputDescriptionsByName.count == 0) {
            NSLog(@"Model has no outputs");
            return 0;
        }
        
        // Find image input
        BOOL hasImageInput = NO;
        for (NSString *inputName in description.inputDescriptionsByName) {
            MLFeatureDescription *desc = description.inputDescriptionsByName[inputName];
            if (desc.type == MLFeatureTypeImage || 
                (desc.type == MLFeatureTypeMultiArray && 
                 [desc valueForKey:@"multiArrayConstraint"] != nil)) {
                hasImageInput = YES;
                break;
            }
        }
        
        if (!hasImageInput) {
            NSLog(@"Model does not have an image input");
            return 0;
        }
        
        NSLog(@"Model validation successful");
        return 1;
    }
}

#ifdef __cplusplus
}
#endif 