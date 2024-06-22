#!/usr/bin/env bash

set -e
set -x

mkdir -p ./build
cd build

cmake .. \
    -GNinja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DWITH_LLVM=yes \
    -DLPYTHON_BUILD_ALL=yes \
    -DWITH_STACKTRACE=yes \
    -DWITH_RUNTIME_STACKTRACE=yes \
    -DWITH_INTRINSIC_MODULES=yes \
    -DWITH_LSP=no \
    -DWITH_LFORTRAN_BINARY_MODFILES=no \
    -DCMAKE_PREFIX_PATH="$PIXI_PROJECT_ROOT/.pixi/envs/host/" \
    -DCMAKE_INSTALL_PREFIX=`pwd`/inst \

cmake --build . --target install
