#!/bin/bash

DIR=$(dirname "$0")

TYPE=${1:-'Release'}
echo "build type: $TYPE"

pushd $DIR

rm -rf build
mkdir -p build
pushd build

cmake .. -DCMAKE_BUILD_TYPE=$TYPE
make -j4

popd

popd
