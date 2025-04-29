#!/bin/bash -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

# Build the Flutter package 
cd "$ROOT_DIR/flutter" 

echo "Copying iOS frameworks and Android libraries to React Native project..."

# Copy the iOS framework and project files
cp -R "$ROOT_DIR/ios/cactus.xcframework" ios/ 
cp -R "$ROOT_DIR/ios/CMakelists.txt" ios/ 

# Copy the contents of android/src/main into react/android/src/main
# This copies all files without replacing the directory (there are important unique files)
cp -R "$ROOT_DIR/android/src/main"/* android/src/main/

flutter pub get