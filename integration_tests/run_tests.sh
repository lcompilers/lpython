#!/bin/bash

set -ex

# Append "-j4" or "-j" to run in parallel
jn=$1

export PATH="$(pwd)/../src/bin:$PATH"

cmake -DKIND=cpython .
make $jn
ctest $jn --output-on-failure

cmake -DKIND=llvm .
make $jn
ctest $jn --output-on-failure

cmake -DKIND=c .
make $jn
ctest $jn --output-on-failure
