#!/bin/bash

# Exit immediately if a command exits with a non-zero status.
set -e

# Get the directory where the script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# Navigate to the project root based on the script location
PROJECT_ROOT="$SCRIPT_DIR/.."
ANDROID_DIR="$PROJECT_ROOT/android"

echo "Navigating to $ANDROID_DIR"
cd "$ANDROID_DIR"

echo "Building Android library (Release)..."
# Clean previous build and assemble the release AAR
./gradlew clean 
./gradlew build

echo "Android library build completed successfully."

# Optionally, navigate back to the original directory
# cd - > /dev/null

exit 0
