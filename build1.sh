#!/bin/bash

set -e
set -x

cmake -DCMAKE_INSTALL_PREFIX=`pwd` -DBUILD_SHARED_LIBS=OFF .
make install
cmake -DCMAKE_INSTALL_PREFIX=`pwd` -DBUILD_SHARED_LIBS=ON .
make install
