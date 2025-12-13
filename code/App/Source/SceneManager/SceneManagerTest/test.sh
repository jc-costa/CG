#!/bin/bash
# ============================================================================
# SCENE MANAGER TEST - Build and Run Script
# ============================================================================
#
# This script compiles and runs the SceneManager test suite.
#
# Usage:
#   ./test.sh           # Build and run tests
#   ./test.sh clean     # Clean build artifacts
#   ./test.sh build     # Build only (don't run)
#
# Requirements:
#   - CMake 3.14+
#   - C++17 compatible compiler
#   - glm library (fetched via CMake)
#
# ============================================================================

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BUILD_DIR="${SCRIPT_DIR}/build"
TEST_EXECUTABLE="${BUILD_DIR}/scene_manager_test"

# Print colored message
print_msg() {
    echo -e "${CYAN}[SceneManagerTest]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SceneManagerTest]${NC} $1"
}

print_error() {
    echo -e "${RED}[SceneManagerTest]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[SceneManagerTest]${NC} $1"
}

# Clean build artifacts
clean() {
    print_msg "Cleaning build artifacts..."
    rm -rf "${BUILD_DIR}"
    rm -f "${SCRIPT_DIR}/scene_manager_test"
    print_success "Clean complete"
}

# Build the test
build() {
    print_msg "Building SceneManager test suite..."
    
    # Create build directory
    mkdir -p "${BUILD_DIR}"
    cd "${BUILD_DIR}"
    
    # Configure with CMake
    print_msg "Configuring with CMake..."
    cmake .. -DCMAKE_BUILD_TYPE=Debug
    
    # Build
    print_msg "Compiling..."
    cmake --build . --parallel
    
    if [ -f "${TEST_EXECUTABLE}" ]; then
        print_success "Build successful: ${TEST_EXECUTABLE}"
    else
        print_error "Build failed: executable not found"
        exit 1
    fi
    
    cd "${SCRIPT_DIR}"
}

# Run the tests
run() {
    if [ ! -f "${TEST_EXECUTABLE}" ]; then
        print_error "Test executable not found. Building first..."
        build
    fi
    
    print_msg "Running tests..."
    echo ""
    
    cd "${SCRIPT_DIR}"
    "${TEST_EXECUTABLE}"
    
    TEST_RESULT=$?
    
    echo ""
    if [ ${TEST_RESULT} -eq 0 ]; then
        print_success "All tests passed!"
    else
        print_error "Some tests failed!"
    fi
    
    exit ${TEST_RESULT}
}

# Main
case "${1:-run}" in
    clean)
        clean
        ;;
    build)
        build
        ;;
    run|"")
        build
        run
        ;;
    *)
        echo "Usage: $0 [clean|build|run]"
        exit 1
        ;;
esac

