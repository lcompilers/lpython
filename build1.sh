#!/bin/bash

set -e
set -x

# Compile LFort runtime library
CFLAGS="-Wall -g -fPIC"
gcc $CFLAGS -o lfort_intrinsics.o -c lfort_intrinsics.c
ar rcs liblfortran.a lfort_intrinsics.o
gcc -shared -o liblfortran.so lfort_intrinsics.o
mkdir -p share/lfortran/lib
mv liblfortran.a share/lfortran/lib/
mv liblfortran.so share/lfortran/lib/
