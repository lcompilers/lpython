#!/bin/bash

set -e
set -x

cmake \
    -DCMAKE_BUILD_TYPE=Debug \
    -DWITH_LLVM=yes \
    -DWITH_STACKTRACE=yes \
    -DCMAKE_INSTALL_PREFIX=`pwd` \
    .
cmake --build . --target install
