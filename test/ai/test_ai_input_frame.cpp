#include "../../src/ai/ai_input_frame.h"
#include "../../src/ai/ai_memory_mapping.h"
#include <cassert>
#include <iostream>
#include <cstring>
#include <memory>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <cmath>

// Mock implementations for FBNeo functions required by AIMemoryMapping
extern "C" {
    unsigned char CpuReadByte(unsigned int /*addr*/) { return 0x42; }
    unsigned short CpuReadWord(unsigned int /*addr*/) { return 0x4242; }
    unsigned int CpuReadLong(unsigned int /*addr*/) { return 0x42424242; }
    int BurnDrvGetTextA(unsigned int nIndex) { 
        static const char* names[] = {
            "Street Fighter II",  // 0: Full name
            "SF2",                // 1: Short name
            "Capcom",             // 2: Manufacturer
            ""
        };
        if (nIndex <= 2) return (int)names[nIndex];
        return 0;
    }
}

// Helper function to test approximate equality for floating-point values
bool approxEqual(float a, float b, float epsilon = 0.0001f) {
    return std::fabs(a - b) < epsilon;
}

// Test suite for AIInputFrame
void test_input_frame_creation() {
    std::cout << "Testing AIInputFrame creation... ";
    
    // Create an input frame
    AIInputFrame frame;
    
    // Check initial values
    assert(frame.getFrameNumber() == 0);
    assert(frame.getPlayerIndex() == 0);
    assert(frame.getGameTick() == 0);
    assert(frame.getMemoryValues().empty());
    assert(frame.getFrameHash() == 0);
    
    std::cout << "PASSED" << std::endl;
}

void test_input_frame_setting_values() {
    std::cout << "Testing AIInputFrame setting values... ";
    
    // Create an input frame
    AIInputFrame frame;
    
    // Set values
    frame.setFrameNumber(42);
    frame.setPlayerIndex(1);
    frame.setGameTick(123);
    
    // Check values
    assert(frame.getFrameNumber() == 42);
    assert(frame.getPlayerIndex() == 1);
    assert(frame.getGameTick() == 123);
    
    std::cout << "PASSED" << std::endl;
}

void test_input_frame_memory_values() {
    std::cout << "Testing AIInputFrame memory values... ";
    
    // Create an input frame
    AIInputFrame frame;
    
    // Add memory values
    frame.setMemoryValue("p1_health", 100);
    frame.setMemoryValue("p2_health", 80);
    frame.setMemoryValue("p1_x_pos", 150);
    frame.setMemoryValue("p1_y_pos", 200);
    
    // Check values exist
    auto memoryValues = frame.getMemoryValues();
    assert(memoryValues.size() == 4);
    assert(memoryValues.find("p1_health") != memoryValues.end());
    assert(memoryValues.find("p2_health") != memoryValues.end());
    assert(memoryValues.find("p1_x_pos") != memoryValues.end());
    assert(memoryValues.find("p1_y_pos") != memoryValues.end());
    
    // Check specific values
    assert(memoryValues["p1_health"] == 100);
    assert(memoryValues["p2_health"] == 80);
    assert(memoryValues["p1_x_pos"] == 150);
    assert(memoryValues["p1_y_pos"] == 200);
    
    // Check direct getter
    assert(frame.getMemoryValue("p1_health") == 100);
    assert(frame.getMemoryValue("p2_health") == 80);
    assert(frame.getMemoryValue("p1_x_pos") == 150);
    assert(frame.getMemoryValue("p1_y_pos") == 200);
    
    // Check non-existent key returns 0
    assert(frame.getMemoryValue("non_existent") == 0);
    
    std::cout << "PASSED" << std::endl;
}

void test_input_frame_clear() {
    std::cout << "Testing AIInputFrame clear... ";
    
    // Create an input frame
    AIInputFrame frame;
    
    // Set values
    frame.setFrameNumber(42);
    frame.setPlayerIndex(1);
    frame.setGameTick(123);
    frame.setMemoryValue("p1_health", 100);
    frame.setMemoryValue("p2_health", 80);
    
    // Clear
    frame.clear();
    
    // Check values are reset
    assert(frame.getFrameNumber() == 0);
    assert(frame.getPlayerIndex() == 0);
    assert(frame.getGameTick() == 0);
    assert(frame.getMemoryValues().empty());
    assert(frame.getFrameHash() == 0);
    
    std::cout << "PASSED" << std::endl;
}

