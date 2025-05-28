#!/bin/bash
# FBNeo Metal Build Script

echo "Building FBNeo Metal version..."

# Create build directories
mkdir -p build/metal/obj/src/burner/metal/fixes

# Compile C files
echo "Compiling C files..."
clang -std=c11 -O3 -Wall -DHAVE_STDINT_H -DHAVE_STDBOOL_H -DMETAL_IMPLEMENTATION_FILE -DUSE_METAL_RENDERER -arch arm64 -DENHANCED_DEBUG_MODE=1 -Isrc -Isrc/burn -Isrc/burner -Isrc/cpu -Isrc/dep -Isrc/burner/metal -c src/burner/metal/fixes/metal_c_globals.c -o build/metal/obj/metal_c_globals.o
clang -std=c11 -O3 -Wall -DHAVE_STDINT_H -DHAVE_STDBOOL_H -DMETAL_IMPLEMENTATION_FILE -DUSE_METAL_RENDERER -arch arm64 -DENHANCED_DEBUG_MODE=1 -Isrc -Isrc/burn -Isrc/burner -Isrc/cpu -Isrc/dep -Isrc/burner/metal -c src/burner/metal/fixes/metal_c_linkage_functions.c -o build/metal/obj/metal_c_linkage_functions.o
clang -std=c11 -O3 -Wall -DHAVE_STDINT_H -DHAVE_STDBOOL_H -DMETAL_IMPLEMENTATION_FILE -DUSE_METAL_RENDERER -arch arm64 -DENHANCED_DEBUG_MODE=1 -Isrc -Isrc/burn -Isrc/burner -Isrc/cpu -Isrc/dep -Isrc/burner/metal -c src/burner/metal/fixes/metal_clean_stubs.c -o build/metal/obj/metal_clean_stubs.o
clang -std=c11 -O3 -Wall -DHAVE_STDINT_H -DHAVE_STDBOOL_H -DMETAL_IMPLEMENTATION_FILE -DUSE_METAL_RENDERER -arch arm64 -DENHANCED_DEBUG_MODE=1 -Isrc -Isrc/burn -Isrc/burner -Isrc/cpu -Isrc/dep -Isrc/burner/metal -c src/burner/metal/fixes/metal_stubs.c -o build/metal/obj/metal_stubs.o
clang -std=c11 -O3 -Wall -DHAVE_STDINT_H -DHAVE_STDBOOL_H -DMETAL_IMPLEMENTATION_FILE -DUSE_METAL_RENDERER -arch arm64 -DENHANCED_DEBUG_MODE=1 -Isrc -Isrc/burn -Isrc/burner -Isrc/cpu -Isrc/dep -Isrc/burner/metal -c src/burner/metal/fixes/missing_stubs.c -o build/metal/obj/missing_stubs.o
clang -std=c11 -O3 -Wall -DHAVE_STDINT_H -DHAVE_STDBOOL_H -DMETAL_IMPLEMENTATION_FILE -DUSE_METAL_RENDERER -arch arm64 -DENHANCED_DEBUG_MODE=1 -Isrc -Isrc/burn -Isrc/burner -Isrc/cpu -Isrc/dep -Isrc/burner/metal -c src/burner/metal/fixes/ai_stubs.c -o build/metal/obj/ai_stubs.o
clang -std=c11 -O3 -Wall -DHAVE_STDINT_H -DHAVE_STDBOOL_H -DMETAL_IMPLEMENTATION_FILE -DUSE_METAL_RENDERER -arch arm64 -DENHANCED_DEBUG_MODE=1 -Isrc -Isrc/burn -Isrc/burner -Isrc/cpu -Isrc/dep -Isrc/burner/metal -c src/burner/metal/fixes/zlib_stubs.c -o build/metal/obj/zlib_stubs.o
clang -std=c11 -O3 -Wall -DHAVE_STDINT_H -DHAVE_STDBOOL_H -DMETAL_IMPLEMENTATION_FILE -DUSE_METAL_RENDERER -arch arm64 -DENHANCED_DEBUG_MODE=1 -Isrc -Isrc/burn -Isrc/burner -Isrc/cpu -Isrc/dep -Isrc/burner/metal -c src/burner/metal/fixes/rom_path_stubs.c -o build/metal/obj/rom_path_stubs.o
clang -std=c11 -O3 -Wall -DHAVE_STDINT_H -DHAVE_STDBOOL_H -DMETAL_IMPLEMENTATION_FILE -DUSE_METAL_RENDERER -arch arm64 -DENHANCED_DEBUG_MODE=1 -Isrc -Isrc/burn -Isrc/burner -Isrc/cpu -Isrc/dep -Isrc/burner/metal -c src/burner/metal/fixes/metal_zip_extract.c -o build/metal/obj/metal_zip_extract.o
clang -std=c11 -O3 -Wall -DHAVE_STDINT_H -DHAVE_STDBOOL_H -DMETAL_IMPLEMENTATION_FILE -DUSE_METAL_RENDERER -arch arm64 -DENHANCED_DEBUG_MODE=1 -Isrc -Isrc/burn -Isrc/burner -Isrc/cpu -Isrc/dep -Isrc/burner/metal -c src/burner/metal/fixes/metal_rom_loader.c -o build/metal/obj/metal_rom_loader.o
clang -std=c11 -O3 -Wall -DHAVE_STDINT_H -DHAVE_STDBOOL_H -DMETAL_IMPLEMENTATION_FILE -DUSE_METAL_RENDERER -arch arm64 -DENHANCED_DEBUG_MODE=1 -Isrc -Isrc/burn -Isrc/burner -Isrc/cpu -Isrc/dep -Isrc/burner/metal -c src/burner/metal/fixes/memory_tracking.c -o build/metal/obj/memory_tracking.o
clang -std=c11 -O3 -Wall -DHAVE_STDINT_H -DHAVE_STDBOOL_H -DMETAL_IMPLEMENTATION_FILE -DUSE_METAL_RENDERER -arch arm64 -DENHANCED_DEBUG_MODE=1 -Isrc -Isrc/burn -Isrc/burner -Isrc/cpu -Isrc/dep -Isrc/burner/metal -c src/burner/metal/fixes/hardware_tracking.c -o build/metal/obj/hardware_tracking.o
clang -std=c11 -O3 -Wall -DHAVE_STDINT_H -DHAVE_STDBOOL_H -DMETAL_IMPLEMENTATION_FILE -DUSE_METAL_RENDERER -arch arm64 -DENHANCED_DEBUG_MODE=1 -Isrc -Isrc/burn -Isrc/burner -Isrc/cpu -Isrc/dep -Isrc/burner/metal -c src/burner/metal/fixes/graphics_tracking.c -o build/metal/obj/graphics_tracking.o
clang -std=c11 -O3 -Wall -DHAVE_STDINT_H -DHAVE_STDBOOL_H -DMETAL_IMPLEMENTATION_FILE -DUSE_METAL_RENDERER -arch arm64 -DENHANCED_DEBUG_MODE=1 -Isrc -Isrc/burn -Isrc/burner -Isrc/cpu -Isrc/dep -Isrc/burner/metal -c src/burner/metal/fixes/audio_stubs.c -o build/metal/obj/audio_stubs.o
clang -std=c11 -O3 -Wall -DHAVE_STDINT_H -DHAVE_STDBOOL_H -DMETAL_IMPLEMENTATION_FILE -DUSE_METAL_RENDERER -arch arm64 -DENHANCED_DEBUG_MODE=1 -Isrc -Isrc/burn -Isrc/burner -Isrc/cpu -Isrc/dep -Isrc/burner/metal -c src/burner/metal/debug_system.c -o build/metal/obj/debug_system.o
clang -std=c11 -O3 -Wall -DHAVE_STDINT_H -DHAVE_STDBOOL_H -DMETAL_IMPLEMENTATION_FILE -DUSE_METAL_RENDERER -arch arm64 -DENHANCED_DEBUG_MODE=1 -Isrc -Isrc/burn -Isrc/burner -Isrc/cpu -Isrc/dep -Isrc/burner/metal -c src/burner/metal/audio_loop_monitor.c -o build/metal/obj/audio_loop_monitor.o

