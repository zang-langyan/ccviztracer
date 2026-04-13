cmake --build ./build
if [ $? -ne 0 ]; then
    echo "Build failed, exiting."
    exit 1
fi
./build/example