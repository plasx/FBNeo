#!/bin/bash
# ============================================================================
# FBNeo Metal Renderer - Build and Run Script
# ============================================================================
# Simple utility to build and run the Metal renderer
# ============================================================================

# Terminal colors for better readability
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Default to enhanced build
BUILD_TYPE="enhanced"
BUILD_CLEAN=0
QUICK_BUILD=0
OPEN_AFTER_BUILD=1
ROM_PATH=""

# Process arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --standalone)
            BUILD_TYPE="standalone"
            shift
            ;;
        --demo)
            BUILD_TYPE="demo"
            shift
            ;;
        --clean)
            BUILD_CLEAN=1
            shift
            ;;
        --quick)
            QUICK_BUILD=1
            shift
            ;;
        --no-run)
            OPEN_AFTER_BUILD=0
            shift
            ;;
        *)
            ROM_PATH="$1"
            shift
            ;;
    esac
done

# Header
echo -e "${BLUE}====================================${NC}"
echo -e "${BLUE}  FBNeo Metal Renderer Build Tool  ${NC}"
echo -e "${BLUE}====================================${NC}"
echo -e "Build type: ${GREEN}$BUILD_TYPE${NC}"

# Clean if requested
if [ $BUILD_CLEAN -eq 1 ]; then
    echo -e "${YELLOW}Cleaning previous build...${NC}"
    make -f makefile.metal clean
fi

# Build
echo -e "${YELLOW}Building FBNeo Metal renderer...${NC}"
if [ $QUICK_BUILD -eq 1 ]; then
    # Quick rebuild without clean
    make -f makefile.metal $BUILD_TYPE
else
    # Standard build
    make -f makefile.metal $BUILD_TYPE
fi

# Check if build was successful
if [ $? -ne 0 ]; then
    echo -e "${RED}Build failed!${NC}"
    exit 1
fi

echo -e "${GREEN}Build successful!${NC}"

# Run if requested
if [ $OPEN_AFTER_BUILD -eq 1 ]; then
    echo -e "${YELLOW}Launching FBNeo...${NC}"
    
    if [ "$BUILD_TYPE" == "enhanced" ]; then
        if [ -n "$ROM_PATH" ]; then
            echo -e "Running with ROM: ${GREEN}$ROM_PATH${NC}"
            open FBNeo.app --args "$ROM_PATH"
        else
            open FBNeo.app
        fi
    elif [ "$BUILD_TYPE" == "standalone" ]; then
        if [ -n "$ROM_PATH" ]; then
            echo -e "Running with ROM: ${GREEN}$ROM_PATH${NC}"
            ./fbneo_metal_standalone "$ROM_PATH"
        else
            ./fbneo_metal_standalone
        fi
    elif [ "$BUILD_TYPE" == "demo" ]; then
        ./fbneo_metal_demo
    fi
fi 