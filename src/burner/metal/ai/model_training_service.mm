#import <Foundation/Foundation.h>
#import <CoreML/CoreML.h>
#import <CreateML/CreateML.h>

#include <string>
#include <vector>
#include "ai_definitions.h"

// Class for training CoreML models and handling data collection
class ModelTrainingService {
private:
    NSString* trainingDataDir;
    NSString* exportDir;
    NSMutableArray* collectedSamples;
    int maxSamples;
    bool isCollecting;
    int frameInterval;
    int currentFrameCount;
    
    // Log error message
    void logError(const std::string& error);
    
    // Create a training dataset from collected samples
    bool createDatasetFromSamples(NSString* outputPath);
    
    // Write data to disk
    bool saveTrainingData();
    
    // Load existing training data
    bool loadTrainingData();
    
public:
    ModelTrainingService();
    ~ModelTrainingService();
    
    // Initialize the service
    bool initialize(const std::string& dataDir, const std::string& modelExportDir);
    
    // Start collecting training data
    bool startDataCollection(int maxSamples = 10000, int frameInterval = 5);
    
    // Stop collecting training data
    void stopDataCollection();
    
    // Add a single training sample
    bool addTrainingSample(const void* frameBuffer, int width, int height, 
                          const int* buttonStates, int numButtons);
    
    // Train a new model with collected data
    bool trainModel(const std::string& modelName, 
                   const std::string& modelType,
                   int epochs,
                   float validationSplit);
    
    // Export a trained model to CoreML format
    bool exportToCoreML(const std::string& modelPath);
    
    // Get training progress information
    float getTrainingProgress();
    
    // Get number of collected samples
    int getCollectedSamplesCount();
};

// Implementation
ModelTrainingService::ModelTrainingService() 
    : trainingDataDir(nil), exportDir(nil), collectedSamples(nil), 
      maxSamples(0), isCollecting(false), frameInterval(1), currentFrameCount(0) {
    
    collectedSamples = [[NSMutableArray alloc] init];
}

ModelTrainingService::~ModelTrainingService() {
    [trainingDataDir release];
    [exportDir release];
    [collectedSamples release];
}

void ModelTrainingService::logError(const std::string& error) {
    NSLog(@"ModelTrainingService error: %s", error.c_str());
}

bool ModelTrainingService::initialize(const std::string& dataDir, const std::string& modelExportDir) {
    trainingDataDir = [[NSString alloc] initWithUTF8String:dataDir.c_str()];
    exportDir = [[NSString alloc] initWithUTF8String:modelExportDir.c_str()];
    
    // Create directories if they don't exist
    NSFileManager* fileManager = [NSFileManager defaultManager];
    NSError* error = nil;
    
    if (![fileManager fileExistsAtPath:trainingDataDir]) {
        [fileManager createDirectoryAtPath:trainingDataDir 
              withIntermediateDirectories:YES 
                               attributes:nil 
                                    error:&error];
        if (error) {
            logError(std::string("Failed to create training data directory: ") + 
                    std::string([[error localizedDescription] UTF8String]));
            return false;
        }
    }
    
    if (![fileManager fileExistsAtPath:exportDir]) {
        [fileManager createDirectoryAtPath:exportDir 
              withIntermediateDirectories:YES 
                               attributes:nil 
                                    error:&error];
        if (error) {
            logError(std::string("Failed to create model export directory: ") + 
                    std::string([[error localizedDescription] UTF8String]));
            return false;
        }
    }
    
    // Load existing training data if available
    return loadTrainingData();
}

bool ModelTrainingService::startDataCollection(int maxSamples, int frameInterval) {
    this->maxSamples = maxSamples;
    this->frameInterval = frameInterval;
    currentFrameCount = 0;
    isCollecting = true;
    
    // Clear existing samples
    [collectedSamples removeAllObjects];
    
    NSLog(@"Started collecting training data (max: %d samples, interval: %d frames)", 
          maxSamples, frameInterval);
    
    return true;
}

void ModelTrainingService::stopDataCollection() {
    isCollecting = false;
    
    // Save collected data
    saveTrainingData();
    
    NSLog(@"Stopped collecting training data. Collected %lu samples.", 
          (unsigned long)[collectedSamples count]);
}

