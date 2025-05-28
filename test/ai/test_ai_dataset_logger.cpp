#include "../../src/ai/ai_dataset_logger.h"
#include "../../src/ai/ai_input_frame.h"
#include "../../src/ai/ai_output_action.h"
#include <cassert>
#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
namespace fs = std::filesystem;

// Helper function to check if file exists
bool fileExists(const std::string& filename) {
    return fs::exists(filename);
}

// Helper function to count lines in a file
int countLines(const std::string& filename) {
    std::ifstream file(filename);
    int count = 0;
    std::string line;
    while (std::getline(file, line)) {
        count++;
    }
    return count;
}

// Helper function to clean up test files
void cleanupTestFile(const std::string& filename) {
    if (fileExists(filename)) {
        fs::remove(filename);
    }
}

// Helper function to create test frames
AIInputFrame createTestInputFrame(int frameNumber, const std::string& gameId = "sf3") {
    // Create a test input frame
    AIInputFrame frame(frameNumber);
    frame.setGameId(gameId);
    
    // Add player 1 data
    frame.addPlayerValue(0, "p1_health", 75.0f);
    frame.addPlayerValue(0, "p1_x_pos", 150.0f);
    frame.addPlayerValue(0, "p1_y_pos", 200.0f);
    frame.addPlayerValue(0, "p1_state", 1.0f);  // Standing
    
    // Add player 2 data
    frame.addPlayerValue(1, "p2_health", 60.0f);
    frame.addPlayerValue(1, "p2_x_pos", 300.0f);
    frame.addPlayerValue(1, "p2_y_pos", 200.0f);
    frame.addPlayerValue(1, "p2_state", 2.0f);  // Crouching
    
    // Add some feature values
    frame.addFeatureValue("round_timer", 60.0f);
    frame.addFeatureValue("stage_id", 3.0f);
    
    // Add frame hash
    frame.setHash(std::to_string(frameNumber * 12345));
    
    return frame;
}

// Helper function to create test actions
AIOutputAction createTestOutputAction(int frameNumber, int playerIndex = 1) {
    AIOutputAction action;
    
    // Set the player index and frame number
    action.setPlayerIndex(playerIndex);
    action.setFrameNumber(frameNumber);
    
    // Set button states
    action.setButtonState(AIOutputAction::Button::Up, false);
    action.setButtonState(AIOutputAction::Button::Down, playerIndex == 0);
    action.setButtonState(AIOutputAction::Button::Left, false);
    action.setButtonState(AIOutputAction::Button::Right, playerIndex == 1);
    action.setButtonState(AIOutputAction::Button::Punch1, true);
    action.setButtonState(AIOutputAction::Button::Punch2, false);
    action.setButtonState(AIOutputAction::Button::Punch3, true);
    action.setButtonState(AIOutputAction::Button::Kick1, false);
    action.setButtonState(AIOutputAction::Button::Kick2, true);
    action.setButtonState(AIOutputAction::Button::Kick3, false);
    
    return action;
}

void test_dataset_logger_creation() {
    std::cout << "Testing AIDatasetLogger creation... ";
    
    const std::string testFile = "test_dataset.jsonl";
    cleanupTestFile(testFile);
    
    // Create a logger with a test file
    AIDatasetLogger logger(testFile);
    
    // Check that the file doesn't exist yet (should be created on first log)
    assert(!fileExists(testFile));
    
    std::cout << "PASSED" << std::endl;
}

void test_dataset_logger_input_frame() {
    std::cout << "Testing AIDatasetLogger input frame logging... ";
    
    const std::string testFile = "test_input_frames.jsonl";
    cleanupTestFile(testFile);
    
    // Create a logger with a test file
    AIDatasetLogger logger(testFile);
    
    // Create an input frame
    AIInputFrame frame(1);
    frame.setGameId("sf3");
    frame.setPlayerCount(2);
    
    // Add player values
    frame.setPlayerValue(0, "p1_health", 100);
    frame.setPlayerValue(0, "p1_x_pos", 150);
    frame.setPlayerValue(0, "p1_y_pos", 200);
    
    frame.setPlayerValue(1, "p2_health", 80);
    frame.setPlayerValue(1, "p2_x_pos", 350);
    frame.setPlayerValue(1, "p2_y_pos", 200);
    
    // Add global values
    frame.setGlobalValue("timer", 99);
    frame.setGlobalValue("round", 1);
    
    // Log the frame
    logger.logInputFrame(frame);
    
    // Check that the file exists now
    assert(fileExists(testFile));
    assert(countLines(testFile) == 1);
    
    // Log another frame
    AIInputFrame frame2(2);
    frame2.setGameId("sf3");
    frame2.setPlayerCount(2);
    frame2.setPlayerValue(0, "p1_health", 90);
    frame2.setPlayerValue(1, "p2_health", 70);
    logger.logInputFrame(frame2);
    
    // Check that we now have two lines
    assert(countLines(testFile) == 2);
    
    // Clean up
    cleanupTestFile(testFile);
    
    std::cout << "PASSED" << std::endl;
}

