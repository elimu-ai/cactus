#!/bin/bash -e

export LEFTHOOK=0 

# Determine the root directory regardless of where this script is called from
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# 1. Sync / patch cactus sources from the pinned llama.cpp commit
"$SCRIPT_DIR/patch-from-llama-cpp.sh"

# 2. Run test
"$SCRIPT_DIR/test-cactus.sh"

# 3. Build native iOS (and tvOS) frameworks
"$SCRIPT_DIR/build-ios.sh"

# 4. Build native Android libraries (.so)
"$SCRIPT_DIR/build-android.sh"

# 5. Build the React‑Native JS package (TS ➜ JS, etc.)
"$SCRIPT_DIR/build-react.sh"

# 6. Build the Swift package
"$SCRIPT_DIR/build-swift.sh"

 echo "All build steps completed successfully."
