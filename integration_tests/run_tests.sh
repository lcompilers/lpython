#!/bin/bash

set -ex

export PATH="$(pwd)/../src/bin:$PATH"

cmake .
ctest

cmake -DLPYTHON_BACKEND=llvm .
make
ctest

cmake -DLPYTHON_BACKEND=c .
make
ctest