void test_dataset_logger_output_action() {
    std::cout << "Testing AIDatasetLogger output action logging... ";
    
    const std::string testFile = "test_output_actions.jsonl";
    cleanupTestFile(testFile);
    
    // Create a logger with a test file
    AIDatasetLogger logger(testFile);
    
    // Create an output action
    AIOutputAction action(1);
    action.setPlayerIndex(0);
    action.setGameId("sf3");
    
    // Set some buttons
    action.setButton("up", true);
    action.setButton("punch_light", true);
    
    // Log the action
    logger.logOutputAction(action);
    
    // Check that the file exists now
    assert(fileExists(testFile));
    assert(countLines(testFile) == 1);
    
    // Log another action
    AIOutputAction action2(2);
    action2.setPlayerIndex(0);
    action2.setGameId("sf3");
    action2.setButton("down", true);
    action2.setButton("kick_medium", true);
    logger.logOutputAction(action2);
    
    // Check that we now have two lines
    assert(countLines(testFile) == 2);
    
    // Clean up
    cleanupTestFile(testFile);
    
    std::cout << "PASSED" << std::endl;
}

void test_dataset_logger_frame_action_pair() {
    std::cout << "Testing AIDatasetLogger frame-action pair logging... ";
    
    const std::string testFile = "test_frame_action_pairs.jsonl";
    cleanupTestFile(testFile);
    
    // Create a logger with a test file
    AIDatasetLogger logger(testFile);
    
    // Create an input frame
    AIInputFrame frame(1);
    frame.setGameId("sf3");
    frame.setPlayerCount(2);
    frame.setPlayerValue(0, "p1_health", 100);
    frame.setPlayerValue(1, "p2_health", 80);
    
    // Create an output action
    AIOutputAction action(1);
    action.setPlayerIndex(0);
    action.setGameId("sf3");
    action.setButton("up", true);
    action.setButton("punch_light", true);
    
    // Log the pair
    logger.logFrameActionPair(frame, action);
    
    // Check that the file exists now
    assert(fileExists(testFile));
    assert(countLines(testFile) == 1);
    
    // Clean up
    cleanupTestFile(testFile);
    
    std::cout << "PASSED" << std::endl;
}

void test_dataset_logger_replay() {
    std::cout << "Testing AIDatasetLogger replay logging... ";
    
    const std::string testFile = "test_replay.jsonl";
    cleanupTestFile(testFile);
    
    // Create a logger with a test file
    AIDatasetLogger logger(testFile);
    
    // Create a sequence of frames and actions
    for (int i = 1; i <= 10; i++) {
        // Create frame
        AIInputFrame frame(i);
        frame.setGameId("sf3");
        frame.setPlayerCount(2);
        frame.setPlayerValue(0, "p1_health", 100 - i);
        frame.setPlayerValue(1, "p2_health", 80 - i * 2);
        
        // Create action
        AIOutputAction action(i);
        action.setPlayerIndex(0);
        action.setGameId("sf3");
        action.setButton("up", i % 2 == 0);
        action.setButton("punch_light", i % 3 == 0);
        
        // Log the pair
        logger.logFrameActionPair(frame, action);
    }
    
    // Check that we have 10 lines
    assert(fileExists(testFile));
    assert(countLines(testFile) == 10);
    
    // Test replay loading
    std::vector<std::pair<AIInputFrame, AIOutputAction>> replay = AIDatasetLogger::loadReplay(testFile);
    
    // Check replay size
    assert(replay.size() == 10);
    
    // Check some values from the first and last entries
    assert(replay[0].first.getFrameNumber() == 1);
    assert(replay[0].first.getPlayerValue(0, "p1_health") == 99);
    assert(replay[0].second.getButton("up") == false);
    
    assert(replay[9].first.getFrameNumber() == 10);
    assert(replay[9].first.getPlayerValue(0, "p1_health") == 90);
    assert(replay[9].second.getButton("up") == true);
    
    // Clean up
    cleanupTestFile(testFile);
    
    std::cout << "PASSED" << std::endl;
}

void test_dataset_logger_append() {
    std::cout << "Testing AIDatasetLogger append mode... ";
    
    const std::string testFile = "test_append.jsonl";
    cleanupTestFile(testFile);
    
    // Create a frame and action
    AIInputFrame frame(1);
    frame.setGameId("sf3");
    frame.setPlayerValue(0, "p1_health", 100);
    
    AIOutputAction action(1);
    action.setPlayerIndex(0);
    action.setGameId("sf3");
    action.setButton("up", true);
    
    {
        // Create first logger and log a pair
        AIDatasetLogger logger1(testFile);
        logger1.logFrameActionPair(frame, action);
        assert(countLines(testFile) == 1);
    }
    
    {
        // Create a second logger for the same file (should append)
        AIDatasetLogger logger2(testFile);
        
        // Create a new frame and action with different frame number
        AIInputFrame frame2(2);
        frame2.setGameId("sf3");
        frame2.setPlayerValue(0, "p1_health", 90);
        
        AIOutputAction action2(2);
        action2.setPlayerIndex(0);
        action2.setGameId("sf3");
        action2.setButton("down", true);
        
        // Log the second pair
        logger2.logFrameActionPair(frame2, action2);
        
        // Should have both entries
        assert(countLines(testFile) == 2);
    }
    
    // Clean up
    cleanupTestFile(testFile);
    
    std::cout << "PASSED" << std::endl;
}

