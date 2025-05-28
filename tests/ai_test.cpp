#include <iostream>
#include <string>
#include "burner/metal/ai/metal_ai_module.h"

int main(int argc, char** argv) {
    std::cout << "FBNeo Metal AI Module Test" << std::endl;
    
    // Initialize AI module
    if (!fbneo::metal::ai::initialize()) {
        std::cerr << "Failed to initialize AI module" << std::endl;
        return 1;
    }
    
    std::cout << "AI module initialized successfully" << std::endl;
    
    // Test enabling/disabling AI
    fbneo::metal::ai::setEnabled(true);
    std::cout << "AI enabled: " << (fbneo::metal::ai::isEnabled() ? "Yes" : "No") << std::endl;
    
    // Test training mode
    fbneo::metal::ai::setTrainingMode(true);
    std::cout << "Training mode enabled: " << (fbneo::metal::ai::isTrainingMode() ? "Yes" : "No") << std::endl;
    
    // Test starting/ending a training session
    fbneo::metal::ai::startTrainingSession();
    float reward = fbneo::metal::ai::endTrainingSession(true);
    std::cout << "Training session ended with reward: " << reward << std::endl;
    
    // Shutdown AI module
    fbneo::metal::ai::shutdown();
    std::cout << "AI module shutdown" << std::endl;
    
    return 0;
} 