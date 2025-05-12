#!/bin/bash -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

export LEFTHOOK=0 

# Remove node_modules if they still exist
[ -d node_modules ] && rm -rf node_modules
[ -d lib ] && rm -rf lib 

# # Build the Android library
# "$SCRIPT_DIR/build-react-android.sh"

# Build the React Native package 
cd "$ROOT_DIR/react" 

echo "Copying iOS frameworks to React Native project..."

# Copy the iOS framework and project files
rm -rf ios/cactus.xcframework
cp -R "$ROOT_DIR/ios"/cactus.xcframework ios/
cp -R "$ROOT_DIR/ios"/CMakeLists.txt ios/

echo "Building React Native package..." 
yarn 
yarn build 

echo "React Native package built successfully!" 