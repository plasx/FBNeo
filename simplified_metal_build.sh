#!/bin/bash
# Simplified build script for FBNeo Metal frontend

set -e  # Exit on error

# Detect architecture
ARCH=$(uname -m)
if [ "$ARCH" == "arm64" ]; then
    ARCH_FLAG="-arch arm64"
else
    ARCH_FLAG="-arch x86_64"
fi

# Core directories
SRCDIR="src"
BURNDIR="${SRCDIR}/burn"
CPUDIR="${SRCDIR}/cpu"
OBJDIR="obj/metal"
OUTPUT="bin/metal/fbneo_metal"

# Create directories
mkdir -p ${OBJDIR}/{burn,cpu,burner/metal}
mkdir -p bin/metal

# Compiler flags
CXXFLAGS="${ARCH_FLAG} -std=c++17 -O2 -fomit-frame-pointer -Wno-write-strings"
CXXFLAGS+=" -Wall -Wno-long-long -Wno-sign-compare -Wno-uninitialized"
CXXFLAGS+=" -Wno-unused -Wno-conversion -Wno-attributes -Wno-unused-parameter" 
CXXFLAGS+=" -Wno-unused-value -Wno-narrowing -Wno-unused-result"

# Include paths - simplified and organized
INCLUDES="-I${SRCDIR} -I${BURNDIR} -I${BURNDIR}/devices -I${BURNDIR}/sound"
INCLUDES+=" -I${CPUDIR} -I${CPUDIR}/m68k -I${CPUDIR}/z80"
INCLUDES+=" -I${SRCDIR}/intf -I${SRCDIR}/intf/video -I${SRCDIR}/intf/audio -I${SRCDIR}/intf/input"
INCLUDES+=" -I${SRCDIR}/burner -I${SRCDIR}/burner/metal"

# Define Metal-specific flag
CXXFLAGS+=" -DBUILD_METAL"

# Frameworks needed for macOS
FRAMEWORKS="-framework Metal -framework MetalKit -framework Cocoa -framework CoreAudio -framework AudioToolbox"

echo "Building Metal frontend for FBNeo..."

# Compile Metal-specific files
echo "Compiling Metal bridge files..."
clang++ ${CXXFLAGS} ${INCLUDES} -c metal_bridge.cpp -o ${OBJDIR}/metal_bridge.o
clang++ ${CXXFLAGS} ${INCLUDES} -c src/burner/metal/fixes/burn_stubs.cpp -o ${OBJDIR}/burn_stubs.o
clang++ ${CXXFLAGS} ${INCLUDES} -c metal_audio.mm -o ${OBJDIR}/metal_audio.o

# Compile minimal core files needed
echo "Compiling core files..."
clang++ ${CXXFLAGS} ${INCLUDES} -c ${BURNDIR}/burn.cpp -o ${OBJDIR}/burn/burn.o
clang++ ${CXXFLAGS} ${INCLUDES} -c ${BURNDIR}/burn_sound.cpp -o ${OBJDIR}/burn/burn_sound.o
clang++ ${CXXFLAGS} ${INCLUDES} -c ${BURNDIR}/cheat.cpp -o ${OBJDIR}/burn/cheat.o
clang++ ${CXXFLAGS} ${INCLUDES} -c ${BURNDIR}/load.cpp -o ${OBJDIR}/burn/load.o

# Compile CPS2 driver (for Marvel vs Capcom)
echo "Compiling CPS2 driver..."
clang++ ${CXXFLAGS} ${INCLUDES} -c ${BURNDIR}/drv/capcom/cps2_d.cpp -o ${OBJDIR}/burn/drv/capcom/cps2_d.o
clang++ ${CXXFLAGS} ${INCLUDES} -c ${BURNDIR}/drv/capcom/cps_mem.cpp -o ${OBJDIR}/burn/drv/capcom/cps_mem.o
clang++ ${CXXFLAGS} ${INCLUDES} -c ${BURNDIR}/drv/capcom/cps_run.cpp -o ${OBJDIR}/burn/drv/capcom/cps_run.o
clang++ ${CXXFLAGS} ${INCLUDES} -c ${BURNDIR}/drv/capcom/cps_rw.cpp -o ${OBJDIR}/burn/drv/capcom/cps_rw.o
clang++ ${CXXFLAGS} ${INCLUDES} -c ${BURNDIR}/drv/capcom/cps_obj.cpp -o ${OBJDIR}/burn/drv/capcom/cps_obj.o
clang++ ${CXXFLAGS} ${INCLUDES} -c ${BURNDIR}/drv/capcom/cps_pal.cpp -o ${OBJDIR}/burn/drv/capcom/cps_pal.o
clang++ ${CXXFLAGS} ${INCLUDES} -c ${BURNDIR}/drv/capcom/cps_scr.cpp -o ${OBJDIR}/burn/drv/capcom/cps_scr.o

# Link everything
echo "Linking Metal frontend..."
clang++ ${ARCH_FLAG} -o ${OUTPUT} \
    ${OBJDIR}/metal_bridge.o \
    ${OBJDIR}/burn_stubs.o \
    ${OBJDIR}/metal_audio.o \
    ${OBJDIR}/burn/burn.o \
    ${OBJDIR}/burn/burn_sound.o \
    ${OBJDIR}/burn/cheat.o \
    ${OBJDIR}/burn/load.o \
    ${OBJDIR}/burn/drv/capcom/cps2_d.o \
    ${OBJDIR}/burn/drv/capcom/cps_mem.o \
    ${OBJDIR}/burn/drv/capcom/cps_run.o \
    ${OBJDIR}/burn/drv/capcom/cps_rw.o \
    ${OBJDIR}/burn/drv/capcom/cps_obj.o \
    ${OBJDIR}/burn/drv/capcom/cps_pal.o \
    ${OBJDIR}/burn/drv/capcom/cps_scr.o \
    ${FRAMEWORKS}

if [ $? -eq 0 ]; then
    echo "Build successful!"
    echo "Binary location: ${OUTPUT}"
    
    # Create a simple app bundle
    echo "Creating app bundle..."
    APP_BUNDLE="bin/metal/FBNeo.app"
    mkdir -p "${APP_BUNDLE}/Contents/MacOS"
    cp "${OUTPUT}" "${APP_BUNDLE}/Contents/MacOS/FBNeo"
    chmod +x "${APP_BUNDLE}/Contents/MacOS/FBNeo"
    
    echo "App bundle created: ${APP_BUNDLE}"
    echo "To run: open ${APP_BUNDLE}"
else
    echo "Build failed."
    exit 1
fi

echo "Build completed successfully." 