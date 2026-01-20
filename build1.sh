#!/usr/bin/env bash

set -e
set -x

SRCDIR=`pwd`
CMAKE_PREFIX_PATH="$CMAKE_PREFIX_PATH_LPYTHON;$CONDA_PREFIX"

# if we are in a pixi project, use the pixi "host" environment for the dependencies
if [[ ! -z $PIXI_PROJECT_ROOT ]]; then
   CMAKE_PREFIX_PATH=$PIXI_PROJECT_ROOT/.pixi/envs/host/
   OUTOFTREE=false

   if [[ ! -f src/libasr/asr.h ]]; then
       pixi run build_0
   fi
fi

# if the outoftree env var is set to true, create a build directory
if [[ "$OUTOFTREE" == "true" ]]; then
    mkdir -p build
    cd build
fi


cmake $SRCDIR \
    -GNinja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DWITH_LLVM=yes \
    -DLPYTHON_BUILD_ALL=yes \
    -DWITH_STACKTRACE=yes \
    -DWITH_RUNTIME_STACKTRACE=yes \
    -DWITH_LSP=no \
    -DWITH_LFORTRAN_BINARY_MODFILES=no \
    -DCMAKE_PREFIX_PATH=$CMAKE_PREFIX_PATH \
    -DCMAKE_INSTALL_PREFIX=`pwd`/inst

cmake --build . -j --target install
