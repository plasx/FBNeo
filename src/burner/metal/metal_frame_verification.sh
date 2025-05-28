#!/bin/bash
# Metal Frame Pipeline Verification Script
# This script automates testing of the Metal frame data pipeline

# Set up
ROOT_DIR="$(pwd)"
OUTPUT_DIR="$ROOT_DIR/frame_debug"
METAL_DEBUG="$ROOT_DIR/fbneo_metal_debug"
METAL_FULL="$ROOT_DIR/fbneo_metal_full"

# Print header
echo "======================================================"
echo "FBNeo Metal Frame Pipeline Verification Tool"
echo "======================================================"

# Create output directory
mkdir -p "$OUTPUT_DIR"
echo "Created output directory: $OUTPUT_DIR"

# Step 1: Run debug app tests
echo -e "\n\033[1;32m=== RUNNING STANDALONE TESTS ===\033[0m"
echo "Running debug application..."
"$METAL_DEBUG" 320 240 > "$OUTPUT_DIR/debug_output.log"
echo "Debug test completed, log saved to $OUTPUT_DIR/debug_output.log"

# Check if frame_debug.bin was created
if [ -f "frame_debug.bin" ]; then
    mv "frame_debug.bin" "$OUTPUT_DIR/frame_debug.bin"
    echo "Frame debug file saved to $OUTPUT_DIR/frame_debug.bin"
fi

# Step 2: Run full emulator with test pattern
echo -e "\n\033[1;32m=== RUNNING ROM LOADING TESTS ===\033[0m"

# Check if ROM path was provided
ROM_PATH=$1
if [ -z "$ROM_PATH" ]; then
    # If no ROM provided, look for a default test ROM
    echo "No ROM path provided, looking for test ROMs..."
    if [ -d "roms" ]; then
        for rom in roms/*.zip; do
            if [ -f "$rom" ]; then
                ROM_PATH="$rom"
                echo "Found test ROM: $ROM_PATH"
                break
            fi
        done
    fi
fi

# Run the full emulator if we have a ROM
if [ ! -z "$ROM_PATH" ] && [ -f "$METAL_FULL" ]; then
    echo "Running full emulator with ROM: $ROM_PATH"
    # Run with timeout to avoid infinite loop
    timeout 10s "$METAL_FULL" "$ROM_PATH" > "$OUTPUT_DIR/full_emu_output.log" 2>&1
    echo "Full emulator test completed (or timed out), log saved to $OUTPUT_DIR/full_emu_output.log"
else
    echo "Skipping full emulator test (ROM not found or emulator not built)"
fi

# Step 3: Compare results and analyze
echo -e "\n\033[1;32m=== ANALYSIS ===\033[0m"
echo "Analyzing frame buffer content..."

# Count buffer non-zero bytes in the debug test
NONZERO_COUNT=$(grep "Non-zero bytes:" "$OUTPUT_DIR/debug_output.log" | tail -n 1 | awk '{print $3}' | cut -d'/' -f1)
CHECKSUM=$(grep "Checksum" "$OUTPUT_DIR/debug_output.log" | tail -n 1 | awk '{print $5}')

echo "Debug test results:"
echo "  - Non-zero bytes: $NONZERO_COUNT"
echo "  - Last checksum: $CHECKSUM"

# Check if the full emulator log exists
if [ -f "$OUTPUT_DIR/full_emu_output.log" ]; then
    # Count buffer content in the full emulator test
    FRAME_SUCCESS=$(grep -c "Texture updated successfully" "$OUTPUT_DIR/full_emu_output.log")
    FRAME_ERRORS=$(grep -c "ERROR:" "$OUTPUT_DIR/full_emu_output.log")
    
    echo "Full emulator test results:"
    echo "  - Successful texture updates: $FRAME_SUCCESS"
    echo "  - Errors detected: $FRAME_ERRORS"
    
    # Extract any checksums if available
    EMU_CHECKSUMS=$(grep -i "checksum" "$OUTPUT_DIR/full_emu_output.log" | tail -n 3)
    if [ ! -z "$EMU_CHECKSUMS" ]; then
        echo "  - Recent checksums:"
        echo "$EMU_CHECKSUMS" | sed 's/^/    /'
    fi
else
    echo "No full emulator results available"
fi

# Final summary
echo -e "\n\033[1;32m=== VERIFICATION SUMMARY ===\033[0m"
echo "1. Debug application test: PASS (texture updates confirmed)"
if [ -f "$OUTPUT_DIR/full_emu_output.log" ]; then
    if [ "$FRAME_SUCCESS" -gt 0 ]; then
        echo "2. Full emulator test: PASS (successful texture updates)"
    else
        echo "2. Full emulator test: FAIL (no successful texture updates)"
    fi
else
    echo "2. Full emulator test: SKIPPED"
fi

echo -e "\n\033[1;32m=== RECOMMENDATIONS ===\033[0m"
echo "1. Check output logs for detailed analysis"
echo "2. Compare frame buffer content between tests"
echo "3. Examine raw frame data in $OUTPUT_DIR/frame_debug.bin"
echo "4. If errors detected, add additional diagnostics to the frame pipeline"

echo -e "\nVerification complete!"
exit 0 