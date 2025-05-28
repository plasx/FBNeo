#!/bin/bash
# Build the FBNeo Metal backend and run any specified ROM

# Set working directory to script location
cd "$(dirname "$0")"

# Check if a ROM path was provided
if [ $# -lt 1 ]; then
    echo "Usage: $0 <path_to_rom_file.zip>"
    echo "Example: $0 /path/to/roms/mvsc.zip"
    exit 1
fi

# ROM file path from argument
ROM_PATH="$1"

# Check if ROM exists
if [ ! -f "$ROM_PATH" ]; then
    echo "Error: ROM not found at $ROM_PATH"
    echo "Please provide a valid ROM path."
    exit 1
fi

ROM_FILENAME=$(basename "$ROM_PATH")
ROM_NAME="${ROM_FILENAME%.*}"

echo "======================================"
echo "FBNeo Metal - ROM Runner"
echo "ROM: $ROM_NAME"
echo "======================================"

# Step 1: Clean and build the Metal backend
echo "Building FBNeo Metal backend..."
make -f makefile.metal clean && make -f makefile.metal -j10

# Step 2: Check if build was successful by looking for the binary
if [ ! -f "./bin/metal/fbneo_metal" ]; then
    echo "Error: Build failed, binary not found!"
    exit 1
fi

echo "Build successful!"

# Step 3: Make the binary executable
chmod +x ./bin/metal/fbneo_metal

# Step 4: Create symlink in root directory
echo "Creating symlink to binary..."
rm -f ./fbneo_metal
ln -sf ./bin/metal/fbneo_metal ./fbneo_metal

# Step 5: Check if symlink was created successfully
if [ ! -L "./fbneo_metal" ] || [ ! -x "./fbneo_metal" ]; then
    echo "Warning: Symlink creation failed, running directly from bin/metal/"
    BINARY_PATH="./bin/metal/fbneo_metal"
else
    echo "Symlink created successfully!"
    BINARY_PATH="./fbneo_metal"
fi

# Step 6: Run the game
echo "======================================"
echo "Running $ROM_NAME..."
echo "======================================"
$BINARY_PATH "$ROM_PATH"

# Check if the game ran successfully
if [ $? -ne 0 ]; then
    echo "Error: Game exited with an error!"
    exit 1
fi

echo "Game execution completed successfully!"
exit 0 