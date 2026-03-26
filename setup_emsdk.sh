#!/bin/bash
# Clone the repository
if [ ! -d "emsdk" ]; then
  git clone https://github.com/emscripten-core/emsdk.git
fi

# Enter the directory
cd emsdk

# Fetch the latest version
git pull

# Install and activate the latest SDK
./emsdk install latest
./emsdk activate latest

# Source the environment variables (manual step for the user)
echo "--------------------------------------------------------"
echo "Installation complete. To use emcc, run:"
echo "source emsdk/emsdk_env.sh"
echo "--------------------------------------------------------"
