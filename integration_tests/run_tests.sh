#!/bin/bash

set -ex

export PATH="$(pwd)/../src/bin:$PATH"

cmake .
ctest --output-on-failure

cmake -DLPYTHON_BACKEND=llvm .
make
ctest --output-on-failure

cmake -DLPYTHON_BACKEND=c .
make
ctest --output-on-failure
