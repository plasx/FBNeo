# makefile.metal
# Standalone build for FBNeo on macOS with Metal + SDL2
# - Does NOT require auto-generated driverlist.h
# - Uses driverlist_metal.h (static driver references)
# - Depends on new tchar_dummy.h to avoid Windows <tchar.h> errors
# - Should not affect other builds

TARGET = fbneo_metal

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
DARWIN = 1
endif

# ------------------------------------------------------------------------
# SOURCE FILES (minimal set)
# ------------------------------------------------------------------------
SOURCES = \
    src/intf/video/sdl/sdl2_video_metal.mm \
    src/intf/video/metal_renderer.mm \
    src/intf/video/vid_interface_metal.cpp \
    \
    src/burn/burn.cpp \
    src/burn/burnint.cpp \
    src/burn/cpuexec.cpp \
    src/burn/state.cpp \
    \
    src/burn/drv/capcom/cps1.cpp \
    src/burn/drv/capcom/cps2.cpp \
    src/burn/drv/neo/neo_geo.cpp \
    \
    # Add more as needed, but be sure to declare them in driverlist_metal.h

OBJS = $(SOURCES:.cpp=.o)
OBJS := $(OBJS:.mm=.o)

# ------------------------------------------------------------------------
# COMPILER & FLAGS
# ------------------------------------------------------------------------
CXX = clang++

# -DMETAL_STANDALONE tells burnint.h to use driverlist_metal.h
CXXFLAGS = -std=c++17 -stdlib=libc++ -Wall -Wextra -Wno-unused-parameter
CXXFLAGS += -ObjC++ -fobjc-arc
CXXFLAGS += -Wno-write-strings
CXXFLAGS += -DMETAL_STANDALONE=1

# ------------------------------------------------------------------------
# INCLUDE PATHS
# ------------------------------------------------------------------------
INCLUDES = \
    -I. \
    -Isrc \
    -Isrc/burn \
    -Isrc/intf/video \
    -I/opt/homebrew/include  # SDL2 installed by brew, for example

# ------------------------------------------------------------------------
# LINKER FLAGS
# ------------------------------------------------------------------------
LDFLAGS = \
    -framework Metal \
    -framework MetalKit \
    -framework Cocoa \
    -framework QuartzCore \
    -framework IOKit \
    -framework CoreVideo \
    -L/opt/homebrew/lib \
    -lSDL2

# ------------------------------------------------------------------------
# BUILD TARGET
# ------------------------------------------------------------------------
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(LDFLAGS)

# Rules to build .cpp and .mm
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

%.o: %.mm
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean