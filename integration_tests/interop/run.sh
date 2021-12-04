#!/usr/bin/env bash

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

python wrap2.py > mod2.f90
./mod2.sh

python wrap_cpp.py > mod2.h
make

python cxxwrap.py > cxx_wrapper.f90
gfortran -c cxx_wrapper.f90 -o cxx_wrapper.o