void test_input_frame_hash() {
    std::cout << "Testing AIInputFrame hash... ";
    
    // Create two identical frames
    AIInputFrame frame1;
    AIInputFrame frame2;
    
    // Set values (identical)
    frame1.setFrameNumber(42);
    frame1.setPlayerIndex(1);
    frame1.setGameTick(123);
    frame1.setMemoryValue("p1_health", 100);
    frame1.setMemoryValue("p2_health", 80);
    
    frame2.setFrameNumber(42);
    frame2.setPlayerIndex(1);
    frame2.setGameTick(123);
    frame2.setMemoryValue("p1_health", 100);
    frame2.setMemoryValue("p2_health", 80);
    
    // Calculate hashes
    uint64_t hash1 = frame1.calculateAndGetFrameHash();
    uint64_t hash2 = frame2.calculateAndGetFrameHash();
    
    // Hashes should be equal for identical frames
    assert(hash1 == hash2);
    assert(hash1 != 0);
    assert(frame1.getFrameHash() == hash1);
    assert(frame2.getFrameHash() == hash2);
    
    // Modify one value
    frame2.setMemoryValue("p1_health", 99);
    uint64_t hash3 = frame2.calculateAndGetFrameHash();
    
    // Hashes should now be different
    assert(hash1 != hash3);
    
    std::cout << "PASSED" << std::endl;
}

void test_input_frame_serialization() {
    std::cout << "Testing AIInputFrame serialization... ";
    
    // Create a frame
    AIInputFrame frame1;
    frame1.setFrameNumber(42);
    frame1.setPlayerIndex(1);
    frame1.setGameTick(123);
    frame1.setMemoryValue("p1_health", 100);
    frame1.setMemoryValue("p2_health", 80);
    
    // Calculate hash
    frame1.calculateAndGetFrameHash();
    
    // Serialize to JSON
    std::string json = frame1.toJson();
    
    // Create a new frame from JSON
    AIInputFrame frame2;
    frame2.fromJson(json);
    
    // Check values are transferred correctly
    assert(frame2.getFrameNumber() == 42);
    assert(frame2.getPlayerIndex() == 1);
    assert(frame2.getGameTick() == 123);
    
    auto memoryValues = frame2.getMemoryValues();
    assert(memoryValues.size() == 2);
    assert(memoryValues["p1_health"] == 100);
    assert(memoryValues["p2_health"] == 80);
    
    // Hash should be preserved
    assert(frame2.getFrameHash() == frame1.getFrameHash());
    
    std::cout << "PASSED" << std::endl;
}

void test_input_frame_copy() {
    std::cout << "Testing AIInputFrame copy... ";
    
    // Create a frame
    AIInputFrame frame1;
    frame1.setFrameNumber(42);
    frame1.setPlayerIndex(1);
    frame1.setGameTick(123);
    frame1.setMemoryValue("p1_health", 100);
    frame1.setMemoryValue("p2_health", 80);
    frame1.calculateAndGetFrameHash();
    
    // Copy constructor
    AIInputFrame frame2(frame1);
    
    // Check values are copied correctly
    assert(frame2.getFrameNumber() == 42);
    assert(frame2.getPlayerIndex() == 1);
    assert(frame2.getGameTick() == 123);
    
    auto memoryValues = frame2.getMemoryValues();
    assert(memoryValues.size() == 2);
    assert(memoryValues["p1_health"] == 100);
    assert(memoryValues["p2_health"] == 80);
    assert(frame2.getFrameHash() == frame1.getFrameHash());
    
    // Assignment operator
    AIInputFrame frame3;
    frame3 = frame1;
    
    // Check values are copied correctly
    assert(frame3.getFrameNumber() == 42);
    assert(frame3.getPlayerIndex() == 1);
    assert(frame3.getGameTick() == 123);
    
    memoryValues = frame3.getMemoryValues();
    assert(memoryValues.size() == 2);
    assert(memoryValues["p1_health"] == 100);
    assert(memoryValues["p2_health"] == 80);
    assert(frame3.getFrameHash() == frame1.getFrameHash());
    
    std::cout << "PASSED" << std::endl;
}