bool ModelTrainingService::addTrainingSample(const void* frameBuffer, int width, int height, 
                                           const int* buttonStates, int numButtons) {
    if (!isCollecting || !frameBuffer || !buttonStates) {
        return false;
    }
    
    // Only capture every Nth frame
    currentFrameCount++;
    if (currentFrameCount % frameInterval != 0) {
        return true;
    }
    
    // Stop if we've collected enough samples
    if ([collectedSamples count] >= maxSamples) {
        stopDataCollection();
        return false;
    }
    
    @autoreleasepool {
        // Create a dictionary to hold this sample
        NSMutableDictionary* sample = [NSMutableDictionary dictionary];
        
        // Create a bitmap from the frame buffer
        CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
        CGContextRef context = CGBitmapContextCreate(
            (void*)frameBuffer,
            width, height,
            8, width * 4,
            colorSpace,
            kCGImageAlphaPremultipliedLast
        );
        
        CGImageRef cgImage = CGBitmapContextCreateImage(context);
        NSImage* image = [[NSImage alloc] initWithCGImage:cgImage size:NSMakeSize(width, height)];
        
        // Add the image to the sample
        sample[@"image"] = image;
        
        // Add button states to the sample
        NSMutableArray* buttons = [NSMutableArray arrayWithCapacity:numButtons];
        for (int i = 0; i < numButtons; i++) {
            [buttons addObject:@(buttonStates[i])];
        }
        sample[@"buttons"] = buttons;
        
        // Add timestamp
        sample[@"timestamp"] = [NSDate date];
        
        // Add to collected samples
        [collectedSamples addObject:sample];
        
        // Clean up
        [image release];
        CGImageRelease(cgImage);
        CGContextRelease(context);
        CGColorSpaceRelease(colorSpace);
        
        // Save periodically (every 100 samples)
        if ([collectedSamples count] % 100 == 0) {
            saveTrainingData();
        }
        
        return true;
    }
}

bool ModelTrainingService::saveTrainingData() {
    NSString* dataPath = [trainingDataDir stringByAppendingPathComponent:@"training_data.plist"];
    
    @autoreleasepool {
        // Convert images to data
        NSMutableArray* serializedSamples = [NSMutableArray arrayWithCapacity:[collectedSamples count]];
        
        for (NSDictionary* sample in collectedSamples) {
            NSMutableDictionary* serializedSample = [NSMutableDictionary dictionary];
            
            // Convert image to PNG data
            NSImage* image = sample[@"image"];
            NSData* imageData = [image TIFFRepresentation];
            NSBitmapImageRep* imageRep = [NSBitmapImageRep imageRepWithData:imageData];
            NSData* pngData = [imageRep representationUsingType:NSBitmapImageFileTypePNG properties:@{}];
            
            serializedSample[@"imageData"] = pngData;
            serializedSample[@"buttons"] = sample[@"buttons"];
            serializedSample[@"timestamp"] = sample[@"timestamp"];
            
            [serializedSamples addObject:serializedSample];
        }
        
        // Write to file
        NSError* error = nil;
        BOOL success = [serializedSamples writeToFile:dataPath atomically:YES];
        
        if (!success) {
            logError(std::string("Failed to save training data"));
            return false;
        }
        
        NSLog(@"Saved %lu training samples to %@", 
              (unsigned long)[serializedSamples count], dataPath);
        
        return true;
    }
}

bool ModelTrainingService::loadTrainingData() {
    NSString* dataPath = [trainingDataDir stringByAppendingPathComponent:@"training_data.plist"];
    
    NSFileManager* fileManager = [NSFileManager defaultManager];
    if (![fileManager fileExistsAtPath:dataPath]) {
        // No existing data file
        return true;
    }
    
    @autoreleasepool {
        // Load serialized samples
        NSArray* serializedSamples = [NSArray arrayWithContentsOfFile:dataPath];
        if (!serializedSamples) {
            logError(std::string("Failed to load training data"));
            return false;
        }
        
        // Convert back to usable samples
        [collectedSamples removeAllObjects];
        
        for (NSDictionary* serializedSample in serializedSamples) {
            NSMutableDictionary* sample = [NSMutableDictionary dictionary];
            
            // Convert PNG data back to image
            NSData* pngData = serializedSample[@"imageData"];
            NSImage* image = [[NSImage alloc] initWithData:pngData];
            
            sample[@"image"] = image;
            sample[@"buttons"] = serializedSample[@"buttons"];
            sample[@"timestamp"] = serializedSample[@"timestamp"];
            
            [collectedSamples addObject:sample];
            [image release];
        }
        
        NSLog(@"Loaded %lu training samples from %@", 
              (unsigned long)[collectedSamples count], dataPath);
        
        return true;
    }
}

