#!/bin/bash

set -ex

git clean -dfx

# Append "-j4" or "-j" to run in parallel
jn=$1

export PATH="$(pwd)/../src/bin:$PATH"

mkdir b1
cd b1
cmake -DKIND=cpython ..
make $jn
ctest $jn --output-on-failure
cd ..


mkdir b2
cd b2
cmake -DKIND=llvm ..
make $jn
ctest $jn --output-on-failure
cd ..

mkdir b3
cd b3

cmake -DKIND=c ..
make $jn
ctest $jn --output-on-failure