void test_dataset_logger_frame_hash() {
    std::cout << "Testing AIDatasetLogger frame hash... ";
    
    const std::string testFile = "test_frame_hash.jsonl";
    cleanupTestFile(testFile);
    
    // Create a logger with a test file
    AIDatasetLogger logger(testFile);
    
    // Create two identical frames
    AIInputFrame frame1(1);
    frame1.setGameId("sf3");
    frame1.setPlayerValue(0, "p1_health", 100);
    frame1.setPlayerValue(1, "p2_health", 80);
    
    AIInputFrame frame2(1);
    frame2.setGameId("sf3");
    frame2.setPlayerValue(0, "p1_health", 100);
    frame2.setPlayerValue(1, "p2_health", 80);
    
    // Create a different frame
    AIInputFrame frame3(1);
    frame3.setGameId("sf3");
    frame3.setPlayerValue(0, "p1_health", 90); // Different health value
    frame3.setPlayerValue(1, "p2_health", 80);
    
    // Compute hashes
    uint64_t hash1 = frame1.computeHash();
    uint64_t hash2 = frame2.computeHash();
    uint64_t hash3 = frame3.computeHash();
    
    // Identical frames should have identical hashes
    assert(hash1 == hash2);
    
    // Different frames should have different hashes
    assert(hash1 != hash3);
    
    // Log frames with hashes
    logger.logInputFrameWithHash(frame1);
    logger.logInputFrameWithHash(frame3);
    
    // Check that we have two lines
    assert(fileExists(testFile));
    assert(countLines(testFile) == 2);
    
    // Load the frames
    std::vector<AIInputFrame> frames = AIDatasetLogger::loadInputFrames(testFile);
    
    // Check that hashes were preserved
    assert(frames.size() == 2);
    assert(frames[0].getHash() == hash1);
    assert(frames[1].getHash() == hash3);
    
    // Clean up
    cleanupTestFile(testFile);
    
    std::cout << "PASSED" << std::endl;
}

void test_dataset_logger_validation() {
    std::cout << "Testing AIDatasetLogger replay validation... ";
    
    const std::string originalFile = "test_original.jsonl";
    const std::string validationFile = "test_validation.jsonl";
    cleanupTestFile(originalFile);
    cleanupTestFile(validationFile);
    
    // Create loggers
    AIDatasetLogger originalLogger(originalFile);
    AIDatasetLogger validationLogger(validationFile);
    
    // Create a sequence of frames for original replay
    for (int i = 1; i <= 10; i++) {
        AIInputFrame frame(i);
        frame.setGameId("sf3");
        frame.setPlayerValue(0, "p1_health", 100 - i);
        frame.setPlayerValue(1, "p2_health", 80 - i * 2);
        frame.computeAndSetHash();
        
        AIOutputAction action(i);
        action.setPlayerIndex(0);
        action.setGameId("sf3");
        action.setButton("up", i % 2 == 0);
        
        originalLogger.logFrameActionPair(frame, action);
    }
    
    // Create validation replay with the same frames but one different
    for (int i = 1; i <= 10; i++) {
        AIInputFrame frame(i);
        frame.setGameId("sf3");
        
        // Make frame 5 different
        if (i == 5) {
            frame.setPlayerValue(0, "p1_health", 50); // Different value
        } else {
            frame.setPlayerValue(0, "p1_health", 100 - i);
        }
        
        frame.setPlayerValue(1, "p2_health", 80 - i * 2);
        frame.computeAndSetHash();
        
        validationLogger.logInputFrameWithHash(frame);
    }
    
    // Validate the replays
    auto results = AIDatasetLogger::validateReplayHashes(originalFile, validationFile);
    
    // Check validation results
    assert(results.first == 9);  // 9 matching frames
    assert(results.second.size() == 1); // 1 mismatch
    assert(results.second[0].first == 5); // Mismatch at frame 5
    
    // Clean up
    cleanupTestFile(originalFile);
    cleanupTestFile(validationFile);
    
    std::cout << "PASSED" << std::endl;
}

int main() {
    std::cout << "==== AIDatasetLogger Tests ====" << std::endl;
    
    test_dataset_logger_creation();
    test_dataset_logger_input_frame();
    test_dataset_logger_output_action();
    test_dataset_logger_frame_action_pair();
    test_dataset_logger_replay();
    test_dataset_logger_append();
    test_dataset_logger_frame_hash();
    test_dataset_logger_validation();
    
    std::cout << "All tests PASSED!" << std::endl;
    return 0;
} 