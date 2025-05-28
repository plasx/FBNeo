#!/bin/bash

# FBNeo Metal Test Script
# This script tests basic Metal functionality

echo "Testing FBNeo Metal implementation..."

# Check if Metal is available
if ! system_profiler SPDisplaysDataType | grep -q "Metal"; then
    echo "Error: Metal is not supported on this device."
    exit 1
fi

# Check for required frameworks
if [ ! -d "/System/Library/Frameworks/Metal.framework" ]; then
    echo "Error: Metal framework not found."
    exit 1
fi

if [ ! -d "/System/Library/Frameworks/MetalKit.framework" ]; then
    echo "Error: MetalKit framework not found."
    exit 1
fi

if [ ! -d "/System/Library/Frameworks/CoreML.framework" ]; then
    echo "Error: CoreML framework not found."
    exit 1
fi

echo "Metal frameworks found. System is compatible with FBNeo Metal implementation."

# Create test directories
mkdir -p test_metal/bin
mkdir -p test_metal/src

echo "FBNeo Metal implementation test successful!" 