#!/usr/bin/env bash
# Generate compile_commands.json for clangd/editor support.
# Usage: ./build.sh [windows]
set -euo pipefail

target="${1:-linux}"
if [ "$target" = "windows" ]; then
  echo "Generating compile_commands.json for Windows (mingw) toolchain..."
  exec nix develop .#default --command bash -c '
    cmake -B build-windows -S . -G Ninja \
      -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
      -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    ln -sf build-windows/compile_commands.json compile_commands.json
  '
else
  echo "Generating compile_commands.json for Linux (clang) toolchain..."
  exec nix develop .#default --command bash -c '
    cmake -B build -S . -G Ninja \
      -DCMAKE_C_COMPILER=clang \
      -DCMAKE_CXX_COMPILER=clang++ \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    ln -sf build/compile_commands.json compile_commands.json
  '
fi
