#!/bin/bash
# Check if emcc is in path, if not try to source from emsdk
if ! command -v emcc &> /dev/null
then
    if [ -d "emsdk" ]; then
        echo "emcc not found, sourcing emsdk..."
        source emsdk/emsdk_env.sh
    else
        echo "Error: emcc not found and emsdk directory not present."
        exit 1
    fi
fi

# Compile the project
emcc main.cpp -o index.js \
  -s WASM=1 \
  -s USE_WEBGL2=1 \
  -s FULL_ES3=1 \
  -s ALLOW_MEMORY_GROWTH=1 \
  -O3 \
  --std=c++17

if [ $? -eq 0 ]; then
    echo "Build successful! Created index.js and index.wasm"
else
    echo "Build failed."
fi
