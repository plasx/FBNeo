#pragma once

#include <string>
#include <map>

#ifdef __OBJC__
@class QuantizationEngine;
#else
class QuantizationEngine;
#endif

// C++ wrapper for the Objective-C QuantizationEngine class
class QuantizationEngineWrapper {
private:
    #ifdef __OBJC__
    QuantizationEngine* quantizationEngine;
    #else
    void* quantizationEngine;
    #endif

public:
    QuantizationEngineWrapper() : quantizationEngine(nullptr) {}
    ~QuantizationEngineWrapper() {}

    // Initialize with a model path
    bool initWithModel(const std::string& modelPath);

    // Quantize the model with the specified options
    bool quantizeModel(bool useInt4, bool useHybrid, const std::string& outputPath);

    // Check if the device supports int4 precision
    static bool supportsInt4Precision();

    // Check if the device supports hybrid precision
    static bool supportsHybridPrecision();

    // Get statistics about the quantization
    std::map<std::string, std::string> getQuantizationStats();
}; 