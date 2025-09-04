#!/bin/bash
# Build script for SHA hardware acceleration test

echo "Building SHA1 hardware acceleration test..."

# Compile flags
CXX="${CXX:-g++}"
CXXFLAGS="-std=c++17 -O3 -march=native -msha"
DEFINES="-DDXVK_USE_SHA_HW -DDXVK_ARCH_X86_64"
INCLUDES="-I../include -I../src"

# Source files
SOURCES="test_sha_hw.cpp"
SOURCES="$SOURCES ../src/util/sha1/sha1.c"
SOURCES="$SOURCES ../src/util/sha1/sha1_util.cpp"
SOURCES="$SOURCES ../src/util/sha1/sha1_hw.cpp"

# Output
OUTPUT="test_sha_hw"

# Build command
echo "Compiling with SHA-NI support..."
$CXX $CXXFLAGS $DEFINES $INCLUDES $SOURCES -o $OUTPUT

if [ $? -eq 0 ]; then
    echo "Build successful! Run with: ./test_sha_hw"
    
    # Check if CPU supports SHA extensions
    if grep -q sha /proc/cpuinfo 2>/dev/null; then
        echo "Your CPU supports SHA extensions!"
    else
        echo "Note: Your CPU may not support SHA extensions. The test will use software fallback."
    fi
else
    echo "Build failed!"
    exit 1
fi