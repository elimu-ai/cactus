#!/bin/bash -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

# Build the React Native package 
cd "$ROOT_DIR/react" 

# Remove node_modules if they still exist
[ -d node_modules ] && rm -rf node_modules
[ -d lib ] && rm -rf lib 

echo "Copying iOS frameworks and Android libraries to React Native project..."

# Copy the iOS framework and project files
cp -R "$ROOT_DIR/ios"/* ios/

# Copy only the jniLibs directory from android/src/main into react/android/src/main
cp -R "$ROOT_DIR/android/src/main/jniLibs" "android/src/main/"

echo "Building React Native package..." 
yarn 
yarn build 

echo "React Native package built successfully!" 