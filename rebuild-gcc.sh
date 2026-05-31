#!/bin/bash
set -e

rm -rf build

echo "================================================================================"
echo "Building GCC Pass and Runtime Object"
# make sure using the same version of clang as llvm pass version
cmake -B build \
  -DCMAKE_CXX_COMPILER=g++ \
  -DCMAKE_C_COMPILER=gcc \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=1

cmake --build ./build
echo "================================================================================"

echo "================================================================================"
echo "Running Example Program"
./build/example
echo "================================================================================"

echo "================================================================================"
echo "Running Multithread Program"
./build/multithread
echo "================================================================================"