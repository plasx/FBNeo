#include <gtest/gtest.h>
#include "../../../src/burner/metal/metal_cps2_renderer.h"
#include "../../../src/burner/metal/metal_declarations.h"

// Mock external dependencies
extern "C" {
    // Mock palette data
    UINT32 MockCpsPal[1024];
}

// Test fixture for Metal palette tests
class MetalPaletteTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize mock palette with known values
        for (int i = 0; i < 1024; i++) {
            // CPS2 format: 0x0RGB
            MockCpsPal[i] = (((i % 16) << 8) |      // R component (4 bits)
                            (((i / 16) % 16) << 4) | // G component (4 bits)
                            ((i / 256) % 16));       // B component (4 bits)
        }
        
        // Redirect CpsPal reference to our mock data
        CpsPal = MockCpsPal;
    }
    
    void TearDown() override {
        CpsPal = nullptr;
    }
};

// Test palette initialization
TEST_F(MetalPaletteTest, InitializationTest) {
    // Ensure initialization succeeds
    EXPECT_EQ(Metal_CPS2_InitRenderer(), 0);
    
    // Get the palette buffer
    UINT32* palette = Metal_CPS2_GetPaletteBuffer();
    EXPECT_NE(palette, nullptr);
    
    // Clean up
    Metal_CPS2_ExitRenderer();
}

// Test palette conversion
TEST_F(MetalPaletteTest, ConversionTest) {
    // Initialize renderer
    Metal_CPS2_InitRenderer();
    
    // Update the palette
    Metal_CPS2_UpdatePalette();
    
    // Get the palette buffer
    UINT32* palette = Metal_CPS2_GetPaletteBuffer();
    EXPECT_NE(palette, nullptr);
    
    // Check a few converted entries
    // Entry 0: R=0, G=0, B=0 should convert to 0xFF000000 (opaque black)
    EXPECT_EQ(palette[0], 0xFF000000);
    
    // Entry 15: R=15, G=0, B=0 should convert to 0xFFFF0000 (opaque red)
    EXPECT_EQ(palette[15], 0xFFFF0000);
    
    // Entry 240: R=0, G=15, B=0 should convert to 0xFF00FF00 (opaque green)
    EXPECT_EQ(palette[240], 0xFF00FF00);
    
    // Entry 255: R=15, G=15, B=0 should convert to 0xFFFFFF00 (opaque yellow)
    EXPECT_EQ(palette[255], 0xFFFFFF00);
    
    // Clean up
    Metal_CPS2_ExitRenderer();
}

// Test palette update flag
TEST_F(MetalPaletteTest, UpdateFlagTest) {
    // Initialize renderer
    Metal_CPS2_InitRenderer();
    
    // Update the palette
    Metal_CPS2_UpdatePalette();
    
    // Flag should be set after update
    EXPECT_TRUE(Metal_CPS2_IsPaletteUpdated());
    
    // Flag should be cleared after checking
    EXPECT_FALSE(Metal_CPS2_IsPaletteUpdated());
    
    // Clean up
    Metal_CPS2_ExitRenderer();
}

// Test with invalid inputs
TEST_F(MetalPaletteTest, InvalidInputTest) {
    // Initialize renderer
    Metal_CPS2_InitRenderer();
    
    // Temporarily set CpsPal to nullptr
    CpsPal = nullptr;
    
    // Update should handle nullptr gracefully
    Metal_CPS2_UpdatePalette();
    
    // Flag should not be set
    EXPECT_FALSE(Metal_CPS2_IsPaletteUpdated());
    
    // Restore CpsPal
    CpsPal = MockCpsPal;
    
    // Clean up
    Metal_CPS2_ExitRenderer();
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 