void test_input_frame_to_string() {
    std::cout << "Testing AIInputFrame toString... ";
    
    // Create a frame
    AIInputFrame frame;
    frame.setFrameNumber(42);
    frame.setPlayerIndex(1);
    frame.setGameTick(123);
    frame.setMemoryValue("p1_health", 100);
    frame.setMemoryValue("p2_health", 80);
    
    // Get string representation
    std::string str = frame.toString();
    
    // Check that the string contains expected values
    assert(str.find("42") != std::string::npos);  // Frame number
    assert(str.find("1") != std::string::npos);   // Player index
    assert(str.find("123") != std::string::npos); // Game tick
    assert(str.find("p1_health") != std::string::npos);
    assert(str.find("p2_health") != std::string::npos);
    assert(str.find("100") != std::string::npos);
    assert(str.find("80") != std::string::npos);
    
    std::cout << "PASSED" << std::endl;
}

// Test frame construction and basic properties
void test_frame_construction() {
    std::cout << "Testing AIInputFrame construction... ";
    
    // Test default constructor
    AIInputFrame emptyFrame;
    assert(emptyFrame.getFrameNumber() == 0);
    assert(emptyFrame.getGameId().empty());
    assert(emptyFrame.getHash().empty());
    
    // Test constructor with frame number
    AIInputFrame frame(42);
    assert(frame.getFrameNumber() == 42);
    assert(frame.getGameId().empty());
    assert(frame.getHash().empty());
    
    // Test setting and getting game ID
    frame.setGameId("test_game");
    assert(frame.getGameId() == "test_game");
    
    // Test setting and getting frame hash
    frame.setHash("test_hash_value");
    assert(frame.getHash() == "test_hash_value");
    
    // Test frame number modification
    frame.setFrameNumber(100);
    assert(frame.getFrameNumber() == 100);
    
    std::cout << "PASSED" << std::endl;
}

// Test player value operations
void test_player_values() {
    std::cout << "Testing AIInputFrame player values... ";
    
    AIInputFrame frame(1);
    frame.setGameId("test_game");
    
    // Test adding player values
    frame.addPlayerValue(0, "health", 100.0f);
    frame.addPlayerValue(0, "x_pos", 150.0f);
    frame.addPlayerValue(0, "y_pos", 200.0f);
    
    frame.addPlayerValue(1, "health", 80.0f);
    frame.addPlayerValue(1, "x_pos", 250.0f);
    frame.addPlayerValue(1, "y_pos", 200.0f);
    
    // Test retrieving player values
    assert(approxEqual(frame.getPlayerValue(0, "health"), 100.0f));
    assert(approxEqual(frame.getPlayerValue(0, "x_pos"), 150.0f));
    assert(approxEqual(frame.getPlayerValue(0, "y_pos"), 200.0f));
    
    assert(approxEqual(frame.getPlayerValue(1, "health"), 80.0f));
    assert(approxEqual(frame.getPlayerValue(1, "x_pos"), 250.0f));
    assert(approxEqual(frame.getPlayerValue(1, "y_pos"), 200.0f));
    
    // Test default value for non-existent keys
    assert(approxEqual(frame.getPlayerValue(0, "non_existent"), 0.0f));
    assert(approxEqual(frame.getPlayerValue(2, "health"), 0.0f));  // Non-existent player
    
    // Test updating values
    frame.addPlayerValue(0, "health", 90.0f);  // Update existing value
    assert(approxEqual(frame.getPlayerValue(0, "health"), 90.0f));
    
    // Test getting player value names
    std::vector<std::string> p0ValueNames = frame.getPlayerValueNames(0);
    assert(p0ValueNames.size() == 3);
    
    // The vector should contain these names, but order is not guaranteed
    assert(std::find(p0ValueNames.begin(), p0ValueNames.end(), "health") != p0ValueNames.end());
    assert(std::find(p0ValueNames.begin(), p0ValueNames.end(), "x_pos") != p0ValueNames.end());
    assert(std::find(p0ValueNames.begin(), p0ValueNames.end(), "y_pos") != p0ValueNames.end());
    
    // Test player indices
    std::vector<int> playerIndices = frame.getPlayerIndices();
    assert(playerIndices.size() == 2);
    assert(std::find(playerIndices.begin(), playerIndices.end(), 0) != playerIndices.end());
    assert(std::find(playerIndices.begin(), playerIndices.end(), 1) != playerIndices.end());
    
    std::cout << "PASSED" << std::endl;
}

