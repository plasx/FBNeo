#!/bin/bash
# FBNeo Metal Port Build Script
# This script builds the Metal port of FBNeo with optimized settings

# Set terminal colors
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Print header
echo -e "${BLUE}============================================${NC}"
echo -e "${BLUE}  FBNeo Metal Port - Build Script${NC}"
echo -e "${BLUE}  For macOS 14+ with Apple Silicon/Intel${NC}"
echo -e "${BLUE}============================================${NC}"

# Check for macOS
if [[ $(uname) != "Darwin" ]]; then
  echo -e "${RED}ERROR: This script is only for macOS.${NC}"
  exit 1
fi

# Check for required tools
if ! command -v clang++ &> /dev/null; then
  echo -e "${RED}ERROR: clang++ not found. Please install Xcode and Command Line Tools.${NC}"
  exit 1
fi

if ! command -v xcrun &> /dev/null; then
  echo -e "${RED}ERROR: xcrun not found. Please install Xcode and Command Line Tools.${NC}"
  exit 1
fi

# Make sure shaders exist
if [[ ! -f ./fbneo_shaders.metallib || ! -f ./default.metallib ]]; then
  echo -e "${YELLOW}Creating placeholder Metal shader files...${NC}"
  touch ./fbneo_shaders.metallib
  touch ./default.metallib
fi

# Determine CPU count for parallel build
CPU_COUNT=$(sysctl -n hw.ncpu)
echo -e "${BLUE}Detected ${CPU_COUNT} CPU cores for parallel build.${NC}"

# Clean build option
if [[ "$1" == "clean" ]]; then
  echo -e "${YELLOW}Cleaning build artifacts...${NC}"
  make -f makefile.metal clean
  echo -e "${GREEN}Clean completed.${NC}"
  exit 0
fi

# Full rebuild option
if [[ "$1" == "rebuild" ]]; then
  echo -e "${YELLOW}Performing clean rebuild...${NC}"
  make -f makefile.metal clean
  echo -e "${GREEN}Clean completed.${NC}"
else
  echo -e "${YELLOW}Performing incremental build...${NC}"
  echo -e "${YELLOW}(Use './build_metal.sh rebuild' for clean build)${NC}"
fi

# Start build
echo -e "${YELLOW}Building FBNeo Metal port...${NC}"
make -f makefile.metal -j${CPU_COUNT}

# Check if build succeeded
if [ $? -eq 0 ]; then
  echo -e "${GREEN}Build successful!${NC}"
  
  # Create a link to the executable in the current directory
  if [ -f "build/metal/fbneo_metal" ]; then
    cp build/metal/fbneo_metal ./fbneo_metal
    chmod +x ./fbneo_metal
    echo -e "${GREEN}Copied executable to ./fbneo_metal${NC}"
  fi
  
  echo -e "${BLUE}============================================${NC}"
  echo -e "${GREEN}FBNeo Metal port built successfully.${NC}"
  echo -e "${YELLOW}Run it with: ./fbneo_metal /path/to/rom.zip${NC}"
  echo -e "${BLUE}============================================${NC}"
else
  echo -e "${RED}Build failed.${NC}"
  exit 1
fi 