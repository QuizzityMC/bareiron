#!/usr/bin/env bash

# Build script for WebAssembly target using Emscripten
# Requires emscripten SDK to be installed and activated

# Check for registries before attempting to compile
if [ ! -f "include/registries.h" ]; then
  echo "Error: 'include/registries.h' is missing."
  echo "Please follow the 'Compilation' section of the README to generate it."
  exit 1
fi

# Check if emcc is available
if ! command -v emcc &> /dev/null; then
  echo "Error: emcc (Emscripten compiler) not found."
  echo "Please install and activate the Emscripten SDK:"
  echo "  https://emscripten.org/docs/getting_started/downloads.html"
  exit 1
fi

echo "Building bareiron for WebAssembly..."

# Compile with Emscripten
# -O2: Optimize for size and speed
# -s ASYNCIFY: Enable synchronous-looking async code
# -s ALLOW_MEMORY_GROWTH: Allow memory to grow as needed
# -s WEBSOCKET_URL: Enable WebSocket support
# -s PROXY_TO_PTHREAD: Run main in a worker thread
# -s EXPORTED_RUNTIME_METHODS: Export functions needed by JS
# -s EXPORTED_FUNCTIONS: Export main function
emcc src/*.c \
  -O2 \
  -Iinclude \
  -DWASM_BUILD \
  -s ASYNCIFY=1 \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]' \
  -s EXPORTED_FUNCTIONS='["_main","_malloc","_free","_wasm_add_client_connection","_wasm_remove_client_connection"]' \
  -s ENVIRONMENT='web,worker' \
  -s PROXY_TO_PTHREAD=0 \
  -s MODULARIZE=1 \
  -s EXPORT_NAME='BareironModule' \
  -s WEBSOCKET_SUBPROTOCOL='binary' \
  -s EXIT_RUNTIME=0 \
  -o web/bareiron.js

if [ $? -eq 0 ]; then
  echo "Build successful! Output: web/bareiron.js and web/bareiron.wasm"
  echo "You can now serve the 'web' directory with any HTTP server."
else
  echo "Build failed!"
  exit 1
fi
