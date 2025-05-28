#include "ai_input_integration.h"
#include "neural_ai_controller.h"
#include "ai_menu_system.h"
#include <iostream>

namespace AI {

// Global AI controller instance
static NeuralAIController* g_aiController = nullptr;

bool InitializeAIInputSystem() {
    // Create AI controller if not already created
    if (!g_aiController) {
        g_aiController = new NeuralAIController();
        
        // Initialize with dummy game name for now
        // In the real implementation, this would use the current game name
        if (!g_aiController->initialize("default")) {
            std::cerr << "Failed to initialize AI controller" << std::endl;
            delete g_aiController;
            g_aiController = nullptr;
            return false;
        }
        
        // Enable debugging in development builds
#ifdef _DEBUG
        g_aiController->setDebug(true);
#endif

        std::cout << "AI Input System initialized" << std::endl;
    }
    
    return true;
}

void ShutdownAIInputSystem() {
    // Clean up AI controller
    if (g_aiController) {
        delete g_aiController;
        g_aiController = nullptr;
    }
    
    std::cout << "AI Input System shutdown" << std::endl;
}

void ProcessAIFrame() {
    // Update AI controller each frame
    if (g_aiController) {
        g_aiController->processFrame();
        
        // Apply any menu system changes
        AIMenuSystem* menuSystem = AIMenuSystem::getInstance();
        if (menuSystem) {
            for (int i = 0; i < 2; i++) {
                // Check if settings have changed
                if (menuSystem->isAIControlEnabled(i) != g_aiController->isControllingPlayer(i)) {
                    g_aiController->setControllingPlayer(i, menuSystem->isAIControlEnabled(i));
                }
                
                // Load model if needed
                std::string modelPath = menuSystem->getCurrentModelPath(i);
                if (!modelPath.empty() && g_aiController->isControllingPlayer(i)) {
                    g_aiController->loadModelForPlayer(modelPath, i);
                }
                
                // Apply difficulty settings
                menuSystem->applyDifficultySettings(g_aiController, menuSystem->getDifficultyLevel(i));
            }
        }
    }
}

uint32_t GetAIInputs(int playerIndex) {
    // Get AI-controlled inputs for this player
    if (g_aiController && g_aiController->isControllingPlayer(playerIndex)) {
        // Show indicator that AI is controlling
        ShowAIIndicator(playerIndex);
        
        // Return AI inputs
        return g_aiController->getPlayerInputs(playerIndex);
    }
    
    // Not AI-controlled, return 0 (no inputs)
    return 0;
}

void ShowAIIndicator(int playerIndex) {
    // This implementation depends on the renderer
    // For now, we'll just output to console in debug builds
#ifdef _DEBUG
    static int frameCounter = 0;
    if (frameCounter++ % 60 == 0) { // Every 60 frames
        std::cout << "AI controlling player " << playerIndex << std::endl;
    }
#endif

    // In the real implementation, this would draw text on screen
    // using the appropriate rendering API (e.g., Metal, OpenGL)
    // DrawText("AI", x, y, color);
}

NeuralAIController* GetAIController() {
    return g_aiController;
}

} // namespace AI 