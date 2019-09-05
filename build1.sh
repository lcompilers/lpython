#!/bin/bash

set -e
set -x

cmake \
    -DCMAKE_BUILD_TYPE=Debug \
    -DWITH_PYTHON=yes \
    -DWITH_LLVM=yes \
    -DCMAKE_INSTALL_PREFIX=`pwd` \
    .
cmake --build . --target install
