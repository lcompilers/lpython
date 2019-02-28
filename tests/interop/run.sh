#!/bin/bash

set -ex

FFLAGS_DEBUG="-Wall -Wextra -Wimplicit-interface -fPIC -g -fcheck=all -fbacktrace"
cmake \
    -DBUILD_SHARED_LIBS=yes \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_Fortran_FLAGS_DEBUG="$FFLAGS_DEBUG" \
    .
make
ctest

python wrap.py > mod2.f90
./mod2.sh
