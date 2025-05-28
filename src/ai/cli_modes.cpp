#include "cli_modes.h"
#include "ai_dataset_logger.h"
#include "neural_ai_controller.h"
#include "headless_mode.h"

#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <getopt.h>

// Globals for CLI mode
static bool g_modeCollect = false;
static bool g_modePlay = false;
static bool g_modeTrain = false;
static bool g_modeReplay = false;
static std::string g_romPath;
static std::string g_romName;
static std::string g_modelPath;
static std::string g_replayPath;
static std::string g_configPath;
static std::string g_outputDir = "output";
static int g_frames = 0;
static bool g_ai1 = false;
static bool g_ai2 = false;

// Print usage information
void printUsage() {
    std::cout << "FBNeo AI Integration Usage:" << std::endl;
    std::cout << "  --collect --rom <romname or path> [--frames N] [--output <dir>]" << std::endl;
    std::cout << "      Run the game in headless mode for N frames (or until game over) and log data." << std::endl;
    std::cout << "  --play --rom <rom> --model <file.pt> [--ai1] [--ai2]" << std::endl;
    std::cout << "      Launch game with AI controlling player 1 and/or 2 using the given model." << std::endl;
    std::cout << "  --train --config <config.json>" << std::endl;
    std::cout << "      Run reinforcement training as per config (experimental)." << std::endl;
    std::cout << "  --replay --rom <rom> --replay <file> [--collect]" << std::endl;
    std::cout << "      Play back a replay file; use --collect to log it as dataset." << std::endl;
}

// Parse command line arguments
bool parseCLIArgs(int argc, char* argv[]) {
    const struct option longOptions[] = {
        {"collect", no_argument, nullptr, 'c'},
        {"play", no_argument, nullptr, 'p'},
        {"train", no_argument, nullptr, 't'},
        {"replay", no_argument, nullptr, 'r'},
        {"rom", required_argument, nullptr, 'm'},
        {"model", required_argument, nullptr, 'd'},
        {"replay-file", required_argument, nullptr, 'f'},
        {"config", required_argument, nullptr, 'g'},
        {"frames", required_argument, nullptr, 'n'},
        {"output", required_argument, nullptr, 'o'},
        {"ai1", no_argument, nullptr, '1'},
        {"ai2", no_argument, nullptr, '2'},
        {"help", no_argument, nullptr, 'h'},
        {nullptr, 0, nullptr, 0}
    };

    int optionIndex = 0;
    int opt;

    while ((opt = getopt_long(argc, argv, "cptrhm:d:f:g:n:o:12", longOptions, &optionIndex)) != -1) {
        switch (opt) {
            case 'c':
                g_modeCollect = true;
                break;
            case 'p':
                g_modePlay = true;
                break;
            case 't':
                g_modeTrain = true;
                break;
            case 'r':
                g_modeReplay = true;
                break;
            case 'm':
                g_romPath = optarg;
                break;
            case 'd':
                g_modelPath = optarg;
                break;
            case 'f':
                g_replayPath = optarg;
                break;
            case 'g':
                g_configPath = optarg;
                break;
            case 'n':
                g_frames = std::stoi(optarg);
                break;
            case 'o':
                g_outputDir = optarg;
                break;
            case '1':
                g_ai1 = true;
                break;
            case '2':
                g_ai2 = true;
                break;
            case 'h':
                printUsage();
                return false;
            default:
                std::cerr << "Unknown option." << std::endl;
                printUsage();
                return false;
        }
    }

    // Extract ROM name from path if not explicitly provided
    if (!g_romPath.empty() && g_romName.empty()) {
        // Extract filename without extension
        size_t lastSlash = g_romPath.find_last_of("/\\");
        size_t lastDot = g_romPath.find_last_of(".");
        
        if (lastSlash == std::string::npos) {
            lastSlash = 0; // No slash found
        } else {
            lastSlash++; // Move past the slash
        }
        
        if (lastDot == std::string::npos || lastDot < lastSlash) {
            // No extension or dot is part of a directory name
            g_romName = g_romPath.substr(lastSlash);
        } else {
            g_romName = g_romPath.substr(lastSlash, lastDot - lastSlash);
        }
    }

    // Validate mode selection (only one mode allowed)
    int modeCount = (g_modeCollect ? 1 : 0) + (g_modePlay ? 1 : 0) + 
                    (g_modeTrain ? 1 : 0) + (g_modeReplay ? 1 : 0);
    
    if (modeCount != 1) {
        std::cerr << "Error: You must specify exactly one mode (--collect, --play, --train, or --replay)." << std::endl;
        printUsage();
        return false;
    }

    // Validate required arguments for each mode
    if (g_modeCollect || g_modePlay || g_modeReplay) {
        if (g_romPath.empty()) {
            std::cerr << "Error: --rom argument is required for this mode." << std::endl;
            return false;
        }
    }

    if (g_modePlay && g_modelPath.empty()) {
        std::cerr << "Error: --model argument is required for --play mode." << std::endl;
        return false;
    }

    if (g_modeTrain && g_configPath.empty()) {
        std::cerr << "Error: --config argument is required for --train mode." << std::endl;
        return false;
    }

    if (g_modeReplay && g_replayPath.empty()) {
        std::cerr << "Error: --replay-file argument is required for --replay mode." << std::endl;
        return false;
    }

    // Default AI players for play mode
    if (g_modePlay && !g_ai1 && !g_ai2) {
        // By default, make player 1 AI controlled
        g_ai1 = true;
    }

    return true;
}

