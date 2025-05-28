#!/bin/bash
echo "Compiling Metal shaders..."
mkdir -p obj/metal/burner/metal
xcrun -sdk macosx metal -c src/burner/metal/Shaders.metal -o obj/metal/burner/metal/Shaders.air
xcrun -sdk macosx metallib obj/metal/burner/metal/Shaders.air -o obj/metal/burner/metal/Shaders.metallib
mkdir -p bin/metal
cp obj/metal/burner/metal/Shaders.metallib bin/metal/
echo "Metal shaders compiled and copied."
