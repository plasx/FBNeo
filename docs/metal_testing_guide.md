# FBNeo Metal Testing Guide

This document provides comprehensive information on testing the Metal implementation in FBNeo. It covers unit testing, integration testing, performance testing, and how to set up a testing environment for continuous integration.

## Testing Architecture

The FBNeo Metal testing suite is organized into three main categories:

1. **Unit Tests** - Test individual components in isolation
2. **Integration Tests** - Test interactions between components
3. **Performance Tests** - Benchmark critical operations

```
tests/metal/
├── unit/              # Unit tests
├── integration/       # Integration tests
├── performance/       # Performance benchmarks
├── fixtures/          # Test data and resources
├── build/             # Compiled test executables
├── reports/           # Test result reports
└── run_tests.sh       # Main test runner script
```

## Setting Up the Testing Environment

### Prerequisites

To run the Metal tests, you'll need:

- macOS 10.15 or newer
- Xcode 12.0 or newer with Command Line Tools
- Google Test framework (`brew install googletest`)
- pkg-config (`brew install pkg-config`)

### Installing Dependencies

```bash
# Install Homebrew if needed
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install required dependencies
brew install cmake pkg-config googletest
```

### Building the Test Suite

```bash
# Navigate to the FBNeo directory
cd /path/to/FBNeo

# Create necessary directories
mkdir -p tests/metal/build tests/metal/reports tests/metal/fixtures

# Build the tests using the makefile target
make -f makefile.metal test-clean
make -f makefile.metal test-all
```

## Running Tests

### Basic Test Execution

```bash
# Run all unit tests
make -f makefile.metal test

# Run integration tests
make -f makefile.metal test-integration

# Run performance tests
make -f makefile.metal test-performance
```

### Using the Test Runner Script Directly

```bash
# Navigate to the test directory
cd tests/metal

# Run all tests
./run_tests.sh

# Clean previous builds and run all tests
./run_tests.sh --clean

# Run with performance tests included
./run_tests.sh --with-perf
```

### Test Output

Tests generate output in two formats:

1. **Console output** - Shows test progress and immediate results
2. **XML reports** - Detailed results in `tests/metal/reports/*.xml`

Example console output:
```
==== Unit Tests ====
Compiling metal_palette_tests...
✓ Compiled metal_palette_tests
Running metal_palette_tests...
✓ PASSED metal_palette_tests

==== Test Summary ====
All tests PASSED!
```

## Unit Tests

Unit tests focus on testing individual components in isolation. Each test should be small, fast, and focused on a specific function or feature.

### Writing Unit Tests

Unit tests use the Google Test framework. Here's an example of a unit test for the palette conversion function:

```cpp
#include <gtest/gtest.h>
#include "../../../src/burner/metal/metal_cps2_renderer.h"

// Test fixture for palette conversion tests
class PaletteConversionTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up test data
    }
    
    void TearDown() override {
        // Clean up test data
    }
};

// Test basic palette conversion
TEST_F(PaletteConversionTest, ConvertsPaletteEntry) {
    // Example CPS2 palette entry (RRRRGGGGBBBB0000)
    UINT32 cpsEntry = 0x0FB00;  // Bright red
    
    // Convert to Metal format
    UINT32 metalEntry = CPS2_ConvertPaletteEntry(cpsEntry);
    
    // Expect ARGB format with full alpha
    EXPECT_EQ(metalEntry, 0xFFFF0000);
}
```

### Key Unit Test Areas

Unit tests cover:

1. **Palette Conversion** - `metal_palette_tests.cpp`
2. **Frame Buffer Management** - `metal_frame_buffer_tests.cpp`
3. **ROM Loading** - `metal_cps2_loader_tests.cpp`
4. **CPS2 Rendering** - `metal_cps2_renderer_tests.cpp`

## Integration Tests

Integration tests verify that different components work together correctly.

### Writing Integration Tests

Integration tests typically inject mock objects and verify behavior across component boundaries:

```cpp
#include <gtest/gtest.h>
#include "../../../src/burner/metal/metal_bridge.h"
#include "../../../src/burner/metal/metal_cps2_renderer.h"

// Test fixture for Metal-FBNeo integration
class MetalFBNeoIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize FBNeo core and Metal bridge
        BurnLibInit();
        Metal_Init();
    }
    
    void TearDown() override {
        // Clean up
        Metal_Exit();
        BurnLibExit();
    }
};

// Test that Metal bridge properly initializes
TEST_F(MetalFBNeoIntegrationTest, InitializationTest) {
    // Test initialization succeeded
    ASSERT_NE(pBurnDraw_Metal, nullptr);
    ASSERT_GT(nBurnPitch_Metal, 0);
}
```

### Key Integration Test Areas

Integration tests cover:

1. **Metal Bridge and FBNeo Core** - `metal_fbneo_integration_tests.cpp`
2. **CPS2 Renderer and ROM Loader** - `metal_cps2_integration_tests.cpp`
3. **Input Handling** - `metal_input_integration_tests.cpp`

## Performance Tests

Performance tests measure the speed and efficiency of critical operations to identify bottlenecks.

### Writing Performance Tests

Performance tests measure execution time and compare against thresholds:

