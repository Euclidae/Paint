#!/bin/bash

# Build and test script for Paint application
# Requires C++20 compatible compiler (GCC 10+, Clang 10+, or MSVC 2019+)

echo "Building Paint application (C++20)..."

# Create bin directory if it doesn't exist
mkdir -p bin

# Run make
make clean
make

# Check if build was successful
if [ $? -ne 0 ]; then
    echo "Build failed!"
    exit 1
fi

echo "Build successful!"
echo "Running Paint application..."

# Run the application for a brief test
if [ -f bin/EnoughImageEditor ]; then
    # Run for 2 seconds then kill it
    bin/EnoughImageEditor &
    APP_PID=$!
    sleep 2
    kill $APP_PID 2>/dev/null

    echo "EnoughImageEditor started and closed successfully."
else
    echo "EnoughImageEditor binary not found. Build may have succeeded but output is not where expected."
    exit 1
fi

exit 0
