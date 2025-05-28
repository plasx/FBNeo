#include "ai_controller.h"
#include "ai_torch_policy.h"
#include "ai_memory_mapping.h"
#include "ai_dataset_logger.h"
#include "ai_menu_system.h"
#include <cstdio>

/**
 * @brief Initialize all AI components
 * @return 0 on success, non-zero on failure
 */
extern "C" int AIModuleInit() {
    printf("Initializing FBNeo AI Module...\n");
    
    // Initialize memory mapping
    if (g_pAIMemoryMapping) {
        printf("AIMemoryMapping already initialized\n");
    } else {
        g_pAIMemoryMapping = new AIMemoryMapping();
        if (!g_pAIMemoryMapping->Initialize()) {
            printf("Failed to initialize AIMemoryMapping\n");
            return 1;
        }
        printf("AIMemoryMapping initialized successfully\n");
    }
    
    // Initialize torch policy
    if (g_pAITorchPolicy) {
        printf("AITorchPolicy already initialized\n");
    } else {
        g_pAITorchPolicy = new AITorchPolicy();
        if (!g_pAITorchPolicy->Initialize()) {
            printf("Failed to initialize AITorchPolicy\n");
            // Non-fatal error - continue without policy
        } else {
            printf("AITorchPolicy initialized successfully\n");
        }
    }
    
    // Initialize dataset logger
    if (g_pAIDatasetLogger) {
        printf("AIDatasetLogger already initialized\n");
    } else {
        g_pAIDatasetLogger = new AIDatasetLogger();
        if (!g_pAIDatasetLogger->Initialize()) {
            printf("Failed to initialize AIDatasetLogger\n");
            // Non-fatal error - continue without logger
        } else {
            printf("AIDatasetLogger initialized successfully\n");
        }
    }
    
    // Initialize AI controller
    AIController::Initialize();
    printf("AIController initialized successfully\n");
    
    // Initialize menu system
    AIMenuSystemInit();
    printf("AIMenuSystem initialized successfully\n");
    
    printf("FBNeo AI Module initialization complete\n");
    return 0;
}

/**
 * @brief Shutdown all AI components
 * @return 0 on success
 */
extern "C" int AIModuleExit() {
    printf("Shutting down FBNeo AI Module...\n");
    
    // Shutdown menu system
    AIMenuSystemExit();
    printf("AIMenuSystem shutdown complete\n");
    
    // Shutdown AI controller
    AIController::Shutdown();
    printf("AIController shutdown complete\n");
    
    // Shutdown dataset logger
    if (g_pAIDatasetLogger) {
        delete g_pAIDatasetLogger;
        g_pAIDatasetLogger = nullptr;
        printf("AIDatasetLogger shutdown complete\n");
    }
    
    // Shutdown torch policy
    if (g_pAITorchPolicy) {
        g_pAITorchPolicy->Shutdown();
        delete g_pAITorchPolicy;
        g_pAITorchPolicy = nullptr;
        printf("AITorchPolicy shutdown complete\n");
    }
    
    // Shutdown memory mapping
    if (g_pAIMemoryMapping) {
        delete g_pAIMemoryMapping;
        g_pAIMemoryMapping = nullptr;
        printf("AIMemoryMapping shutdown complete\n");
    }
    
    printf("FBNeo AI Module shutdown complete\n");
    return 0;
}

/**
 * @brief Update AI systems each frame
 * Call this from the main emulation loop
 */
extern "C" void AIModuleUpdate() {
    // Update AI controller
    if (g_pAIController) {
        g_pAIController->Update();
    }
}

/**
 * @brief Handle game loading to load appropriate game-specific settings
 * @param game_name Name of the loaded game
 */
extern "C" void AIModuleGameLoaded(const char* game_name) {
    printf("AI Module: Game loaded - %s\n", game_name);
    
    // Load memory mapping for this game
    if (g_pAIMemoryMapping) {
        g_pAIMemoryMapping->LoadMappingForGame(game_name);
    }
    
    // Initialize AI controller with game-specific settings
    if (g_pAIController) {
        // Nothing to do here yet
    }
    
    // Initialize menu system with game-specific options
    if (g_pAIMenuSystem) {
        g_pAIMenuSystem->Initialize();
    }
} 