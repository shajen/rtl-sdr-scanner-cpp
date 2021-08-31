#!/bin/bash

DIR=$(dirname "$0")

pushd $DIR/build
make -j4
popd
