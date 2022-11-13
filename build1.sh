#!/usr/bin/env bash

set -e
set -x

WITH_STACKTRACE=yes

if [[ -n "${CC}" ]]; then
  CC=$(which "${CC}")
fi

if [[ -n "${CXX}" ]]; then
  CXX=$(which "${CXX}")
fi

if [[ -n "${CONDA_PREFIX}" ]] && [[ "$(echo ${CC} | grep -m 1 -c ${CONDA_PREFIX})" == 1 ]]; then
    GCC_INCLUDE_PATH=$(dirname $(find ${CONDA_PREFIX} -name cxxabi.h))
    GCC_ARCH_INCLUDE_PATH=$(dirname $(dirname $(find ${GCC_INCLUDE_PATH} -name c++config.h)))

    # ref: https://github.com/zdevito/ATen/issues/183
    EXTRA_FLAGS="-fPIC -no-pie -D__STDC_FORMAT_MACROS"
    EXTRA_FLAGS="${EXTRA_FLAGS} -L${CONDA_PREFIX}/lib -Wl,-rpath=${CONDA_PREFIX}/lib"
    EXTRA_FLAGS="${EXTRA_FLAGS} -I${CONDA_PREFIX}/include"
    EXTRA_FLAGS="${EXTRA_FLAGS} -I${GCC_INCLUDE_PATH}"
    EXTRA_FLAGS="${EXTRA_FLAGS} -I${GCC_ARCH_INCLUDE_PATH}/"

    WITH_STACKTRACE=no

    export CFLAGS="${CFLAGS} ${EXTRA_FLAGS}"
    export CXXFLAGS="${CXXFLAGS} ${EXTRA_FLAGS}"
fi

cmake \
    -DCMAKE_BUILD_TYPE=Debug \
    -DWITH_LLVM=yes \
    -DLPYTHON_BUILD_ALL=yes \
    -DWITH_STACKTRACE="${WITH_STACKTRACE}" \
    -DWITH_LSP=no \
    -DWITH_LFORTRAN_BINARY_MODFILES=no \
    -DCMAKE_PREFIX_PATH="$CMAKE_PREFIX_PATH_LPYTHON;$CONDA_PREFIX" \
    -DCMAKE_INSTALL_PREFIX="$(pwd)/inst" \
    .
cmake --build . -j16 --target install
