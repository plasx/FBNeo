#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <fstream>
#include <filesystem>
#include <cassert>

// Include the AIMemoryMapping
#include "../../src/ai/ai_memory_mapping.h"

namespace fs = std::filesystem;

// Mock external memory variables for testing
unsigned char* CpuRam = nullptr;
unsigned char* MainRam = nullptr;
unsigned char* Z80Ram = nullptr;
unsigned char* CpsRam = nullptr;
unsigned char* CpsFrg = nullptr;
unsigned char* CpsZRam = nullptr;

// Mock memory for testing
#define MOCK_MEM_SIZE (1024 * 1024)  // 1MB
unsigned char mockMemory[MOCK_MEM_SIZE];

// Mock memory read function for testing
extern "C" {
    uint8_t ReadMemory(uint32_t address) {
        if (address < MOCK_MEM_SIZE) {
            return mockMemory[address];
        }
        return 0;
    }
}

// Helper function to create a test mapping file
bool createTestMappingFile(const std::string& filePath) {
    std::string mappingJson = R"({
        "game_name": "Test Game",
        "architecture": "Test",
        "version": "1.0.0",
        "description": "Test mapping for AIMemoryMapping unit tests",
        "supported_roms": ["test"],
        
        "mappings": {
            "test_category": [
                {
                    "name": "test_byte",
                    "address": "0x100",
                    "type": "byte",
                    "description": "Test byte value",
                    "min_value": 0,
                    "max_value": 255
                },
                {
                    "name": "test_word",
                    "address": "0x200",
                    "type": "word",
                    "description": "Test word value",
                    "endianness": "little",
                    "min_value": 0,
                    "max_value": 65535
                },
                {
                    "name": "test_dword",
                    "address": "0x300",
                    "type": "dword",
                    "description": "Test dword value",
                    "endianness": "little",
                    "min_value": 0,
                    "max_value": 4294967295
                },
                {
                    "name": "test_bit",
                    "address": "0x400",
                    "type": "bit",
                    "description": "Test bit value",
                    "bit_position": 3
                },
                {
                    "name": "test_threshold",
                    "address": "0x500",
                    "type": "byte",
                    "description": "Test value with change threshold",
                    "min_value": 0,
                    "max_value": 100,
                    "change_threshold": 0.1
                }
            ]
        },
        
        "groups": {
            "test_group": [
                "test_byte",
                "test_word",
                "test_dword",
                "test_bit"
            ]
        }
    })";

    try {
        std::ofstream file(filePath);
        if (!file.is_open()) {
            std::cerr << "Failed to create test mapping file: " << filePath << std::endl;
            return false;
        }
        
        file << mappingJson;
        file.close();
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error creating test mapping file: " << e.what() << std::endl;
        return false;
    }
}

// Test cases for AIMemoryMapping
class AIMemoryMappingTester {
public:
    AIMemoryMappingTester() {
        // Initialize mock memory
        for (size_t i = 0; i < MOCK_MEM_SIZE; ++i) {
            mockMemory[i] = static_cast<unsigned char>(i & 0xFF);
        }
        
        // Point global memory pointers to mock memory
        CpuRam = mockMemory;
        MainRam = mockMemory;
        Z80Ram = mockMemory;
        CpsRam = mockMemory;
        CpsFrg = mockMemory;
        CpsZRam = mockMemory;
        
        // Create test directory if it doesn't exist
        fs::create_directories("test/tmp");
        
        // Create test mapping file
        m_mappingFile = "test/tmp/test_mapping.json";
        if (!createTestMappingFile(m_mappingFile)) {
            std::cerr << "Failed to create test mapping file." << std::endl;
            return;
        }
        
        std::cout << "AIMemoryMappingTester initialized" << std::endl;
    }
    
    ~AIMemoryMappingTester() {
        // Clean up test files
        if (fs::exists(m_mappingFile)) {
            fs::remove(m_mappingFile);
        }
        
        // Clean up mock memory
        memset(mockMemory, 0, MOCK_MEM_SIZE);
    }
    
    bool testLoadMappingFile() {
        AI::AIMemoryMapping mapping;
        bool success = mapping.loadFromFile(m_mappingFile);
        
        if (!success) {
            std::cerr << "Failed to load mapping file." << std::endl;
            return false;
        }
        
        if (!mapping.isLoaded()) {
            std::cerr << "Mapping is not loaded." << std::endl;
            return false;
        }
        
        if (mapping.getGameName() != "Test Game") {
            std::cerr << "Unexpected game name: " << mapping.getGameName() << std::endl;
            return false;
        }
        
        std::vector<std::string> categories = mapping.getCategories();
        if (categories.size() != 1 || categories[0] != "test_category") {
            std::cerr << "Unexpected categories." << std::endl;
            return false;
        }
        
        std::vector<std::string> groups = mapping.getGroups();
        if (groups.size() != 1 || groups[0] != "test_group") {
            std::cerr << "Unexpected groups." << std::endl;
            return false;
        }
        
        std::cout << "✅ testLoadMappingFile passed" << std::endl;
        return true;
    }
    
