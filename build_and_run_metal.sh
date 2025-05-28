#!/bin/bash

# FBNeo Metal Build and Run Script
# This script handles building and running the FBNeo Metal backend

# Default settings
CLEAN_BUILD=0
DEBUG_MODE=0
ROM_PATH=""
BUILD_ONLY=0
RUN_ONLY=0
MAKEFILE="makefile.metal.new"
JOBS=4

# Banner
echo "====================================================="
echo "  FBNeo Metal Build and Run Script"
echo "====================================================="

# Parse arguments
while [[ $# -gt 0 ]]; do
  key="$1"
  case $key in
    -c|--clean)
      CLEAN_BUILD=1
      shift
      ;;
    -d|--debug)
      DEBUG_MODE=1
      shift
      ;;
    -r|--rom)
      ROM_PATH="$2"
      shift 2
      ;;
    -b|--build-only)
      BUILD_ONLY=1
      shift
      ;;
    -x|--run-only)
      RUN_ONLY=1
      shift
      ;;
    -j|--jobs)
      JOBS="$2"
      shift 2
      ;;
    -h|--help)
      echo "Usage: $0 [options]"
      echo "Options:"
      echo "  -c, --clean       Perform a clean build"
      echo "  -d, --debug       Enable debug mode"
      echo "  -r, --rom PATH    Specify ROM path to run"
      echo "  -b, --build-only  Build only, don't run"
      echo "  -x, --run-only    Run only, don't build"
      echo "  -j, --jobs N      Use N parallel jobs (default: 4)"
      echo "  -h, --help        Show this help message"
      exit 0
      ;;
    *)
      echo "Unknown option: $1"
      echo "Use -h or --help to see available options"
      exit 1
      ;;
  esac
done

# Ensure the bin/metal directory exists
mkdir -p bin/metal

# Build process
if [[ $RUN_ONLY -eq 0 ]]; then
  echo "[BUILD] Building FBNeo Metal..."
  
  # Clean build if requested
  if [[ $CLEAN_BUILD -eq 1 ]]; then
    echo "[BUILD] Performing clean build..."
    make -f $MAKEFILE clean
  fi
  
  # Set build flags
  BUILD_FLAGS=""
  if [[ $DEBUG_MODE -eq 1 ]]; then
    BUILD_FLAGS="SYMBOL=1"
    echo "[BUILD] Debug mode enabled"
  fi
  
  # Compile
  echo "[BUILD] Compiling with $JOBS parallel jobs..."
  make -f $MAKEFILE -j$JOBS $BUILD_FLAGS
  
  # Check if build was successful
  if [[ $? -ne 0 ]]; then
    echo "[ERROR] Build failed. See above for errors."
    exit 1
  fi
  
  echo "[BUILD] Build completed successfully"
fi

# Run the emulator if not in build-only mode
if [[ $BUILD_ONLY -eq 0 ]]; then
  # Check if executable exists
  if [[ ! -f "bin/metal/fbneo_metal" ]]; then
    if [[ $RUN_ONLY -eq 1 ]]; then
      echo "[ERROR] Executable not found. Build first with -b option."
      exit 1
    else
      echo "[ERROR] Executable not found after build. Check above for errors."
      exit 1
    fi
  fi
  
  echo "[RUN] Running FBNeo Metal..."
  
  # Prepare command
  RUN_CMD="./bin/metal/fbneo_metal"
  
  # Add ROM path if specified
  if [[ -n "$ROM_PATH" ]]; then
    echo "[RUN] Using ROM: $ROM_PATH"
    RUN_CMD="$RUN_CMD -rom \"$ROM_PATH\""
  fi
  
  # Add debug flags if in debug mode
  if [[ $DEBUG_MODE -eq 1 ]]; then
    echo "[RUN] Debug mode enabled"
    RUN_CMD="$RUN_CMD -debug"
  fi
  
  # Run the command
  echo "[RUN] Executing: $RUN_CMD"
  eval $RUN_CMD
fi

echo "====================================================="
echo "  FBNeo Metal session completed"
echo "====================================================="

exit 0 