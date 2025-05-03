#!/bin/bash -e

export LEFTHOOK=0

# Initialize release flags
release_react=false
release_flutter=false
release_android=false

# Parse command-line arguments
for arg in "$@"
do
  case $arg in
    --release-react)
    release_react=true
    shift # Remove --release-react from processing
    ;;
    --release-flutter)
    release_flutter=true
    shift # Remove --release-flutter from processing
    ;;
    --release-android)
    release_android=true
    shift # Remove --release-android from processing
    ;;
    *)
    # Unknown option
    ;;
  esac
done

# Determine the root directory regardless of where this script is called from
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

# Test the core Cactus Engine
"$SCRIPT_DIR/test-cactus.sh"

# Build native iOS (and tvOS) frameworks
"$SCRIPT_DIR/build-ios.sh"

# Build native Android libraries (.so)
"$SCRIPT_DIR/build-android.sh"

# Build the React‑Native JS package (TS ➜ JS, etc.)
"$SCRIPT_DIR/build-react.sh"

# Build the Flutter package
"$SCRIPT_DIR/build-flutter.sh"

echo "All build steps completed successfully."

# Release the React Native package if flag is set
if [ "$release_react" = true ]; then
  echo "Releasing React Native package..."
  cd "$ROOT_DIR/react"
  yarn release
  cd ..
fi

# Release the Flutter package if flag is set
if [ "$release_flutter" = true ]; then
  echo "Releasing Flutter package..."
  cd "$ROOT_DIR/flutter"
  flutter pub publish
  cd ..
fi

# Release the Android library if flag is set
# create a file ~/.gradle/gradle.properties in the android directory
# add the following:
# gpr.user=cactus-compute
# gpr.key=GITHUB_TOKEN 

if [ "$release_android" = true ]; then
  echo "Releasing Android library..."
  cd "$ROOT_DIR/android"
  ./gradlew publishReleasePublicationToGitHubPackagesRepository
  cd ..
fi

