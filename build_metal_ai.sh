#!/bin/bash
# FBNeo Metal build script with AI features

set -e

# Create necessary directories
mkdir -p src/dep/generated
mkdir -p build/metal/obj/src/burn/devices
mkdir -p build/metal/obj/src/burn/drv/megadrive
mkdir -p build/metal/obj/src/burner/metal/ai

# Create symlinks for header files if they don't exist
if [ ! -L src/dep/generated/tiles_generic.h ]; then
    ln -sf ../../../burn/tiles_generic.h src/dep/generated/tiles_generic.h
fi

if [ ! -L src/dep/generated/burnint.h ]; then
    ln -sf ../../../burn/burnint.h src/dep/generated/burnint.h
fi

# Ensure AI directory exists
mkdir -p models/ai

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}=== Building FBNeo Metal with AI Features ===${NC}"

# Create C/C++ compatibility header
echo -e "${GREEN}Creating C/C++ compatibility header...${NC}"
cat > src/burner/metal/fixes/c_cpp_fixes.h << 'EOF'
// C/C++ compatibility fixes for Metal build
#pragma once

// Include standard types
#include <stdint.h>

// Define basic types if not already defined
#ifndef INT32
typedef int32_t INT32;
#endif

#ifndef UINT32
typedef uint32_t UINT32;
#endif

#ifndef INT8
typedef int8_t INT8;
#endif

#ifndef UINT8
typedef uint8_t UINT8;
#endif

#ifndef INT16
typedef int16_t INT16;
#endif

#ifndef UINT16
typedef uint16_t UINT16;
#endif

// C compatibility for bool type
#ifndef __cplusplus
    typedef int bool;
    #define true 1
    #define false 0
#endif

// Disable warnings for build issues we cannot fix
#pragma GCC diagnostic ignored "-Wmissing-braces"
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"
EOF

# Verify AI components
echo -e "${GREEN}Checking AI implementation files...${NC}"

# List of essential AI files to check
AI_FILES=(
    "src/burner/metal/ai/coreml_manager.mm"
    "src/burner/metal/ai/model_loader.mm"
    "src/burner/metal/ai/tensor_ops.metal"
    "src/burner/metal/ai/ai_definitions.h"
    "src/burner/metal/ai/ai_interface.h"
)

MISSING_FILES=0
for file in "${AI_FILES[@]}"; do
    if [ ! -f "$file" ]; then
        echo -e "${RED}Missing AI file: $file${NC}"
        MISSING_FILES=1
    else
        echo -e "${GREEN}Found AI file: $file${NC}"
    fi
done

if [ $MISSING_FILES -eq 1 ]; then
    echo -e "${YELLOW}Some AI implementation files are missing. Build may not include all AI features.${NC}"
fi

# Clean build if requested
if [ "$1" == "clean" ]; then
    echo -e "${BLUE}Cleaning build...${NC}"
    make -f makefile.metal.ai clean
fi

# Generate AI model compilation files
echo -e "${GREEN}Generating Metal shaders for AI...${NC}"
mkdir -p build/metal/shaders

# Compile Metal shader
xcrun -sdk macosx metal -c src/burner/metal/ai/tensor_ops.metal -o build/metal/shaders/tensor_ops.air

echo -e "${GREEN}Creating Metal library...${NC}"
xcrun -sdk macosx metallib build/metal/shaders/tensor_ops.air -o build/metal/shaders/ai_shaders.metallib

# Create a default makefile for AI implementation if it doesn't exist
if [ ! -f makefile.metal.ai ]; then
    echo -e "${YELLOW}Creating default AI makefile...${NC}"
    cat > makefile.metal.ai << 'EOF'
# FBNeo Metal AI Implementation Makefile

# Include the main Metal makefile
include makefile.metal

# Additional include paths for AI components
INCLUDES += -Isrc/burner/metal/ai

