#!/bin/bash

set -e
set -x

# Compile LFort runtime library
CFLAGS="-Wall -g"
gcc $CFLAGS -o lfort_intrinsics.o -c lfort_intrinsics.c
ar rcs liblfort.a lfort_intrinsics.o
