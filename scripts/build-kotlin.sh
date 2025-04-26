#!/bin/bash -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

echo "Building Kotlin package..."
cd "$ROOT_DIR/kotlin"

# Create destination directories if they don't exist
mkdir -p android/src/main

# Copy the contents of android/src/main into kotlin/android/src/main
# This copies all files without replacing the directory structure
cp -R "$ROOT_DIR/android/src/main"/* android/src/main/

echo "Kotlin package updated successfully!"