// Test feature value operations
void test_feature_values() {
    std::cout << "Testing AIInputFrame feature values... ";
    
    AIInputFrame frame(1);
    
    // Test adding feature values
    frame.addFeatureValue("round_timer", 60.0f);
    frame.addFeatureValue("round_number", 2.0f);
    frame.addFeatureValue("stage_id", 3.0f);
    
    // Test retrieving feature values
    assert(approxEqual(frame.getFeatureValue("round_timer"), 60.0f));
    assert(approxEqual(frame.getFeatureValue("round_number"), 2.0f));
    assert(approxEqual(frame.getFeatureValue("stage_id"), 3.0f));
    
    // Test default value for non-existent keys
    assert(approxEqual(frame.getFeatureValue("non_existent"), 0.0f));
    
    // Test updating values
    frame.addFeatureValue("round_timer", 59.0f);  // Update existing value
    assert(approxEqual(frame.getFeatureValue("round_timer"), 59.0f));
    
    // Test getting feature value names
    std::vector<std::string> featureNames = frame.getFeatureValueNames();
    assert(featureNames.size() == 3);
    
    // The vector should contain these names, but order is not guaranteed
    assert(std::find(featureNames.begin(), featureNames.end(), "round_timer") != featureNames.end());
    assert(std::find(featureNames.begin(), featureNames.end(), "round_number") != featureNames.end());
    assert(std::find(featureNames.begin(), featureNames.end(), "stage_id") != featureNames.end());
    
    std::cout << "PASSED" << std::endl;
}

// Test JSON serialization and deserialization
void test_json_serialization() {
    std::cout << "Testing AIInputFrame JSON serialization... ";
    
    // Create a frame with data
    AIInputFrame originalFrame(42);
    originalFrame.setGameId("test_game");
    originalFrame.setHash("abcdef123456");
    
    // Add player values
    originalFrame.addPlayerValue(0, "health", 90.0f);
    originalFrame.addPlayerValue(0, "x_pos", 150.0f);
    originalFrame.addPlayerValue(1, "health", 80.0f);
    originalFrame.addPlayerValue(1, "x_pos", 250.0f);
    
    // Add feature values
    originalFrame.addFeatureValue("round_timer", 59.0f);
    originalFrame.addFeatureValue("stage_id", 3.0f);
    
    // Serialize to JSON string
    std::string jsonStr = originalFrame.toJson();
    
    // Create a new frame from the JSON
    AIInputFrame deserializedFrame;
    bool loadResult = deserializedFrame.fromJson(jsonStr);
    assert(loadResult);
    
    // Verify deserialized data
    assert(deserializedFrame.getFrameNumber() == 42);
    assert(deserializedFrame.getGameId() == "test_game");
    assert(deserializedFrame.getHash() == "abcdef123456");
    
    assert(approxEqual(deserializedFrame.getPlayerValue(0, "health"), 90.0f));
    assert(approxEqual(deserializedFrame.getPlayerValue(0, "x_pos"), 150.0f));
    assert(approxEqual(deserializedFrame.getPlayerValue(1, "health"), 80.0f));
    assert(approxEqual(deserializedFrame.getPlayerValue(1, "x_pos"), 250.0f));
    
    assert(approxEqual(deserializedFrame.getFeatureValue("round_timer"), 59.0f));
    assert(approxEqual(deserializedFrame.getFeatureValue("stage_id"), 3.0f));
    
    // Test with invalid JSON
    AIInputFrame invalidFrame;
    bool invalidResult = invalidFrame.fromJson("{invalid json}");
    assert(!invalidResult);
    
    // Test with valid JSON but missing fields
    bool incompleteResult = invalidFrame.fromJson(R"({"game_id": "test"})");
    assert(incompleteResult);  // Should still succeed but with default values
    assert(invalidFrame.getGameId() == "test");
    assert(invalidFrame.getFrameNumber() == 0);  // Default value
    
    std::cout << "PASSED" << std::endl;
}