# Compile C++ files
echo "Compiling C++ files..."
clang++ -std=c++17 -O3 -Wall -DMACOSX -DUSE_METAL_FIXES -DMETAL_IMPLEMENTATION_FILE -DUSE_METAL_RENDERER -DDARWIN -arch arm64 -DENHANCED_DEBUG_MODE=1 -Isrc -Isrc/burn -Isrc/burner -Isrc/cpu -Isrc/dep -Isrc/burner/metal -c src/burner/metal/rom_verifier_stub.cpp -o build/metal/obj/rom_verifier_stub.o
clang++ -std=c++17 -O3 -Wall -DMACOSX -DUSE_METAL_FIXES -DMETAL_IMPLEMENTATION_FILE -DUSE_METAL_RENDERER -DDARWIN -arch arm64 -DENHANCED_DEBUG_MODE=1 -Isrc -Isrc/burn -Isrc/burner -Isrc/cpu -Isrc/dep -Isrc/burner/metal -c src/burner/metal/rom_loading_debug.cpp -o build/metal/obj/rom_loading_debug.o

# Compile Objective-C++ files
echo "Compiling Objective-C++ files..."
clang++ -std=c++17 -O3 -fobjc-arc -DMACOSX -DUSE_METAL_FIXES -DMETAL_IMPLEMENTATION_FILE -DUSE_REAL_METAL_RENDERER -arch arm64 -DENHANCED_DEBUG_MODE=1 -Isrc -Isrc/burn -Isrc/burner -Isrc/cpu -Isrc/dep -Isrc/burner/metal -c src/burner/metal/metal_standalone_main.mm -o build/metal/obj/metal_standalone_main.o
clang++ -std=c++17 -O3 -fobjc-arc -DMACOSX -DUSE_METAL_FIXES -DMETAL_IMPLEMENTATION_FILE -DUSE_REAL_METAL_RENDERER -arch arm64 -DENHANCED_DEBUG_MODE=1 -Isrc -Isrc/burn -Isrc/burner -Isrc/cpu -Isrc/dep -Isrc/burner/metal -c src/burner/metal/metal_bridge.mm -o build/metal/obj/metal_bridge.o
clang++ -std=c++17 -O3 -fobjc-arc -DMACOSX -DUSE_METAL_FIXES -DMETAL_IMPLEMENTATION_FILE -DUSE_REAL_METAL_RENDERER -arch arm64 -DENHANCED_DEBUG_MODE=1 -Isrc -Isrc/burn -Isrc/burner -Isrc/cpu -Isrc/dep -Isrc/burner/metal -c src/burner/metal/metal_standalone_utils.mm -o build/metal/obj/metal_standalone_utils.o
clang++ -std=c++17 -O3 -fobjc-arc -DMACOSX -DUSE_METAL_FIXES -DMETAL_IMPLEMENTATION_FILE -DUSE_REAL_METAL_RENDERER -arch arm64 -DENHANCED_DEBUG_MODE=1 -Isrc -Isrc/burn -Isrc/burner -Isrc/cpu -Isrc/dep -Isrc/burner/metal -c src/burner/metal/metal_main.mm -o build/metal/obj/metal_main.o

