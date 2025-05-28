#include <gtest/gtest.h>
#include "../../../src/burner/metal/cps2_rom_loader.h"
#include <string>

// Mock ROM path for testing
const char* TEST_ROM_PATH = "tests/metal/fixtures/test_rom.zip";

// Test fixture for CPS2 ROM Loader tests
class CPS2RomLoaderTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize the ROM loader
        CPS2_InitROMLoader();
        
        // Create test directories if needed
        system("mkdir -p tests/metal/fixtures");
        
        // Create a minimal mock ROM ZIP file for testing if it doesn't exist
        struct stat buffer;
        if (stat(TEST_ROM_PATH, &buffer) != 0) {
            CreateMockRomZip();
        }
    }
    
    void TearDown() override {
        // Clean up ROM loader
        CPS2_CleanupROMFiles();
        CPS2_ShutdownROMLoader();
    }
    
    void CreateMockRomZip() {
        // Create a temporary directory
        system("mkdir -p /tmp/mock_cps2_rom");
        
        // Create dummy ROM files
        FILE* f = fopen("/tmp/mock_cps2_rom/mvc.03", "wb");
        if (f) {
            const char data[] = "MOCKCPS2ROM";
            fwrite(data, 1, sizeof(data), f);
            fclose(f);
        }
        
        f = fopen("/tmp/mock_cps2_rom/mvc.04", "wb");
        if (f) {
            const char data[] = "MOCKCPS2ROM";
            fwrite(data, 1, sizeof(data), f);
            fclose(f);
        }
        
        // Create a ZIP file from these dummy files
        std::string cmd = "cd /tmp/mock_cps2_rom && zip -q ";
        cmd += TEST_ROM_PATH;
        cmd += " *";
        system(cmd.c_str());
        
        // Clean up temporary directory
        system("rm -rf /tmp/mock_cps2_rom");
    }
};

// Test initialization and shutdown
TEST_F(CPS2RomLoaderTest, InitShutdownTest) {
    // Should already be initialized in SetUp()
    // Try reinitializing, which should be safe
    EXPECT_TRUE(CPS2_InitROMLoader());
    
    // Shutdown
    CPS2_ShutdownROMLoader();
    
    // Reinitialize for other tests
    EXPECT_TRUE(CPS2_InitROMLoader());
}

// Test getting ROM info
TEST_F(CPS2RomLoaderTest, GetROMInfoTest) {
    // When no ROM is loaded, should return nullptr
    EXPECT_EQ(CPS2_GetROMInfo(), nullptr);
    
    // Try loading a ROM (will be mocked in further tests)
    // Here we just ensure the function exists and doesn't crash
    CPS2_LoadROMSet("mvsc");
    
    // Clean up
    CPS2_CleanupROMFiles();
}

// Test getting supported games list
TEST_F(CPS2RomLoaderTest, GetSupportedGamesTest) {
    // Get the list of supported games
    std::vector<CPS2GameInfo> games = CPS2_GetSupportedGames();
    
    // Should have at least a few games
    EXPECT_GT(games.size(), 0);
    
    // Check for a few known games
    bool foundMvsc = false;
    bool foundSfa3 = false;
    
    for (const auto& game : games) {
        if (game.id == "mvsc") foundMvsc = true;
        if (game.id == "sfa3") foundSfa3 = true;
    }
    
    EXPECT_TRUE(foundMvsc) << "Marvel vs. Capcom should be in supported games list";
    EXPECT_TRUE(foundSfa3) << "Street Fighter Alpha 3 should be in supported games list";
}

// Test ROM loading with invalid parameters
TEST_F(CPS2RomLoaderTest, LoadROMInvalidParamsTest) {
    // Try loading with nullptr
    EXPECT_FALSE(CPS2_LoadROMSet(nullptr));
    
    // Try loading with empty string
    EXPECT_FALSE(CPS2_LoadROMSet(""));
    
    // Try loading non-existent ROM
    EXPECT_FALSE(CPS2_LoadROMSet("nonexistent_rom_xyzabc"));
}

// Test ROM file operations with invalid parameters
TEST_F(CPS2RomLoaderTest, ROMFileInvalidParamsTest) {
    // Get file with nullptr
    EXPECT_EQ(CPS2_GetROMFile(nullptr), nullptr);
    
    // Get non-existent file
    EXPECT_EQ(CPS2_GetROMFile("nonexistent.rom"), nullptr);
    
    // Load ROM data with nullptr name
    UINT8 buffer[100];
    EXPECT_FALSE(CPS2_LoadROMData(nullptr, buffer, sizeof(buffer)));
    
    // Load ROM data with nullptr buffer
    EXPECT_FALSE(CPS2_LoadROMData("test.rom", nullptr, 100));
    
    // Load ROM data with zero size
    EXPECT_FALSE(CPS2_LoadROMData("test.rom", buffer, 0));
}

// Mock class for testing ROM verification
class MockVerifier : public ::testing::Test {
public:
    static bool VerifyCPS2ROM(const char* romPath, ROMVerify::ROMSetVerification& result) {
        // Simple mock that always returns success
        result.setName = "mock_rom";
        result.complete = true;
        result.playable = true;
        
        ROMVerify::VerificationResult mockResult;
        mockResult.success = true;
        mockResult.romName = "mvc.03";
        mockResult.expectedChecksum = "12345678";
        mockResult.actualChecksum = "12345678";
        mockResult.errorMessage = "";
        
        result.results.push_back(mockResult);
        
        return true;
    }
};

// Test ROM verification function
TEST_F(CPS2RomLoaderTest, VerifyROMTest) {
    // This is a bit tricky to test without a real ROM
    // We'll just check that the function exists and doesn't crash
    EXPECT_FALSE(CPS2_VerifyLoadedROM()); // No ROM loaded yet
    
    // In a real test, we'd need to mock the verification functions
    // and load an actual ROM, but that's complex for this example
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 