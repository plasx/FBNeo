#!/bin/bash
# Marvel vs Capcom - FBNeo Metal Renderer Launcher
# Enhanced with debugging and diagnostics

# Set up environment variables
export METAL_DEVICE_WRAPPER_TYPE=1
export METAL_DEBUG_OPTIONS="verbose,validation,shader-validation,global-capture"

# Enable debug logging for Metal
export METAL_DEBUG=1
export FB_DEBUG=1

# Prepare the output directory
mkdir -p debug_output

# Create log file
LOG_FILE="debug_output/mvsc_metal_debug.log"
echo "Starting Marvel vs Capcom with Metal renderer at $(date)" > $LOG_FILE

# Possible ROM paths
ROM_PATHS=(
  "./roms/mvsc"
  "./roms/mvsc.zip"
  "../roms/mvsc"
  "../roms/mvsc.zip"
  "/Users/plasx/ROMs/mvsc.zip"
  "/Users/plasx/ROMs/MAME/mvsc.zip"
  "/Users/plasx/dev/ROMs/mvsc.zip"
)

# Look for the ROM in all possible paths
ROM_PATH=""
for path in "${ROM_PATHS[@]}"; do
  echo "Checking for ROM at: $path" | tee -a $LOG_FILE
  if [ -f "$path" ] || [ -d "$path" ]; then
    ROM_PATH="$path"
    echo "Found ROM at: $ROM_PATH" | tee -a $LOG_FILE
    break
  fi
done

if [ -z "$ROM_PATH" ]; then
  echo "ERROR: ROM not found in any of the search paths." | tee -a $LOG_FILE
  echo "Please place mvsc.zip in ./roms/ directory or specify its location." | tee -a $LOG_FILE
  exit 1
fi

# Show diagnostic info
echo "System information:" | tee -a $LOG_FILE
uname -a | tee -a $LOG_FILE
echo "Running from directory: $(pwd)" | tee -a $LOG_FILE

# Run the game with Metal renderer
echo "Starting FBNeo with Metal renderer..." | tee -a $LOG_FILE
echo "Command: ./fbneo_metal \"$ROM_PATH\" -verbose -debug" | tee -a $LOG_FILE

# Run with verbose output
./fbneo_metal "$ROM_PATH" -verbose -debug 2>&1 | tee -a $LOG_FILE

# Check exit status
EXIT_CODE=$?
echo "FBNeo exited with code: $EXIT_CODE" | tee -a $LOG_FILE

echo "Debug log saved to: $LOG_FILE" 