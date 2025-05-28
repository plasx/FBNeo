#include <gtest/gtest.h>
#include <chrono>
#include <vector>
#include <numeric>
#include <thread>
#include "../../../src/burner/metal/metal_bridge.h"
#include "../../../src/burner/metal/metal_cps2_renderer.h"

// Performance test fixture
class MetalPerformanceTest : public ::testing::Test {
protected:
    static const int TEST_WIDTH = 384;
    static const int TEST_HEIGHT = 224;
    static const int TEST_BUFFER_SIZE = TEST_WIDTH * TEST_HEIGHT * 4;
    static const int NUM_FRAMES = 60;  // Number of frames to test
    
    UINT8* mockSourceBuffer;
    UINT8* mockDestBuffer;
    
    void SetUp() override {
        // Initialize Metal bridge
        Metal_Init();
        
        // Create test buffers
        mockSourceBuffer = new UINT8[TEST_BUFFER_SIZE];
        mockDestBuffer = new UINT8[TEST_BUFFER_SIZE];
        
        // Initialize source buffer with test pattern
        for (int i = 0; i < TEST_BUFFER_SIZE; i += 4) {
            mockSourceBuffer[i] = i & 0xFF;        // B
            mockSourceBuffer[i + 1] = (i >> 8) & 0xFF; // G
            mockSourceBuffer[i + 2] = (i >> 16) & 0xFF; // R
            mockSourceBuffer[i + 3] = 0xFF;        // A
        }
        
        // Set up global variables for testing
        g_frameWidth = TEST_WIDTH;
        g_frameHeight = TEST_HEIGHT;
        g_gameInitialized = true;
        pBurnDraw = mockSourceBuffer;
        nBurnBpp = 32;
        nBurnPitch = TEST_WIDTH * 4;
        
        // Initialize CPS2 renderer
        Metal_CPS2_InitRenderer();
    }
    
    void TearDown() override {
        // Clean up
        Metal_CPS2_ExitRenderer();
        Metal_Exit();
        
        // Free test buffers
        delete[] mockSourceBuffer;
        delete[] mockDestBuffer;
        
        // Reset global state
        g_gameInitialized = false;
        pBurnDraw = nullptr;
    }
    
    // Helper to measure function execution time in milliseconds
    template<typename Func>
    double MeasureExecutionTime(Func func, int iterations = 1) {
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < iterations; i++) {
            func();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        
        return static_cast<double>(duration) / iterations / 1000.0; // Convert to milliseconds
    }
};