```cpp
#include <gtest/gtest.h>
#include <chrono>
#include "../../../src/burner/metal/metal_cps2_renderer.h"

// Test fixture for performance tests
class MetalPerformanceTest : public ::testing::Test {
protected:
    // Helper to measure execution time
    double MeasureExecutionTimeMs(std::function<void()> func) {
        auto start = std::chrono::high_resolution_clock::now();
        func();
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::milli>(end - start).count();
    }
};

// Test palette conversion performance
TEST_F(MetalPerformanceTest, PaletteConversionPerformance) {
    // Create test palette
    UINT32 mockPalette[1024];
    for (int i = 0; i < 1024; i++) {
        mockPalette[i] = i;
    }
    
    // Measure conversion time for full palette
    double timeMs = MeasureExecutionTimeMs([&]() {
        for (int i = 0; i < 1024; i++) {
            CPS2_ConvertPaletteEntry(mockPalette[i]);
        }
    });
    
    // Palette conversion should be fast
    EXPECT_LT(timeMs, 1.0) << "Palette conversion is too slow";
    std::cout << "Palette conversion time: " << timeMs << " ms" << std::endl;
}
```

### Key Performance Test Areas

Performance tests cover:

1. **Frame Rendering** - `metal_render_performance_tests.cpp`
2. **Palette Conversion** - `metal_palette_performance_tests.cpp` 
3. **Resolution Scaling** - `metal_scaling_performance_tests.cpp`

## Mocks and Test Fixtures

The test suite uses mock objects and fixtures to isolate components and provide consistent test data.

### Mock Objects

Mock objects simulate real components with controlled behavior:

```cpp
// Mock CPS2 ROM loader
class MockCPS2ROMLoader {
public:
    MOCK_METHOD(bool, LoadROMSet, (const char* name));
    MOCK_METHOD(bool, VerifyROM, (const char* path));
    MOCK_METHOD(const CPS2ROMInfo*, GetROMInfo, ());
};
```

### Test Fixtures

Fixtures provide standardized test data:

1. **Mock ROMs** - `tests/metal/fixtures/test_rom.zip`
2. **Test Frame Data** - Pre-rendered frame data for consistent testing
3. **Checksum Files** - Known good checksums for verification testing

## Continuous Integration

### GitHub Actions

The tests are designed to work with GitHub Actions CI. The workflow configuration is in `.github/workflows/metal_tests.yml`:

```yaml
name: FBNeo Metal Tests

on:
  push:
    branches: [ master, main ]
    paths:
      - 'src/burner/metal/**'
      - 'tests/metal/**'
  pull_request:
    branches: [ master, main ]

jobs:
  build-and-test:
    runs-on: macos-latest
    
    steps:
    - uses: actions/checkout@v3
    
    - name: Install dependencies
      run: |
        brew update
        brew install googletest pkg-config
    
    - name: Run tests
      run: |
        make -f makefile.metal test-all
    
    - name: Archive test results
      uses: actions/upload-artifact@v3
      with:
        name: test-results
        path: tests/metal/reports/*.xml
```

### Test Result Analysis

CI builds generate test reports that can be analyzed to identify trends and regressions:

1. **Pass/Fail Metrics** - Track overall test health
2. **Performance Trends** - Monitor performance changes over time
3. **Coverage Analysis** - Ensure adequate test coverage

## Debugging Failed Tests

When tests fail, follow these steps to debug:

1. **Check the XML report** - Find detailed error information
2. **Run the specific test** - Focus on just the failing test:
   ```bash
   cd tests/metal/build
   ./metal_palette_tests --gtest_filter=PaletteConversionTest.ConvertsPaletteEntry
   ```
3. **Add debug output** - Temporarily add `std::cout` statements to trace execution
4. **Check test fixtures** - Ensure test data is available and correct

## Adding New Tests

To add a new test:

1. **Create a new test file** in the appropriate directory
2. **Include necessary headers**
3. **Define test fixtures** for setup and teardown
4. **Write test cases** using Google Test macros
5. **Update run_tests.sh** if needed (usually automatic)

Example new test file:

```cpp
#include <gtest/gtest.h>
#include "../../../src/burner/metal/metal_cps2_renderer.h"

// Test fixture
class NewFeatureTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code
    }
    
    void TearDown() override {
        // Cleanup code
    }
};

// Test case
TEST_F(NewFeatureTest, ShouldBehaveProperly) {
    // Test code
    EXPECT_TRUE(FeatureWorks());
}

// More test cases...
```

## Testing Best Practices

1. **Test one thing per test** - Keep tests focused and simple
2. **Use descriptive test names** - Names should describe what's being tested
3. **Set up clean state** - Each test should start with a known state
4. **Clean up after tests** - Prevent one test from affecting others
5. **Test edge cases** - Include boundary conditions and error cases
6. **Keep tests fast** - Slow tests discourage frequent testing

## Troubleshooting

### Common Issues

1. **Missing Google Test**
   ```
   Error: Google Test framework not found
   ```
   Solution: Install with `brew install googletest`

2. **Failed to build test**
   ```
   error: unknown type name 'UINT32'
   ```
   Solution: Ensure includes are correct and types are defined

3. **Test runner permission denied**
   ```
   bash: ./run_tests.sh: Permission denied
   ```
   Solution: Make executable with `chmod +x run_tests.sh`

4. **Test crashes**
   ```
   Segmentation fault (core dumped)
   ```
   Solution: Run with debugger: `lldb ./tests/metal/build/test_name`

## References

- [Google Test Documentation](https://google.github.io/googletest/)
- [GitHub Actions Documentation](https://docs.github.com/en/actions)
- [Metal Programming Guide](https://developer.apple.com/documentation/metal) 