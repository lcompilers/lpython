#!/usr/bin/env bash

set -e
set -x

cmake \
    -DCMAKE_BUILD_TYPE=Debug \
    -DWITH_LLVM=yes \
    -DLPYTHON_BUILD_ALL=yes \
    -DWITH_STACKTRACE=yes \
    -DWITH_RUNTIME_STACKTRACE=yes \
    -DWITH_LSP=no \
    -DWITH_LCOMPILERS_BINARY_MODFILES=no \
    -DCMAKE_PREFIX_PATH="$CMAKE_PREFIX_PATH_LPYTHON;$CONDA_PREFIX" \
    -DCMAKE_INSTALL_PREFIX=`pwd`/inst \
    .
cmake --build . -j16 --target install
