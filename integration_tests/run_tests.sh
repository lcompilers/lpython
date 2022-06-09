#!/bin/bash

set -ex

cmake .
ctest

cmake -DLPYTHON_BACKEND=llvm .
make
ctest

cmake -DLPYTHON_BACKEND=c .
make
ctest