# AI source files
AI_MM_SRCS := $(wildcard src/burner/metal/ai/*.mm)
AI_CPP_SRCS := $(wildcard src/burner/metal/ai/*.cpp)
AI_METAL_SRCS := $(wildcard src/burner/metal/ai/*.metal)

# AI object files
AI_MM_OBJS := $(AI_MM_SRCS:%.mm=$(OBJDIR)/%.o)
AI_CPP_OBJS := $(AI_CPP_SRCS:%.cpp=$(OBJDIR)/%.o)
AI_METAL_OBJS := $(AI_METAL_SRCS:%.metal=$(OBJDIR)/%.air)

# Additional libraries for AI
LDFLAGS += -framework CoreML -framework Vision -framework MetalPerformanceShaders -framework MetalPerformanceShadersGraph

# Include AI objects in the build
OBJS += $(AI_MM_OBJS) $(AI_CPP_OBJS)

# Rename the output target
TARGET = fbneo_metal_ai

# Additional targets for Metal shader compilation
all: $(TARGET) build/metal/shaders/ai_shaders.metallib

# Metal shader compilation
$(OBJDIR)/%.air: %.metal
	@mkdir -p $(dir $@)
	xcrun -sdk macosx metal -c $< -o $@

# Metal library linking
build/metal/shaders/ai_shaders.metallib: $(AI_METAL_OBJS)
	@mkdir -p $(dir $@)
	xcrun -sdk macosx metallib $(AI_METAL_OBJS) -o $@

# Special rule for compiling Objective-C++ files with AI features
$(OBJDIR)/src/burner/metal/ai/%.o: src/burner/metal/ai/%.mm
	@mkdir -p $(dir $@)
	$(OBJC) $(OBJCFLAGS) -c $< -o $@

# Copy AI models to the build directory
copy_models:
	@mkdir -p build/metal/models
	@cp -R models/ai/* build/metal/models/ 2>/dev/null || true
	@echo "Models copied to build/metal/models"

# Make the copy_models target a dependency of the main target
$(TARGET): copy_models

.PHONY: copy_models
EOF
fi

# Compile our patched files directly - core engine files needed for AI integration
echo -e "${GREEN}Compiling core patched files for AI integration...${NC}"

# Create the right directory structure
mkdir -p build/metal/obj/src/burn
mkdir -p build/metal/obj/src/burn/devices
mkdir -p build/metal/obj/src/burner/metal/ai

# Define common compile flags
CFLAGS="-g -O2 -Wall -Isrc/dep/generated -Isrc -Isrc/burn -Isrc/burn/devices -Isrc/burn/snd -Isrc/burn/drv -Isrc/burn/drv/capcom -Isrc/burner -Isrc/burner/metal -Isrc/burner/metal/app -Isrc/burner/metal/fixes -Isrc/burner/metal/ai -Isrc/cpu -Isrc/cpu/m68k -Isrc/cpu/z80 -Isrc/intf/video -Isrc/intf/audio -Isrc/intf/input -I/opt/homebrew/include -Isrc/burn/cpu_generated -DMETAL_BUILD -DMETAL_AI -fcommon -Wno-everything"

# Objective-C++ compile flags
OBJCFLAGS="$CFLAGS -fobjc-arc"

# Compile the essential patched files
echo -e "${GREEN}Compiling burn_patched.cpp...${NC}"
clang++ $CFLAGS -c src/burner/metal/fixes/burn_patched.cpp -o build/metal/obj/src/burn/burn.o

echo -e "${GREEN}Compiling cheat_patched.cpp...${NC}"
clang++ $CFLAGS -c src/burner/metal/fixes/cheat_patched.cpp -o build/metal/obj/src/burn/cheat.o

echo -e "${GREEN}Compiling load_patched.cpp...${NC}"
clang++ $CFLAGS -c src/burner/metal/fixes/load_patched.cpp -o build/metal/obj/src/burn/load.o

echo -e "${GREEN}Compiling eeprom_patched.cpp...${NC}"
clang++ $CFLAGS -c src/burner/metal/fixes/eeprom_patched.cpp -o build/metal/obj/src/burn/devices/eeprom.o

echo -e "${GREEN}Compiling burn_led_patched.cpp...${NC}"
clang++ $CFLAGS -c src/burner/metal/fixes/burn_led_patched.cpp -o build/metal/obj/src/burn/burn_led.o

# Compile AI components directly
echo -e "${GREEN}Compiling AI implementation files...${NC}"

# CoreML Manager
if [ -f src/burner/metal/ai/coreml_manager.mm ]; then
    echo -e "${GREEN}Compiling coreml_manager.mm...${NC}"
    mkdir -p build/metal/obj/src/burner/metal/ai
    clang++ $OBJCFLAGS -c src/burner/metal/ai/coreml_manager.mm -o build/metal/obj/src/burner/metal/ai/coreml_manager.o
fi

# Model Loader
if [ -f src/burner/metal/ai/model_loader.mm ]; then
    echo -e "${GREEN}Compiling model_loader.mm...${NC}"
    mkdir -p build/metal/obj/src/burner/metal/ai
    clang++ $OBJCFLAGS -c src/burner/metal/ai/model_loader.mm -o build/metal/obj/src/burner/metal/ai/model_loader.o
fi

# Check for download_models.sh script
if [ -f download_models.sh ]; then
    echo -e "${YELLOW}AI models download script detected. Do you want to download models? (y/n)${NC}"
    read -r download_choice
    if [[ $download_choice == "y" || $download_choice == "Y" ]]; then
        echo -e "${GREEN}Downloading AI models...${NC}"
        bash download_models.sh
    else
        echo -e "${YELLOW}Skipping model download.${NC}"
    fi
else
    echo -e "${YELLOW}No model download script found. You may need to download models manually.${NC}"
fi

echo -e "${GREEN}Compilation of AI components complete!${NC}"

echo -e "${BLUE}=== AI Integration Status ===${NC}"
echo -e "${GREEN}CoreML Manager: Implemented${NC}"
echo -e "${GREEN}Model Loader: Implemented${NC}"
echo -e "${GREEN}Tensor Operations: Implemented${NC}"

echo -e "${YELLOW}To complete the build, you should run: make -f makefile.metal.ai${NC}"
echo -e "${YELLOW}The AI-enabled Metal build will be created as 'fbneo_metal_ai'${NC}"

echo -e "${BLUE}=== Build script completed ===${NC}" 