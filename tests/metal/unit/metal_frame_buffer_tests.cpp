#include <gtest/gtest.h>
#include "../../../src/burner/metal/metal_bridge.h"
#include "../../../src/burner/metal/metal_declarations.h"

// Test fixture for Metal frame buffer tests
class MetalFrameBufferTest : public ::testing::Test {
protected:
    // Mock frame buffer
    static const int TEST_WIDTH = 384;
    static const int TEST_HEIGHT = 224;
    static const int TEST_BPP = 32;
    
    UINT8* mockFrameBuffer;
    
    void SetUp() override {
        // Allocate test frame buffer
        mockFrameBuffer = new UINT8[TEST_WIDTH * TEST_HEIGHT * (TEST_BPP / 8)];
        
        // Initialize with test pattern
        for (int y = 0; y < TEST_HEIGHT; y++) {
            for (int x = 0; x < TEST_WIDTH; x++) {
                int offset = (y * TEST_WIDTH + x) * (TEST_BPP / 8);
                
                // Create a checkerboard pattern
                UINT8 value = ((x / 16) + (y / 16)) % 2 ? 0xFF : 0x00;
                
                mockFrameBuffer[offset] = value;         // Blue
                mockFrameBuffer[offset + 1] = value;     // Green
                mockFrameBuffer[offset + 2] = value;     // Red
                mockFrameBuffer[offset + 3] = 0xFF;      // Alpha
            }
        }
        
        // Set up global variables
        g_frameWidth = TEST_WIDTH;
        g_frameHeight = TEST_HEIGHT;
        pBurnDraw = mockFrameBuffer;
        nBurnBpp = TEST_BPP;
        nBurnPitch = TEST_WIDTH * (TEST_BPP / 8);
    }
    
    void TearDown() override {
        delete[] mockFrameBuffer;
        pBurnDraw = nullptr;
    }
};

// Test frame rendering function
TEST_F(MetalFrameBufferTest, RenderFrameTest) {
    // Create destination buffer
    UINT8* destBuffer = new UINT8[TEST_WIDTH * TEST_HEIGHT * 4]; // 32bpp RGBA
    memset(destBuffer, 0, TEST_WIDTH * TEST_HEIGHT * 4);
    
    // Set game initialized flag (needed for rendering)
    g_gameInitialized = true;
    
    // Call render function
    EXPECT_EQ(Metal_RenderFrame(destBuffer, TEST_WIDTH, TEST_HEIGHT), 0);
    
    // Verify some pixels in the destination buffer
    // We're not testing a CPS2 game, so it should do a direct copy
    for (int y = 0; y < TEST_HEIGHT; y += 32) {
        for (int x = 0; x < TEST_WIDTH; x += 32) {
            int srcOffset = (y * TEST_WIDTH + x) * 4;
            int destOffset = (y * TEST_WIDTH + x) * 4;
            
            // Compare pixels (BGRA format)
            EXPECT_EQ(destBuffer[destOffset], mockFrameBuffer[srcOffset]);
            EXPECT_EQ(destBuffer[destOffset + 1], mockFrameBuffer[srcOffset + 1]);
            EXPECT_EQ(destBuffer[destOffset + 2], mockFrameBuffer[srcOffset + 2]);
            EXPECT_EQ(destBuffer[destOffset + 3], mockFrameBuffer[srcOffset + 3]);
        }
    }
    
    // Clean up
    delete[] destBuffer;
    g_gameInitialized = false;
}

// Test frame rendering with invalid input
TEST_F(MetalFrameBufferTest, RenderFrameInvalidInputTest) {
    // Call with NULL buffer
    EXPECT_NE(Metal_RenderFrame(nullptr, TEST_WIDTH, TEST_HEIGHT), 0);
    
    // Call with invalid dimensions
    UINT8* destBuffer = new UINT8[TEST_WIDTH * TEST_HEIGHT * 4];
    EXPECT_NE(Metal_RenderFrame(destBuffer, 0, TEST_HEIGHT), 0);
    EXPECT_NE(Metal_RenderFrame(destBuffer, TEST_WIDTH, 0), 0);
    
    // Call with game not initialized
    g_gameInitialized = false;
    EXPECT_NE(Metal_RenderFrame(destBuffer, TEST_WIDTH, TEST_HEIGHT), 0);
    
    // Clean up
    delete[] destBuffer;
}

// Test frame rendering with smaller destination buffer
TEST_F(MetalFrameBufferTest, RenderFrameResizeTest) {
    // Create a smaller destination buffer
    int smallWidth = TEST_WIDTH / 2;
    int smallHeight = TEST_HEIGHT / 2;
    UINT8* destBuffer = new UINT8[smallWidth * smallHeight * 4];
    memset(destBuffer, 0, smallWidth * smallHeight * 4);
    
    // Set game initialized flag
    g_gameInitialized = true;
    
    // Call render function - should handle the size difference
    EXPECT_EQ(Metal_RenderFrame(destBuffer, smallWidth, smallHeight), 0);
    
    // Clean up
    delete[] destBuffer;
    g_gameInitialized = false;
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 