#include <gtest/gtest.h>
#include "../../../src/burner/metal/metal_bridge.h"
#include "../../../src/burner/metal/metal_cps2_renderer.h"
#include "../../../src/burner/metal/cps2_rom_loader.h"
#include "../../../src/burner/metal/rom_verify.h"

// External functions from FBNeo core
extern "C" {
    INT32 BurnLibInit();
    INT32 BurnLibExit();
    INT32 BurnDrvInit(INT32 nDrvNum);
    INT32 BurnDrvExit();
    INT32 BurnDrvGetScreenSize(INT32* pnWidth, INT32* pnHeight);
    char* BurnDrvGetTextA(UINT32 i);
    UINT32 BurnDrvGetHardwareCode();
}

// Test fixture for Metal-FBNeo integration tests
class MetalFBNeoIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize FBNeo core
        ASSERT_EQ(BurnLibInit(), 0);
        
        // Initialize Metal bridge
        ASSERT_EQ(Metal_Init(), 0);
        
        // Initialize CPS2 ROM loader
        ASSERT_TRUE(CPS2_InitROMLoader());
    }
    
    void TearDown() override {
        // Clean up CPS2 ROM loader
        CPS2_CleanupROMFiles();
        CPS2_ShutdownROMLoader();
        
        // Clean up Metal bridge
        Metal_Exit();
        
        // Clean up FBNeo core
        BurnLibExit();
    }
    
    // Helper to find a driver by name
    int FindDriverIndex(const char* drvName) {
        extern INT32 BurnDrvGetIndexByName(const char* szName);
        return BurnDrvGetIndexByName(drvName);
    }
};

// Test Metal initialization with FBNeo core
TEST_F(MetalFBNeoIntegrationTest, InitializationTest) {
    // Metal_Init should have been called in SetUp
    // Now check if the frame buffer was correctly allocated
    ASSERT_NE(pBurnDraw_Metal, nullptr);
    ASSERT_GT(nBurnPitch_Metal, 0);
    ASSERT_EQ(nBurnBpp_Metal, 32); // Should be 32-bit color
}

// Test Metal running a frame
TEST_F(MetalFBNeoIntegrationTest, RunFrameTest) {
    // Mock game initialization
    g_gameInitialized = true;
    
    // Run a frame without drawing
    ASSERT_EQ(Metal_RunFrame(0), 0);
    
    // Run a frame with drawing
    ASSERT_EQ(Metal_RunFrame(1), 0);
    
    // Reset state
    g_gameInitialized = false;
}

// Test ROM path detection
TEST_F(MetalFBNeoIntegrationTest, ROMPathDetectionTest) {
    // Detect ROM paths
    int numPaths = DetectRomPaths();
    
    // Should find at least one path
    ASSERT_GT(numPaths, 0);
    
    // Get paths and check they're non-empty
    const std::vector<std::string>& paths = GetRomPaths();
    ASSERT_EQ(paths.size(), numPaths);
    
    for (const auto& path : paths) {
        ASSERT_FALSE(path.empty());
    }
}

// Test driver selection
TEST_F(MetalFBNeoIntegrationTest, DriverSelectionTest) {
    // Find CPS2 driver indices
    int mvscIndex = FindDriverIndex("mvsc");
    int sfa3Index = FindDriverIndex("sfa3");
    int ssf2tIndex = FindDriverIndex("ssf2t");
    
    // At least one of these should be valid
    ASSERT_TRUE(mvscIndex >= 0 || sfa3Index >= 0 || ssf2tIndex >= 0);
    
    // For valid drivers, test getting hardware info
    if (mvscIndex >= 0) {
        extern void BurnDrvSelect(UINT32 i);
        BurnDrvSelect(mvscIndex);
        
        // Should be CPS2 hardware
        UINT32 hwCode = BurnDrvGetHardwareCode();
        // Check for CPS2 flag (actual value depends on FBNeo core)
        // This is a simplified check - real code would use proper constants
        ASSERT_TRUE((hwCode & 0x0000FF00) == 0x00001100); 
    }
}

// Test Metal CPS2 renderer initialization with FBNeo core
TEST_F(MetalFBNeoIntegrationTest, CPS2RendererTest) {
    // Initialize CPS2 renderer
    ASSERT_EQ(Metal_CPS2_InitRenderer(), 0);
    
    // Get palette buffer
    UINT32* palette = Metal_CPS2_GetPaletteBuffer();
    ASSERT_NE(palette, nullptr);
    
    // Clean up
    Metal_CPS2_ExitRenderer();
}

// Test Metal rendering dimensions with FBNeo core
TEST_F(MetalFBNeoIntegrationTest, RenderingDimensionsTest) {
    // Get screen size from Metal bridge
    int width = g_frameWidth;
    int height = g_frameHeight;
    
    // These should match standard CPS2 dimensions
    ASSERT_EQ(width, 384);
    ASSERT_EQ(height, 224);
    
    // Try getting dimensions via CPS2 renderer
    float scale = 0.0f;
    Metal_CPS2_GetDimensions(&width, &height, &scale);
    
    // Should still be standard dimensions (no ROM loaded)
    ASSERT_EQ(width, 384);
    ASSERT_EQ(height, 224);
    ASSERT_GT(scale, 0.0f);
}

// Test ROM verification integration
TEST_F(MetalFBNeoIntegrationTest, ROMVerificationTest) {
    // Create a mock ROM verification test
    ROMVerify::ROMSetVerification result;
    
    // Call verification function with invalid path (should fail gracefully)
    bool verified = ROMVerify::VerifyCPS2ROM("nonexistent_rom.zip", result);
    
    // Should fail but not crash
    ASSERT_FALSE(verified);
    ASSERT_FALSE(result.playable);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 