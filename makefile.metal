# makefile.metal
# A dynamic approach, using vpath, for a minimal Metal build of FBNeo on macOS.

TARGET = fbneo_metal

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
DARWIN = 1
endif

# Where source files live
srcdir = src/

# Define directories to search for source files
alldir = burn burn/drv/capcom burn/drv/neo intf/video

# vpath helps make find .cpp and .mm across these directories
vpath %.cpp $(foreach dir,$(alldir),$(srcdir)$(dir))
vpath %.mm  $(foreach dir,$(alldir),$(srcdir)$(dir))
vpath %.h   $(foreach dir,$(alldir),$(srcdir)$(dir))

# Collect all .cpp and .mm in those directories
SOURCES := $(foreach dir,$(alldir),$(wildcard $(srcdir)$(dir)/*.cpp))
SOURCES += $(foreach dir,$(alldir),$(wildcard $(srcdir)$(dir)/*.mm))

# Convert *.cpp/*.mm to *.o
OBJS = $(SOURCES:.cpp=.o)
OBJS := $(OBJS:.mm=.o)

# Compiler and flags
CXX = clang++

# -DMETAL_STANDALONE tells burnint.h to use driverlist_metal.h
CXXFLAGS = -std=c++17 -stdlib=libc++ -Wall -Wextra -Wno-unused-parameter
CXXFLAGS += -ObjC++ -fobjc-arc
CXXFLAGS += -Wno-write-strings
CXXFLAGS += -DMETAL_STANDALONE=1

# Include paths
INCLUDES = \
    -I. \
    -Isrc \
    -Isrc/burn \
    -Isrc/burn/devices \
    -Isrc/intf/video \
    -I/opt/homebrew/include  # For SDL2 or other headers if needed

# Linker flags
# Add more frameworks or libraries if needed, e.g. -framework MetalKit -lSDL2
LDFLAGS = \
    -framework Metal \
    -framework Cocoa \
    -framework QuartzCore

# Default target
.PHONY: all clean
all: $(TARGET)

# Link the final executable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(LDFLAGS)

# Compile rules for .cpp and .mm
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

%.o: %.mm
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)