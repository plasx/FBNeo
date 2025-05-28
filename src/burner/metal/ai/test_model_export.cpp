/**
 * @brief Test program for model export and optimization functionality
 */
#include <iostream>
#include <string>
#include <vector>
#include "ai_torch_policy.h"
#include "pytorch_to_coreml.h"
#include "model_optimization.h"

using namespace fbneo::ai;

// Helper function to print test result
void printTestResult(const std::string& testName, bool success) {
    std::cout << "[" << (success ? "PASS" : "FAIL") << "] " << testName << std::endl;
}

// Test basic PyTorch to CoreML conversion
bool testBasicConversion() {
    std::cout << "\n=== Testing Basic PyTorch to CoreML Conversion ===" << std::endl;
    
    // Create a test PyTorch model (simulated)
    std::string torchModelPath = "test_model.pt";
    std::ofstream torchFile(torchModelPath);
    torchFile << "Test PyTorch model - this is a stub for testing" << std::endl;
    torchFile.close();
    
    // Define input shape
    std::vector<int> inputShape = {1, 3, 128, 128}; // Batch, channels, height, width
    
    // Define output path
    std::string coremlPath = "test_model.mlmodel";
    
    // Try conversion
    std::cout << "Converting " << torchModelPath << " to " << coremlPath << "..." << std::endl;
    bool success = convertPyTorchToCoreML(torchModelPath, coremlPath, inputShape, true, true);
    
    // Clean up
    std::remove(torchModelPath.c_str());
    if (success) {
        std::remove(coremlPath.c_str());
    }
    
    return success;
}

// Test enhanced conversion with additional options
bool testEnhancedConversion() {
    std::cout << "\n=== Testing Enhanced PyTorch to CoreML Conversion ===" << std::endl;
    
    // Create a test PyTorch model (simulated)
    std::string torchModelPath = "test_enhanced_model.pt";
    std::ofstream torchFile(torchModelPath);
    torchFile << "Test PyTorch model - this is a stub for testing enhanced conversion" << std::endl;
    torchFile.close();
    
    // Define input and output shapes
    std::vector<int> inputShape = {1, 3, 224, 224}; // Batch, channels, height, width
    std::vector<int> outputShape = {1, 10}; // Batch, number of classes
    
    // Define output path
    std::string coremlPath = "test_enhanced_model.mlmodel";
    
    // Try enhanced conversion
    std::cout << "Converting with enhanced options..." << std::endl;
    bool success = enhancedPyTorchToCoreML(
        torchModelPath, 
        coremlPath, 
        inputShape, 
        outputShape, 
        true,   // Use Neural Engine
        16      // Use FP16 quantization
    );
    
    // Clean up
    std::remove(torchModelPath.c_str());
    if (success) {
        std::remove(coremlPath.c_str());
    }
    
    return success;
}

// Test model optimization
bool testModelOptimization() {
    std::cout << "\n=== Testing Model Optimization ===" << std::endl;
    
    // Create a test CoreML model (simulated)
    std::string modelPath = "test_optimization_model.mlmodel";
    std::ofstream modelFile(modelPath);
    modelFile << "Test CoreML model - this is a stub for testing optimization" << std::endl;
    modelFile.close();
    
    // Define output path
    std::string optimizedPath = "test_optimization_model_optimized.mlmodel";
    
    // Test optimization for speed
    std::cout << "Testing optimization for speed..." << std::endl;
    bool speedSuccess = optimizeModelForSpeed(modelPath, optimizedPath);
    printTestResult("Optimize for Speed", speedSuccess);
    
    if (speedSuccess) {
        std::remove(optimizedPath.c_str());
    }
    
    // Test optimization for size
    std::cout << "Testing optimization for size..." << std::endl;
    bool sizeSuccess = optimizeModelForSize(modelPath, optimizedPath);
    printTestResult("Optimize for Size", sizeSuccess);
    
    if (sizeSuccess) {
        std::remove(optimizedPath.c_str());
    }
    
    // Test optimization for accuracy
    std::cout << "Testing optimization for accuracy..." << std::endl;
    bool accuracySuccess = optimizeModelForAccuracy(modelPath, optimizedPath);
    printTestResult("Optimize for Accuracy", accuracySuccess);
    
    if (accuracySuccess) {
        std::remove(optimizedPath.c_str());
    }
    
    // Test custom optimization
    std::cout << "Testing custom optimization..." << std::endl;
    
    ModelOptimizer optimizer;
    OptimizationConfig config;
    config.quantizationBits = 8;
    config.pruningThreshold = 0.01f;
    config.useNeuralEngine = true;
    config.compressionLevel = 3;
    
    bool customSuccess = optimizer.optimizeModel(modelPath, optimizedPath, config);
    printTestResult("Custom Optimization", customSuccess);
    
    // Clean up
    std::remove(modelPath.c_str());
    if (customSuccess) {
        std::remove(optimizedPath.c_str());
    }
    
    return speedSuccess && sizeSuccess && accuracySuccess && customSuccess;
}

// Test AITorchPolicy export to CoreML
bool testPolicyExport() {
    std::cout << "\n=== Testing AITorchPolicy Export to CoreML ===" << std::endl;
    
    // Create a policy
    AITorchPolicy policy;
    
    // Initialize with dummy dimensions
    std::vector<int> inputDims = {3, 128, 128}; // channels, height, width
    policy.initialize(inputDims, 10); // 10 actions
    
    // Define output path
    std::string exportPath = "test_policy_export.mlmodel";
    
    // Try exporting to CoreML
    std::cout << "Exporting policy to CoreML..." << std::endl;
    bool success = policy.exportTo(exportPath, "coreml");
    
    // Clean up
    if (success) {
        std::remove(exportPath.c_str());
        std::remove((exportPath + ".optimized.mlmodel").c_str());
    }
    
    return success;
}

int main() {
    std::cout << "=== FBNeo AI Model Export and Optimization Test ===" << std::endl;
    
    // Run tests
    bool basicConversionSuccess = testBasicConversion();
    printTestResult("Basic PyTorch to CoreML Conversion", basicConversionSuccess);
    
    bool enhancedConversionSuccess = testEnhancedConversion();
    printTestResult("Enhanced PyTorch to CoreML Conversion", enhancedConversionSuccess);
    
    bool optimizationSuccess = testModelOptimization();
    printTestResult("Model Optimization", optimizationSuccess);
    
    bool policyExportSuccess = testPolicyExport();
    printTestResult("AITorchPolicy Export", policyExportSuccess);
    
    // Overall result
    bool allSuccess = basicConversionSuccess && 
                     enhancedConversionSuccess && 
                     optimizationSuccess && 
                     policyExportSuccess;
    
    std::cout << "\n=== Test Summary ===" << std::endl;
    std::cout << "Basic Conversion: " << (basicConversionSuccess ? "PASS" : "FAIL") << std::endl;
    std::cout << "Enhanced Conversion: " << (enhancedConversionSuccess ? "PASS" : "FAIL") << std::endl;
    std::cout << "Model Optimization: " << (optimizationSuccess ? "PASS" : "FAIL") << std::endl;
    std::cout << "Policy Export: " << (policyExportSuccess ? "PASS" : "FAIL") << std::endl;
    std::cout << "\nOverall Result: " << (allSuccess ? "PASS" : "FAIL") << std::endl;
    
    return allSuccess ? 0 : 1;
} 