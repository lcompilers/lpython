#!/bin/bash

set -e
set -x

cmake \
    -DCMAKE_BUILD_TYPE=Debug \
    -DWITH_PYTHON=yes \
    -DCMAKE_INSTALL_PREFIX=`pwd` \
    .
cmake --build . --target install
