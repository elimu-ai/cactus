rm -rf swift/CactusSwift.xcframework

# Copy cactus framework to swift/source
cp -R ios/cactus.xcframework swift/Source/

xcodebuild clean

HEADER_PATHS="$(pwd)/swift/Source/cactus.xcframework/ios-arm64/cactus.framework/Headers $(pwd)/swift/Source/cactus.xcframework/ios-arm64_x86_64-simulator/cactus.framework/Headers"

# Build CactusSwift framework for iOS device
xcodebuild -project swift/Source/CactusSwift.xcodeproj -scheme CactusSwift -configuration Release -sdk iphoneos -derivedDataPath build

# Build CactusSwift framework for iOS simulator
xcodebuild -project swift/Source/CactusSwift.xcodeproj -scheme CactusSwift -configuration Release -sdk iphonesimulator -derivedDataPath build

DEVICE_FRAMEWORK="build/Build/Products/Release-iphoneos/CactusSwift.framework"
SIMULATOR_FRAMEWORK="build/Build/Products/Release-iphonesimulator/CactusSwift.framework"

if [ ! -d "$DEVICE_FRAMEWORK" ]; then
  echo "Device framework not found at $DEVICE_FRAMEWORK"
  exit 1
fi

if [ ! -d "$SIMULATOR_FRAMEWORK" ]; then
  echo "Simulator framework not found at $SIMULATOR_FRAMEWORK"
  exit 1
fi

xcodebuild -create-xcframework \
  -framework "$DEVICE_FRAMEWORK" \
  -framework "$SIMULATOR_FRAMEWORK" \
  -output swift/CactusSwift.xcframework

echo "CactusSwift.xcframework built successfully!"

rm -rf build
 
 
 