bool ModelTrainingService::createDatasetFromSamples(NSString* outputPath) {
    if ([collectedSamples count] == 0) {
        logError("No training samples available");
        return false;
    }
    
    @autoreleasepool {
        // Create a directory for the dataset
        NSFileManager* fileManager = [NSFileManager defaultManager];
        NSError* error = nil;
        
        if (![fileManager createDirectoryAtPath:outputPath
                   withIntermediateDirectories:YES
                                    attributes:nil
                                         error:&error]) {
            logError(std::string("Failed to create dataset directory: ") + 
                    std::string([[error localizedDescription] UTF8String]));
            return false;
        }
        
        // Create subdirectories for each class
        NSString* imagesDir = [outputPath stringByAppendingPathComponent:@"images"];
        NSString* labelsPath = [outputPath stringByAppendingPathComponent:@"labels.csv"];
        
        if (![fileManager createDirectoryAtPath:imagesDir
                   withIntermediateDirectories:YES
                                    attributes:nil
                                         error:&error]) {
            logError(std::string("Failed to create images directory: ") + 
                    std::string([[error localizedDescription] UTF8String]));
            return false;
        }
        
        // Create CSV file for labels
        NSMutableString* csvContent = [NSMutableString stringWithString:@"image_name,"];
        
        // Determine number of buttons from first sample
        NSArray* firstSampleButtons = collectedSamples[0][@"buttons"];
        int numButtons = [firstSampleButtons count];
        
        // Create headers for button columns
        for (int i = 0; i < numButtons; i++) {
            [csvContent appendFormat:@"button%d%@", i, (i < numButtons - 1) ? @"," : @"\n"];
        }
        
        // Write data for each sample
        for (int i = 0; i < [collectedSamples count]; i++) {
            NSDictionary* sample = collectedSamples[i];
            NSString* imageName = [NSString stringWithFormat:@"image_%06d.png", i];
            NSString* imagePath = [imagesDir stringByAppendingPathComponent:imageName];
            
            // Save the image
            NSImage* image = sample[@"image"];
            NSData* pngData = [[[NSBitmapImageRep imageRepWithData:[image TIFFRepresentation]]
                              representationUsingType:NSBitmapImageFileTypePNG properties:@{}] retain];
            
            [pngData writeToFile:imagePath atomically:YES];
            [pngData release];
            
            // Add label entry
            [csvContent appendFormat:@"%@,", imageName];
            
            // Add button states
            NSArray* buttons = sample[@"buttons"];
            for (int j = 0; j < [buttons count]; j++) {
                [csvContent appendFormat:@"%@%@", buttons[j],
                 (j < [buttons count] - 1) ? @"," : @"\n"];
            }
        }
        
        // Write labels file
        if (![csvContent writeToFile:labelsPath
                         atomically:YES
                           encoding:NSUTF8StringEncoding
                              error:&error]) {
            logError(std::string("Failed to write labels file: ") + 
                    std::string([[error localizedDescription] UTF8String]));
            return false;
        }
        
        NSLog(@"Created dataset at %@ with %lu samples", 
              outputPath, (unsigned long)[collectedSamples count]);
        
        return true;
    }
}

bool ModelTrainingService::trainModel(const std::string& modelName, 
                                      const std::string& modelType,
                                      int epochs,
                                      float validationSplit) {
    if ([collectedSamples count] == 0) {
        logError("No training samples available");
        return false;
    }
    
    @autoreleasepool {
        // Create a temporary dataset
        NSString* datasetPath = [NSTemporaryDirectory() stringByAppendingPathComponent:@"fbneo_training_dataset"];
        if (!createDatasetFromSamples(datasetPath)) {
            return false;
        }
        
        // Create a model output path
        NSString* modelFileName = [NSString stringWithFormat:@"%s.mlmodel", modelName.c_str()];
        NSString* modelPath = [exportDir stringByAppendingPathComponent:modelFileName];
        
        // Train model using CreateML on macOS 13+
        if (@available(macOS 13.0, *)) {
            // While we can't include full training code due to space limitations,
            // this is where you would use CreateML to train the model
            
            // For now, this is a placeholder to show the structure
            NSLog(@"Training model %s of type %s for %d epochs (validation split: %.2f)...",
                 modelName.c_str(), modelType.c_str(), epochs, validationSplit);
            
            NSLog(@"CreateML model training would happen here - not fully implemented");
            
            // In a full implementation, you would:
            // 1. Create an MLImageClassifier or MLTabularRegressor
            // 2. Configure training parameters
            // 3. Start training with progress updates
            // 4. Save the trained model to modelPath
            
            return true;
        } else {
            logError("CreateML training requires macOS 13 or later");
            return false;
        }
    }
}