// Test Metal frame rendering performance
TEST_F(MetalPerformanceTest, RenderFramePerformance) {
    std::vector<double> frameTimes;
    frameTimes.reserve(NUM_FRAMES);
    
    // Warm-up run
    Metal_RenderFrame(mockDestBuffer, TEST_WIDTH, TEST_HEIGHT);
    
    // Measure frame rendering times
    for (int i = 0; i < NUM_FRAMES; i++) {
        double frameTime = MeasureExecutionTime([&]() {
            Metal_RenderFrame(mockDestBuffer, TEST_WIDTH, TEST_HEIGHT);
        });
        
        frameTimes.push_back(frameTime);
        
        // Small delay to simulate frame timing
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    
    // Calculate statistics
    double avgFrameTime = std::accumulate(frameTimes.begin(), frameTimes.end(), 0.0) / frameTimes.size();
    
    // Find min and max
    double minFrameTime = *std::min_element(frameTimes.begin(), frameTimes.end());
    double maxFrameTime = *std::max_element(frameTimes.begin(), frameTimes.end());
    
    // Calculate standard deviation
    double sumSquaredDiff = 0.0;
    for (double time : frameTimes) {
        double diff = time - avgFrameTime;
        sumSquaredDiff += diff * diff;
    }
    double stdDev = std::sqrt(sumSquaredDiff / frameTimes.size());
    
    // Average should be reasonably fast
    EXPECT_LT(avgFrameTime, 10.0) << "Average frame render time is too slow";
    
    // Variance should be reasonably low
    EXPECT_LT(stdDev, 5.0) << "Frame render times are too variable";
    
    // Log performance results
    std::cout << "Frame rendering performance:" << std::endl;
    std::cout << "  Average: " << avgFrameTime << " ms" << std::endl;
    std::cout << "  Min: " << minFrameTime << " ms" << std::endl;
    std::cout << "  Max: " << maxFrameTime << " ms" << std::endl;
    std::cout << "  StdDev: " << stdDev << " ms" << std::endl;
}

// Test palette conversion performance
TEST_F(MetalPerformanceTest, PaletteConversionPerformance) {
    // Initialize some fake palette data
    UINT32 mockPalette[1024];
    for (int i = 0; i < 1024; i++) {
        mockPalette[i] = i; // Simple pattern
    }
    
    // Set the external CpsPal pointer to our mock data
    CpsPal = mockPalette;
    
    // Measure palette update performance (100 iterations)
    double updateTime = MeasureExecutionTime([&]() {
        Metal_CPS2_UpdatePalette();
    }, 100);
    
    // Reset CpsPal
    CpsPal = nullptr;
    
    // Should be reasonably fast
    EXPECT_LT(updateTime, 1.0) << "Palette conversion is too slow";
    
    // Log performance results
    std::cout << "Palette conversion performance:" << std::endl;
    std::cout << "  Average: " << updateTime << " ms" << std::endl;
}

// Test running multiple frames performance
TEST_F(MetalPerformanceTest, RunFramePerformance) {
    std::vector<double> frameTimes;
    frameTimes.reserve(NUM_FRAMES);
    
    // Warm-up run
    Metal_RunFrame(1);
    
    // Measure frame times
    for (int i = 0; i < NUM_FRAMES; i++) {
        double frameTime = MeasureExecutionTime([&]() {
            Metal_RunFrame(1);
        });
        
        frameTimes.push_back(frameTime);
    }
    
    // Calculate statistics
    double avgFrameTime = std::accumulate(frameTimes.begin(), frameTimes.end(), 0.0) / frameTimes.size();
    double minFrameTime = *std::min_element(frameTimes.begin(), frameTimes.end());
    double maxFrameTime = *std::max_element(frameTimes.begin(), frameTimes.end());
    
    // Log performance results
    std::cout << "Frame running performance:" << std::endl;
    std::cout << "  Average: " << avgFrameTime << " ms" << std::endl;
    std::cout << "  Min: " << minFrameTime << " ms" << std::endl;
    std::cout << "  Max: " << maxFrameTime << " ms" << std::endl;
    
    // Calculate effective FPS
    double fps = 1000.0 / avgFrameTime;
    std::cout << "  Estimated FPS: " << fps << std::endl;
    
    // Should be at least 30 FPS
    EXPECT_GT(fps, 30.0) << "Frame rate is too low";
}

// Stress test with different resolutions
TEST_F(MetalPerformanceTest, ResolutionScalingPerformance) {
    struct ResolutionTest {
        int width;
        int height;
        std::string name;
    };
    
    std::vector<ResolutionTest> resolutions = {
        {320, 240, "QVGA"},
        {384, 224, "CPS2 Standard"},
        {512, 384, "512x384"},
        {640, 480, "VGA"},
        {800, 600, "SVGA"}
    };
    
    std::cout << "Resolution scaling performance:" << std::endl;
    
    for (const auto& res : resolutions) {
        // Resize destination buffer if needed
        int bufferSize = res.width * res.height * 4;
        UINT8* resizedBuffer = new UINT8[bufferSize];
        
        // Measure rendering time for this resolution
        double renderTime = MeasureExecutionTime([&]() {
            Metal_RenderFrame(resizedBuffer, res.width, res.height);
        }, 10); // Do 10 iterations
        
        std::cout << "  " << res.name << " (" << res.width << "x" << res.height << "): " 
                 << renderTime << " ms" << std::endl;
        
        // Higher resolutions should still be reasonably fast
        EXPECT_LT(renderTime, 15.0) << "Rendering at " << res.name << " is too slow";
        
        delete[] resizedBuffer;
    }
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 