// Test frame comparison
void test_frame_comparison() {
    std::cout << "Testing AIInputFrame comparison... ";
    
    // Create two identical frames
    AIInputFrame frame1(1);
    frame1.setGameId("test_game");
    frame1.setHash("hash1");
    frame1.addPlayerValue(0, "health", 100.0f);
    frame1.addFeatureValue("timer", 60.0f);
    
    AIInputFrame frame2(1);
    frame2.setGameId("test_game");
    frame2.setHash("hash1");
    frame2.addPlayerValue(0, "health", 100.0f);
    frame2.addFeatureValue("timer", 60.0f);
    
    // Create a frame with different values
    AIInputFrame frame3(1);
    frame3.setGameId("test_game");
    frame3.setHash("hash2");  // Different hash
    frame3.addPlayerValue(0, "health", 90.0f);  // Different health
    frame3.addFeatureValue("timer", 60.0f);
    
    // Test equality
    assert(frame1.equals(frame2));
    assert(!frame1.equals(frame3));
    
    // Test frame difference
    std::vector<std::string> differences;
    bool areDifferent = frame1.findDifferences(frame3, differences);
    assert(areDifferent);
    assert(differences.size() >= 2);  // At least hash and health should be different
    
    // Check that specific differences are reported
    bool hashDiffFound = false;
    bool healthDiffFound = false;
    
    for (const auto& diff : differences) {
        if (diff.find("hash") != std::string::npos) hashDiffFound = true;
        if (diff.find("health") != std::string::npos) healthDiffFound = true;
    }
    
    assert(hashDiffFound);
    assert(healthDiffFound);
    
    // Test with identical frames
    differences.clear();
    areDifferent = frame1.findDifferences(frame2, differences);
    assert(!areDifferent);
    assert(differences.empty());
    
    std::cout << "PASSED" << std::endl;
}

// Test frame hash generation
void test_frame_hash_generation() {
    std::cout << "Testing AIInputFrame hash generation... ";
    
    AIInputFrame frame(1);
    frame.setGameId("test_game");
    
    // Add player values
    frame.addPlayerValue(0, "health", 100.0f);
    frame.addPlayerValue(0, "x_pos", 150.0f);
    frame.addPlayerValue(1, "health", 80.0f);
    
    // Add feature values
    frame.addFeatureValue("round_timer", 60.0f);
    
    // Generate hash
    frame.generateHash();
    
    // Verify hash is not empty
    assert(!frame.getHash().empty());
    
    // Generate hash for identical frame and verify it matches
    AIInputFrame identicalFrame(1);
    identicalFrame.setGameId("test_game");
    identicalFrame.addPlayerValue(0, "health", 100.0f);
    identicalFrame.addPlayerValue(0, "x_pos", 150.0f);
    identicalFrame.addPlayerValue(1, "health", 80.0f);
    identicalFrame.addFeatureValue("round_timer", 60.0f);
    identicalFrame.generateHash();
    
    assert(frame.getHash() == identicalFrame.getHash());
    
    // Change a value and verify hash is different
    AIInputFrame differentFrame(1);
    differentFrame.setGameId("test_game");
    differentFrame.addPlayerValue(0, "health", 99.0f);  // Different health
    differentFrame.addPlayerValue(0, "x_pos", 150.0f);
    differentFrame.addPlayerValue(1, "health", 80.0f);
    differentFrame.addFeatureValue("round_timer", 60.0f);
    differentFrame.generateHash();
    
    assert(frame.getHash() != differentFrame.getHash());
    
    std::cout << "PASSED" << std::endl;
}