# Link the final binary
echo "Linking final binary..."
clang++ -std=c++17 -O3 -fobjc-arc -DMACOSX -DUSE_METAL_FIXES -DMETAL_IMPLEMENTATION_FILE -DUSE_REAL_METAL_RENDERER -arch arm64 -DENHANCED_DEBUG_MODE=1 -o fbneo_metal build/metal/obj/metal_c_globals.o build/metal/obj/metal_c_linkage_functions.o build/metal/obj/metal_clean_stubs.o build/metal/obj/metal_stubs.o build/metal/obj/missing_stubs.o build/metal/obj/ai_stubs.o build/metal/obj/zlib_stubs.o build/metal/obj/rom_path_stubs.o build/metal/obj/metal_zip_extract.o build/metal/obj/metal_rom_loader.o build/metal/obj/memory_tracking.o build/metal/obj/hardware_tracking.o build/metal/obj/graphics_tracking.o build/metal/obj/audio_stubs.o build/metal/obj/debug_system.o build/metal/obj/audio_loop_monitor.o build/metal/obj/rom_verifier_stub.o build/metal/obj/rom_loading_debug.o build/metal/obj/metal_standalone_main.o build/metal/obj/metal_bridge.o build/metal/obj/metal_standalone_utils.o build/metal/obj/metal_main.o -framework Cocoa -framework Metal -framework MetalKit -framework AVFoundation -framework AudioToolbox -framework CoreAudio -framework AudioUnit -framework CoreML -framework Vision -framework CoreGraphics -framework CoreImage -framework GameController -framework QuartzCore -lz

# Make the binary executable
chmod +x fbneo_metal

echo "Build completed. Final binary: fbneo_metal"
echo "To run: ./fbneo_metal [rompath]" 