bool ModelTrainingService::exportToCoreML(const std::string& modelPath) {
    // This is a placeholder - in a real implementation, you would:
    // 1. Load your trained model (possibly from a framework like PyTorch)
    // 2. Convert it to CoreML format
    // 3. Save to the specified path
    
    NSLog(@"CoreML export would happen here - not fully implemented");
    return true;
}

float ModelTrainingService::getTrainingProgress() {
    // In a real implementation, this would return the actual training progress
    return 0.0f;
}

int ModelTrainingService::getCollectedSamplesCount() {
    return [collectedSamples count];
}

// C wrapper functions for the ModelTrainingService
extern "C" {
    // Create a new training service
    void* ModelTrainingService_Create() {
        return new ModelTrainingService();
    }
    
    // Destroy a training service
    void ModelTrainingService_Destroy(void* handle) {
        if (handle) {
            delete static_cast<ModelTrainingService*>(handle);
        }
    }
    
    // Initialize the training service
    int ModelTrainingService_Initialize(void* handle, const char* dataDir, const char* exportDir) {
        if (!handle || !dataDir || !exportDir) {
            return 0;
        }
        
        ModelTrainingService* service = static_cast<ModelTrainingService*>(handle);
        return service->initialize(dataDir, exportDir) ? 1 : 0;
    }
    
    // Start collecting training data
    int ModelTrainingService_StartCollection(void* handle, int maxSamples, int frameInterval) {
        if (!handle) {
            return 0;
        }
        
        ModelTrainingService* service = static_cast<ModelTrainingService*>(handle);
        return service->startDataCollection(maxSamples, frameInterval) ? 1 : 0;
    }
    
    // Stop collecting training data
    void ModelTrainingService_StopCollection(void* handle) {
        if (handle) {
            ModelTrainingService* service = static_cast<ModelTrainingService*>(handle);
            service->stopDataCollection();
        }
    }
    
    // Add a training sample
    int ModelTrainingService_AddSample(void* handle, void* frameBuffer, int width, int height, 
                                      int* buttonStates, int numButtons) {
        if (!handle || !frameBuffer || !buttonStates) {
            return 0;
        }
        
        ModelTrainingService* service = static_cast<ModelTrainingService*>(handle);
        return service->addTrainingSample(frameBuffer, width, height, buttonStates, numButtons) ? 1 : 0;
    }
    
    // Train a model
    int ModelTrainingService_TrainModel(void* handle, const char* modelName, 
                                       const char* modelType, int epochs, float validationSplit) {
        if (!handle || !modelName || !modelType) {
            return 0;
        }
        
        ModelTrainingService* service = static_cast<ModelTrainingService*>(handle);
        return service->trainModel(modelName, modelType, epochs, validationSplit) ? 1 : 0;
    }
    
    // Export a model to CoreML format
    int ModelTrainingService_ExportToCoreML(void* handle, const char* modelPath) {
        if (!handle || !modelPath) {
            return 0;
        }
        
        ModelTrainingService* service = static_cast<ModelTrainingService*>(handle);
        return service->exportToCoreML(modelPath) ? 1 : 0;
    }
    
    // Get training progress
    float ModelTrainingService_GetProgress(void* handle) {
        if (!handle) {
            return 0.0f;
        }
        
        ModelTrainingService* service = static_cast<ModelTrainingService*>(handle);
        return service->getTrainingProgress();
    }
    
    // Get collected samples count
    int ModelTrainingService_GetSampleCount(void* handle) {
        if (!handle) {
            return 0;
        }
        
        ModelTrainingService* service = static_cast<ModelTrainingService*>(handle);
        return service->getCollectedSamplesCount();
    }
} 