// Test frame copying
void test_frame_copying() {
    std::cout << "Testing AIInputFrame copying... ";
    
    // Create a frame with data
    AIInputFrame originalFrame(42);
    originalFrame.setGameId("test_game");
    originalFrame.setHash("abcdef123456");
    originalFrame.addPlayerValue(0, "health", 90.0f);
    originalFrame.addFeatureValue("round_timer", 59.0f);
    
    // Test copy constructor
    AIInputFrame copiedFrame(originalFrame);
    
    assert(copiedFrame.getFrameNumber() == 42);
    assert(copiedFrame.getGameId() == "test_game");
    assert(copiedFrame.getHash() == "abcdef123456");
    assert(approxEqual(copiedFrame.getPlayerValue(0, "health"), 90.0f));
    assert(approxEqual(copiedFrame.getFeatureValue("round_timer"), 59.0f));
    
    // Modify original and ensure copy is not affected
    originalFrame.setFrameNumber(43);
    originalFrame.addPlayerValue(0, "health", 80.0f);
    
    assert(copiedFrame.getFrameNumber() == 42);  // Unchanged
    assert(approxEqual(copiedFrame.getPlayerValue(0, "health"), 90.0f));  // Unchanged
    
    // Test assignment operator
    AIInputFrame assignedFrame;
    assignedFrame = originalFrame;
    
    assert(assignedFrame.getFrameNumber() == 43);
    assert(assignedFrame.getGameId() == "test_game");
    assert(assignedFrame.getHash() == "abcdef123456");
    assert(approxEqual(assignedFrame.getPlayerValue(0, "health"), 80.0f));
    assert(approxEqual(assignedFrame.getFeatureValue("round_timer"), 59.0f));
    
    std::cout << "PASSED" << std::endl;
}

// Test file I/O operations with frames
void test_frame_file_io() {
    std::cout << "Testing AIInputFrame file I/O... ";
    
    // Create a temporary file
    std::string filename = "test_frame.jsonl";
    std::ofstream file(filename);
    assert(file.is_open());
    
    // Create and save multiple frames
    AIInputFrame frame1(1);
    frame1.setGameId("test_game");
    frame1.addPlayerValue(0, "health", 100.0f);
    frame1.generateHash();
    
    AIInputFrame frame2(2);
    frame2.setGameId("test_game");
    frame2.addPlayerValue(0, "health", 90.0f);
    frame2.generateHash();
    
    // Write frames to file
    file << frame1.toJson() << std::endl;
    file << frame2.toJson() << std::endl;
    file.close();
    
    // Read frames from file
    std::ifstream inputFile(filename);
    assert(inputFile.is_open());
    
    std::string line;
    std::vector<AIInputFrame> loadedFrames;
    
    while (std::getline(inputFile, line)) {
        AIInputFrame frame;
        bool success = frame.fromJson(line);
        assert(success);
        loadedFrames.push_back(frame);
    }
    
    inputFile.close();
    
    // Verify loaded frames
    assert(loadedFrames.size() == 2);
    assert(loadedFrames[0].getFrameNumber() == 1);
    assert(loadedFrames[1].getFrameNumber() == 2);
    assert(approxEqual(loadedFrames[0].getPlayerValue(0, "health"), 100.0f));
    assert(approxEqual(loadedFrames[1].getPlayerValue(0, "health"), 90.0f));
    
    // Clean up
    std::remove(filename.c_str());
    
    std::cout << "PASSED" << std::endl;
}

int main() {
    std::cout << "==== AIInputFrame Tests ====" << std::endl;
    
    test_input_frame_creation();
    test_input_frame_setting_values();
    test_input_frame_memory_values();
    test_input_frame_clear();
    test_input_frame_hash();
    test_input_frame_serialization();
    test_input_frame_copy();
    test_input_frame_to_string();
    test_frame_construction();
    test_player_values();
    test_feature_values();
    test_json_serialization();
    test_frame_comparison();
    test_frame_hash_generation();
    test_frame_copying();
    test_frame_file_io();
    
    std::cout << "All tests PASSED!" << std::endl;
    return 0;
} 