#!/bin/bash

set -ex

# Append "-j4" or "-j" to run in parallel
jn=$1

export PATH="$(pwd)/../src/bin:$PATH"

cmake .
ctest $jn --output-on-failure

cmake -DLPYTHON_BACKEND=llvm .
make $jn
ctest $jn --output-on-failure

cmake -DLPYTHON_BACKEND=c .
make $jn
ctest $jn --output-on-failure
