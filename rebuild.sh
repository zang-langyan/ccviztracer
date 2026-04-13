#!/bin/bash
set -e

LLVM_HOME="/opt/homebrew/opt/llvm"
# export PATH="$LLVM_HOME/bin:$PATH"

rm -rf build

echo "================================================================================"
echo "Building LLVM Pass and Runtime Object"
cmake -B build \
  -DCMAKE_CXX_COMPILER=$LLVM_HOME/bin/clang++ \
  -DCMAKE_C_COMPILER=$LLVM_HOME/bin/clang \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=1

cmake --build ./build
echo "================================================================================"

echo "================================================================================"
echo "Running Example Program"
./build/example
echo "================================================================================"