    bool testReadValues() {
        AI::AIMemoryMapping mapping;
        if (!mapping.loadFromFile(m_mappingFile)) {
            std::cerr << "Failed to load mapping file." << std::endl;
            return false;
        }
        
        // Set test values in mock memory
        mockMemory[0x100] = 42;
        mockMemory[0x200] = 0x34;
        mockMemory[0x201] = 0x12;  // Little-endian: 0x1234
        mockMemory[0x300] = 0x78;
        mockMemory[0x301] = 0x56;
        mockMemory[0x302] = 0x34;
        mockMemory[0x303] = 0x12;  // Little-endian: 0x12345678
        mockMemory[0x400] = 0x08;  // Bit 3 set
        
        // Test reading byte value
        AI::AIMemoryMapping::ValueType byteValue;
        if (!mapping.readValue("test_byte", byteValue)) {
            std::cerr << "Failed to read test_byte." << std::endl;
            return false;
        }
        
        if (!std::holds_alternative<uint8_t>(byteValue) || std::get<uint8_t>(byteValue) != 42) {
            std::cerr << "Unexpected test_byte value." << std::endl;
            return false;
        }
        
        // Test reading word value
        AI::AIMemoryMapping::ValueType wordValue;
        if (!mapping.readValue("test_word", wordValue)) {
            std::cerr << "Failed to read test_word." << std::endl;
            return false;
        }
        
        if (!std::holds_alternative<uint16_t>(wordValue) || std::get<uint16_t>(wordValue) != 0x1234) {
            std::cerr << "Unexpected test_word value." << std::endl;
            return false;
        }
        
        // Test reading dword value
        AI::AIMemoryMapping::ValueType dwordValue;
        if (!mapping.readValue("test_dword", dwordValue)) {
            std::cerr << "Failed to read test_dword." << std::endl;
            return false;
        }
        
        if (!std::holds_alternative<uint32_t>(dwordValue) || std::get<uint32_t>(dwordValue) != 0x12345678) {
            std::cerr << "Unexpected test_dword value." << std::endl;
            return false;
        }
        
        // Test reading bit value
        AI::AIMemoryMapping::ValueType bitValue;
        if (!mapping.readValue("test_bit", bitValue)) {
            std::cerr << "Failed to read test_bit." << std::endl;
            return false;
        }
        
        if (!std::holds_alternative<bool>(bitValue) || std::get<bool>(bitValue) != true) {
            std::cerr << "Unexpected test_bit value." << std::endl;
            return false;
        }
        
        std::cout << "✅ testReadValues passed" << std::endl;
        return true;
    }
    
    bool testNormalizedValues() {
        AI::AIMemoryMapping mapping;
        if (!mapping.loadFromFile(m_mappingFile)) {
            std::cerr << "Failed to load mapping file." << std::endl;
            return false;
        }
        
        // Set test values in mock memory
        mockMemory[0x100] = 127;  // 50% of 0-255 range
        
        // Test reading normalized value
        float normalizedValue = 0.0f;
        if (!mapping.readNormalizedValue("test_byte", normalizedValue)) {
            std::cerr << "Failed to read normalized test_byte." << std::endl;
            return false;
        }
        
        // Should be close to 0.5 (127/255)
        if (std::abs(normalizedValue - 0.498f) > 0.01f) {
            std::cerr << "Unexpected normalized value: " << normalizedValue << std::endl;
            return false;
        }
        
        std::cout << "✅ testNormalizedValues passed" << std::endl;
        return true;
    }
    