// Run collect mode
int runCollectMode() {
    std::cout << "Running collect mode for ROM: " << g_romPath;
    if (g_frames > 0) {
        std::cout << " for " << g_frames << " frames";
    }
    std::cout << std::endl;

    // Create headless runner
    void* runner = fbneo_headless_create();
    if (!runner) {
        std::cerr << "Failed to create headless runner" << std::endl;
        return 1;
    }

    // Initialize with config
    std::string configJson = "{";
    configJson += "\"outputDir\": \"" + g_outputDir + "\"";
    if (g_frames > 0) {
        configJson += ", \"maxEpisodeLength\": " + std::to_string(g_frames);
    }
    configJson += "}";

    if (!fbneo_headless_init(runner, configJson.c_str())) {
        std::cerr << "Failed to initialize headless runner" << std::endl;
        fbneo_headless_destroy(runner);
        return 1;
    }

    // Setup data collection
    AIDatasetLogger logger(g_outputDir, g_romName);
    logger.setEnabled(true);

    // Start the runner
    if (!fbneo_headless_start(runner, g_romPath.c_str(), g_romName.c_str())) {
        std::cerr << "Failed to start headless runner" << std::endl;
        fbneo_headless_destroy(runner);
        return 1;
    }

    // Wait for the runner to finish or until frames reached
    std::cout << "Collecting data... Press Ctrl+C to stop." << std::endl;
    
    // Simple polling loop to wait for completion
    while (fbneo_headless_get_running(runner)) {
        // Print progress every second
        std::cout << "Frame: " << fbneo_headless_get_frame_count(runner) << "\r" << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    // Get final status
    int frameCount = fbneo_headless_get_frame_count(runner);
    float totalReward = fbneo_headless_get_reward(runner);

    std::cout << std::endl;
    std::cout << "Collection completed:" << std::endl;
    std::cout << "  Frames: " << frameCount << std::endl;
    std::cout << "  Total reward: " << totalReward << std::endl;
    
    // Flush logs and clean up
    logger.flush();
    logger.setEnabled(false);
    
    // Stop and destroy runner
    fbneo_headless_stop(runner);
    fbneo_headless_destroy(runner);

    std::cout << "Data collection finished. Output saved to: " << g_outputDir << std::endl;
    return 0;
}

// Run play mode
int runPlayMode() {
    std::cout << "Running play mode for ROM: " << g_romPath;
    std::cout << " with model: " << g_modelPath << std::endl;
    
    // Create AI controller
    NeuralAIController* aiController = new NeuralAIController();
    
    // Load the model
    if (!aiController->loadModel(g_modelPath.c_str())) {
        std::cerr << "Failed to load model: " << g_modelPath << std::endl;
        delete aiController;
        return 1;
    }
    
    // Initialize the controller for this game
    if (!aiController->initialize(g_romName.c_str())) {
        std::cerr << "Failed to initialize AI controller for game: " << g_romName << std::endl;
        delete aiController;
        return 1;
    }
    
    // Configure which players are AI controlled
    setAIControllerForPlayer(1, g_ai1 ? aiController : nullptr);
    setAIControllerForPlayer(2, g_ai2 ? aiController : nullptr);
    
    // Load the ROM and start the game
    if (!loadRom(g_romPath.c_str())) {
        std::cerr << "Failed to load ROM: " << g_romPath << std::endl;
        delete aiController;
        return 1;
    }
    
    // The main loop will now use the AI controller for the specified players
    std::cout << "Game started. ";
    if (g_ai1 && g_ai2) {
        std::cout << "Both players are AI controlled.";
    } else if (g_ai1) {
        std::cout << "Player 1 is AI controlled.";
    } else if (g_ai2) {
        std::cout << "Player 2 is AI controlled.";
    }
    std::cout << std::endl;
    
    // Note: The actual game loop is handled by the main emulator loop
    // We just return here and let the normal runtime handle it
    
    return 0;
}

// Run train mode
int runTrainMode() {
    std::cout << "Running train mode with config: " << g_configPath << std::endl;
    
    // For now, just print a message about using external training
    std::cout << "Note: Training functionality is currently implemented as an external Python script." << std::endl;
    std::cout << "Please use the provided training_pipeline.py script:" << std::endl;
    std::cout << "  python training_pipeline.py --data-dir " << g_outputDir << " --config " << g_configPath << std::endl;
    
    // In the future, we could implement training directly or launch the Python script
    return 0;
}

// Run replay mode
int runReplayMode() {
    std::cout << "Running replay mode for ROM: " << g_romPath;
    std::cout << " with replay file: " << g_replayPath << std::endl;
    
    // Load the replay file
    // (This part would need to be implemented based on the actual replay format)
    
    // Check if we should also collect data while replaying
    bool collectData = g_modeCollect;
    
    if (collectData) {
        std::cout << "Also collecting data to: " << g_outputDir << std::endl;
        
        // Enable data collection
        AIDatasetLogger logger(g_outputDir, g_romName);
        logger.setEnabled(true);
        
        // TODO: Implement replay playback with data collection
        
        // Flush logs and clean up
        logger.flush();
        logger.setEnabled(false);
    } else {
        // TODO: Implement replay playback without data collection
    }
    
    std::cout << "Replay finished." << std::endl;
    return 0;
}

// Main function to handle CLI modes
int handleCLIModes(int argc, char* argv[]) {
    // Parse command line arguments
    if (!parseCLIArgs(argc, argv)) {
        return 1;
    }
    
    // Run the selected mode
    if (g_modeCollect) {
        return runCollectMode();
    } else if (g_modePlay) {
        return runPlayMode();
    } else if (g_modeTrain) {
        return runTrainMode();
    } else if (g_modeReplay) {
        return runReplayMode();
    }
    
    // Should never reach here due to validation in parseCLIArgs
    return 1;
} 