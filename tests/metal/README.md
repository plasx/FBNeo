# FBNeo Metal Test Suite

This directory contains automated tests for the Metal-specific functionalities within the FBNeo emulator. These tests are designed to ensure the reliability and performance of the Metal integration, with a focus on CPS2 rendering and ROM handling.

## Test Categories

The test suite is organized into three main categories:

1. **Unit Tests** - Test individual components in isolation
   - Metal palette conversion
   - Frame buffer handling
   - CPS2 ROM loader functionality

2. **Integration Tests** - Test how components interact with each other
   - Metal-FBNeo core integration
   - ROM path detection
   - Driver selection and hardware detection

3. **Performance Tests** - Benchmark critical paths
   - Frame rendering performance
   - Palette conversion speed
   - Resolution scaling
   - Overall frame rate

## Requirements

- macOS 10.15 or later
- Xcode Command Line Tools
- Google Test framework (`brew install googletest`)
- pkg-config (`brew install pkg-config`)

## Running the Tests

You can run the tests using the included `run_tests.sh` script or via the makefile targets.

### Using the script directly

```bash
# Run all unit tests
cd tests/metal
./run_tests.sh

# Clean previous builds and run all tests
./run_tests.sh --clean

# Include performance tests
./run_tests.sh --with-perf

# Clean and run all tests including performance tests
./run_tests.sh --clean --with-perf
```

### Using makefile targets

```bash
# Run unit tests only
make -f makefile.metal test

# Run integration tests
make -f makefile.metal test-integration

# Run performance tests
make -f makefile.metal test-performance

# Run all tests
make -f makefile.metal test-all
```

## Test Results

Test results are saved as XML files in the `tests/metal/reports` directory. These files are compatible with CI systems like GitHub Actions and can be visualized using tools that support the JUnit XML format.

## Adding New Tests

To add a new test:

1. Create a new `.cpp` file in the appropriate test directory:
   - `tests/metal/unit/` for unit tests
   - `tests/metal/integration/` for integration tests
   - `tests/metal/performance/` for performance tests

2. Include the Google Test framework and relevant headers:
   ```cpp
   #include <gtest/gtest.h>
   #include "../../../src/burner/metal/relevant_header.h"
   ```

3. Add test fixtures and test cases following the Google Test format.

4. Run the tests to verify your new test works as expected.

## CI Pipeline Integration

The test suite is integrated with GitHub Actions for continuous integration. See `.github/workflows/metal_tests.yml` for the CI configuration.

The CI pipeline:
- Runs on every push and pull request that affects Metal-specific code
- Builds the FBNeo Metal project
- Runs unit and integration tests
- Archives and publishes test results
- Runs performance tests (as informational, not causing build failures)

## Testing Strategy

The FBNeo Metal tests focus on specific areas that are critical for proper operation:

1. **Correctness**
   - Ensuring Metal implementations correctly handle input data
   - Verifying palette conversion accuracy
   - Validating ROM loading and verification

2. **Stability**
   - Testing error handling and boundary conditions
   - Ensuring proper handling of null pointers and invalid inputs
   - Checking resource cleanup

3. **Performance**
   - Measuring frame rendering times
   - Benchmarking palette conversion
   - Testing different resolutions
   - Ensuring 60 FPS operation is achievable

## Mocks and Test Fixtures

The tests use mocks and fixtures to isolate components:

- Mock frame buffers for testing rendering
- Mock palette data for testing color conversion
- Mock ROM data for testing loading and verification

This approach allows testing components without requiring actual ROM files or hardware access.

## Troubleshooting

If tests are failing, check the following:

1. Ensure all dependencies are installed
2. Verify that the Metal framework is available on your system
3. Check that the test fixture directories exist and are writable
4. Examine the XML reports for specific test failures

For performance test failures, remember that these are often environment-dependent and may pass on some systems while failing on others with different hardware.

## License

These tests are covered by the same license as the main FBNeo project. 