    bool testStateChangeDetection() {
        AI::AIMemoryMapping mapping;
        if (!mapping.loadFromFile(m_mappingFile)) {
            std::cerr << "Failed to load mapping file." << std::endl;
            return false;
        }
        
        // Initial state
        mockMemory[0x100] = 10;
        mockMemory[0x500] = 50;  // Value with threshold
        
        // First refresh should not report changes (initial state)
        mapping.refreshValues(1);
        
        // Change the value
        mockMemory[0x100] = 20;
        
        // Second refresh should detect the change
        mapping.refreshValues(2);
        const std::vector<std::string>& changedMappings = mapping.getChangedMappings();
        
        if (std::find(changedMappings.begin(), changedMappings.end(), "test_byte") == changedMappings.end()) {
            std::cerr << "Change in test_byte not detected." << std::endl;
            return false;
        }
        
        // Test threshold-based detection
        // Small change (less than 10%)
        mockMemory[0x500] = 54;  // 4% change from 50 (threshold is 10%)
        mapping.refreshValues(3);
        const std::vector<std::string>& minorChanges = mapping.getChangedMappings();
        
        // Should be detected as a change, but not as a significant change
        if (std::find(minorChanges.begin(), minorChanges.end(), "test_threshold") == minorChanges.end()) {
            std::cerr << "Minor change in test_threshold not detected." << std::endl;
            return false;
        }
        
        std::vector<std::string> significantChanges = mapping.getSignificantChanges(0.1);  // 10% threshold
        if (std::find(significantChanges.begin(), significantChanges.end(), "test_threshold") != significantChanges.end()) {
            std::cerr << "Minor change incorrectly reported as significant." << std::endl;
            return false;
        }
        
        // Significant change (more than 10%)
        mockMemory[0x500] = 70;  // 20% change from 50
        mapping.refreshValues(4);
        significantChanges = mapping.getSignificantChanges(0.1);
        
        if (std::find(significantChanges.begin(), significantChanges.end(), "test_threshold") == significantChanges.end()) {
            std::cerr << "Significant change not detected." << std::endl;
            return false;
        }
        
        // Test value history
        std::vector<double> values;
        std::vector<int> frames;
        if (!mapping.getValueHistory("test_threshold", values, frames, 3)) {
            std::cerr << "Failed to get history." << std::endl;
            return false;
        }
        
        if (values.size() != 3 || frames.size() != 3) {
            std::cerr << "Unexpected history size." << std::endl;
            return false;
        }
        
        // The history should contain the 3 most recent values: 50, 54, 70
        if (values[0] != 50 || values[1] != 54 || values[2] != 70) {
            std::cerr << "Unexpected history values: " << values[0] << ", " << values[1] << ", " << values[2] << std::endl;
            return false;
        }
        
        if (frames[0] != 2 || frames[1] != 3 || frames[2] != 4) {
            std::cerr << "Unexpected history frames: " << frames[0] << ", " << frames[1] << ", " << frames[2] << std::endl;
            return false;
        }
        
        std::cout << "✅ testStateChangeDetection passed" << std::endl;
        return true;
    }
    
    bool testValueExport() {
        AI::AIMemoryMapping mapping;
        if (!mapping.loadFromFile(m_mappingFile)) {
            std::cerr << "Failed to load mapping file." << std::endl;
            return false;
        }
        
        // Set test values
        mockMemory[0x100] = 42;
        mockMemory[0x200] = 0x34;
        mockMemory[0x201] = 0x12;
        
        // Refresh to load values
        mapping.refreshValues(1);
        
        // Export values to JSON
        std::string jsonExport = mapping.exportValuesToJson();
        
        // Create a new mapping and import values
        AI::AIMemoryMapping importMapping;
        if (!importMapping.loadFromFile(m_mappingFile)) {
            std::cerr << "Failed to load mapping file for import." << std::endl;
            return false;
        }
        
        // Import values
        if (!importMapping.importValuesFromJson(jsonExport)) {
            std::cerr << "Failed to import values." << std::endl;
            return false;
        }
        
        // Verify imported values
        AI::AIMemoryMapping::ValueType byteValue;
        if (!importMapping.readValue("test_byte", byteValue)) {
            std::cerr << "Failed to read imported test_byte." << std::endl;
            return false;
        }
        
        if (!std::holds_alternative<uint8_t>(byteValue) || std::get<uint8_t>(byteValue) != 42) {
            std::cerr << "Unexpected imported test_byte value." << std::endl;
            return false;
        }
        
        std::cout << "✅ testValueExport passed" << std::endl;
        return true;
    }
    
    bool runAllTests() {
        bool success = true;
        
        success &= testLoadMappingFile();
        success &= testReadValues();
        success &= testNormalizedValues();
        success &= testStateChangeDetection();
        success &= testValueExport();
        
        return success;
    }

private:
    std::string m_mappingFile;
};

int main() {
    std::cout << "Running AIMemoryMapping tests..." << std::endl;
    
    AIMemoryMappingTester tester;
    bool success = tester.runAllTests();
    
    if (success) {
        std::cout << "All tests passed successfully!" << std::endl;
        return 0;
    } else {
        std::cerr << "Some tests failed." << std::endl;
        return 1;
    }
} 