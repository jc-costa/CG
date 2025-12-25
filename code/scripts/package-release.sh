#!/bin/bash
# Package Release Script for Cinematic Path Tracer
# Usage: ./scripts/package-release.sh [version]

set -e

VERSION=${1:-"1.0.0"}
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$PROJECT_DIR/build"
RELEASE_DIR="$PROJECT_DIR/release"

# Detect platform
case "$(uname -s)" in
    Darwin*)
        PLATFORM="macOS"
        if [[ "$(uname -m)" == "arm64" ]]; then
            PLATFORM="macOS-ARM64"
        fi
        ARCHIVE_EXT="zip"
        ;;
    Linux*)
        PLATFORM="Linux"
        ARCHIVE_EXT="tar.gz"
        ;;
    MINGW*|CYGWIN*|MSYS*)
        PLATFORM="Windows"
        ARCHIVE_EXT="zip"
        ;;
    *)
        echo "Unknown platform"
        exit 1
        ;;
esac

PACKAGE_NAME="CinematicPathTracer-${PLATFORM}"
PACKAGE_DIR="$RELEASE_DIR/$PACKAGE_NAME"

echo "==========================================="
echo "Cinematic Path Tracer Release Packager"
echo "==========================================="
echo "Version:  $VERSION"
echo "Platform: $PLATFORM"
echo "Package:  $PACKAGE_NAME.$ARCHIVE_EXT"
echo "==========================================="
echo ""

# Check if build exists
if [ ! -f "$BUILD_DIR/App/App" ] && [ ! -f "$BUILD_DIR/App/Release/App.exe" ]; then
    echo "Error: Build not found. Please run 'make' first."
    exit 1
fi

# Clean previous release
rm -rf "$PACKAGE_DIR"
mkdir -p "$PACKAGE_DIR"

echo "📦 Packaging release..."

# Copy executable
if [ -f "$BUILD_DIR/App/App" ]; then
    cp "$BUILD_DIR/App/App" "$PACKAGE_DIR/"
    chmod +x "$PACKAGE_DIR/App"
elif [ -f "$BUILD_DIR/App/Release/App.exe" ]; then
    cp "$BUILD_DIR/App/Release/App.exe" "$PACKAGE_DIR/"
fi

# Copy shaders
cp -r "$BUILD_DIR/App/Shaders" "$PACKAGE_DIR/"

# Copy assets
cp -r "$BUILD_DIR/App/assets" "$PACKAGE_DIR/"

# Copy README
cp "$PROJECT_DIR/README.md" "$PACKAGE_DIR/"

# Create archive
cd "$RELEASE_DIR"
if [ "$ARCHIVE_EXT" = "zip" ]; then
    rm -f "$PACKAGE_NAME.zip"
    zip -r "$PACKAGE_NAME.zip" "$PACKAGE_NAME"
    echo ""
    echo "✅ Created: $RELEASE_DIR/$PACKAGE_NAME.zip"
else
    rm -f "$PACKAGE_NAME.tar.gz"
    tar -czvf "$PACKAGE_NAME.tar.gz" "$PACKAGE_NAME"
    echo ""
    echo "✅ Created: $RELEASE_DIR/$PACKAGE_NAME.tar.gz"
fi

# Show package contents
echo ""
echo "📋 Package contents:"
if [ "$ARCHIVE_EXT" = "zip" ]; then
    unzip -l "$PACKAGE_NAME.zip" | head -20
else
    tar -tzvf "$PACKAGE_NAME.tar.gz" | head -20
fi

echo ""
echo "==========================================="
echo "Release package ready for GitHub upload!"
echo "==========================================="

