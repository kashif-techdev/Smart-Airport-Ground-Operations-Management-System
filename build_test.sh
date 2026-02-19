#!/bin/bash

# Build test script for Airport Simulator
# This script checks if the project compiles successfully

echo "========================================="
echo "Airport Simulator Build Test"
echo "========================================="
echo ""

# Check for required dependencies
echo "Checking dependencies..."
if ! command -v g++ &> /dev/null; then
    echo "ERROR: g++ compiler not found!"
    exit 1
fi

if ! pkg-config --exists ncurses; then
    echo "WARNING: ncurses library not found. Install with: sudo apt-get install libncurses5-dev"
    echo "Continuing anyway..."
fi

echo "✓ Dependencies check complete"
echo ""

# Clean previous build
echo "Cleaning previous build..."
make clean
echo ""

# Build project
echo "Building project..."
if make; then
    echo ""
    echo "========================================="
    echo "✓ Build successful!"
    echo "========================================="
    echo ""
    echo "To run the simulator:"
    echo "  ./airport_simulator"
    echo ""
    exit 0
else
    echo ""
    echo "========================================="
    echo "✗ Build failed!"
    echo "========================================="
    echo ""
    exit 1
fi

