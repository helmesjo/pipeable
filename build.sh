#!/bin/bash

function on_error {
    echo "Build failed..."
    exit 1
}
trap on_error ERR

BUILD_DIR=build
CONFIG=Release

cmake -E make_directory $BUILD_DIR

cmake -E chdir $BUILD_DIR \
    cmake -DCMAKE_BUILD_TYPE=$CONFIG -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON ..
cmake -E chdir $BUILD_DIR \
    cmake --build . --config $CONFIG
cmake -E chdir $BUILD_DIR \
    ctest --build-config $CONFIG
