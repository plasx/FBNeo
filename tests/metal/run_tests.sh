#!/bin/bash
# FBNeo Metal Tests Runner Script
# This script compiles and runs all FBNeo Metal tests

set -e

# Script location
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"

# Test directories
UNIT_TEST_DIR="$SCRIPT_DIR/unit"
INTEGRATION_TEST_DIR="$SCRIPT_DIR/integration"
PERFORMANCE_TEST_DIR="$SCRIPT_DIR/performance"

# Output directories
BUILD_DIR="$SCRIPT_DIR/build"
REPORTS_DIR="$SCRIPT_DIR/reports"

# Colors for prettier output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Create directories if they don't exist
mkdir -p "$BUILD_DIR"
mkdir -p "$REPORTS_DIR"

# Print section header
print_header() {
    echo -e "\n${BLUE}==== $1 ====${NC}"
}

# Compile a test file
compile_test() {
    local test_file="$1"
    local test_name=$(basename "$test_file" .cpp)
    local output_file="$BUILD_DIR/$test_name"
    
    echo -e "${YELLOW}Compiling${NC} $test_name..."
    
    # Compile with appropriate flags
    clang++ -std=c++17 -ObjC++ \
        -I"$ROOT_DIR/src" \
        -I"$ROOT_DIR/src/burner" \
        -I"$ROOT_DIR/src/burn" \
        -framework Cocoa -framework Metal -framework MetalKit \
        -lgtest -lgtest_main -lpthread \
        "$test_file" -o "$output_file"
        
    echo -e "${GREEN}Compiled${NC} $test_name"
    
    # Return the output file path
    echo "$output_file"
}

# Run a compiled test
run_test() {
    local test_file="$1"
    local test_name=$(basename "$test_file")
    local xml_output="$REPORTS_DIR/${test_name}_report.xml"
    
    echo -e "${YELLOW}Running${NC} $test_name..."
    
    # Run the test with XML output for CI integration
    "$test_file" --gtest_output="xml:$xml_output"
    
    # Check result
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}PASSED${NC} $test_name"
        return 0
    else
        echo -e "${RED}FAILED${NC} $test_name"
        return 1
    fi
}

# Run all tests in a directory
run_tests_in_dir() {
    local dir="$1"
    local name="$2"
    local failed=0
    
    print_header "$name Tests"
    
    # Find all test files
    local test_files=$(find "$dir" -name "*.cpp")
    
    # Check if there are any tests
    if [ -z "$test_files" ]; then
        echo "No tests found in $dir"
        return 0
    fi
    
    # Compile and run each test
    for test_file in $test_files; do
        local compiled_test=$(compile_test "$test_file")
        
        # Run the compiled test
        run_test "$compiled_test"
        
        # Track failures
        if [ $? -ne 0 ]; then
            failed=$((failed + 1))
        fi
    done
    
    # Return number of failed tests
    return $failed
}

# Check for Google Test framework
if ! pkg-config --exists gtest; then
    echo -e "${RED}Error: Google Test framework not found${NC}"
    echo "Please install Google Test:"
    echo "  brew install googletest"
    exit 1
fi

# Print test information
print_header "FBNeo Metal Tests"
echo "Running tests from: $SCRIPT_DIR"
echo "Build directory: $BUILD_DIR"
echo "Reports directory: $REPORTS_DIR"

# Clean previous builds if requested
if [ "$1" == "--clean" ]; then
    echo -e "${YELLOW}Cleaning previous builds...${NC}"
    rm -rf "$BUILD_DIR"/*
    rm -rf "$REPORTS_DIR"/*
    mkdir -p "$BUILD_DIR"
    mkdir -p "$REPORTS_DIR"
fi

# Track total failures
TOTAL_FAILURES=0

# Run unit tests
run_tests_in_dir "$UNIT_TEST_DIR" "Unit"
TOTAL_FAILURES=$((TOTAL_FAILURES + $?))

# Run integration tests
run_tests_in_dir "$INTEGRATION_TEST_DIR" "Integration"
TOTAL_FAILURES=$((TOTAL_FAILURES + $?))

# Run performance tests (optional)
if [ "$1" == "--with-perf" ] || [ "$2" == "--with-perf" ]; then
    run_tests_in_dir "$PERFORMANCE_TEST_DIR" "Performance"
    # Don't count performance test failures in total (they're often threshold-dependent)
fi

# Print summary
print_header "Test Summary"
if [ $TOTAL_FAILURES -eq 0 ]; then
    echo -e "${GREEN}All tests PASSED!${NC}"
    exit 0
else
    echo -e "${RED}$TOTAL_FAILURES test(s) FAILED${NC}"
    exit 1
fi 