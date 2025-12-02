#!/bin/bash

echo "╔════════════════════════════════════════╗"
echo "║   QUADRIC SURFACES - BUILD & TEST      ║"
echo "╚════════════════════════════════════════╝"
echo ""

cd "$(dirname "$0")"

# Check if glm is available
if [ ! -d "../../../vendor" ]; then
    echo "⚠ Warning: vendor directory not found"
fi

echo "→ Compiling Quadric Test..."
g++ -std=c++17 -Wall \
    -I../../.. \
    QuadricTest.cpp Quadric.cpp \
    -o quadric_test -lm

if [ $? -eq 0 ]; then
    echo "✓ Compilation successful!"
    echo ""
    echo "→ Running tests..."
    echo ""
    ./quadric_test
else
    echo "✗ Compilation failed!"
    exit 1
fi
