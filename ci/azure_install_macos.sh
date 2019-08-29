#!/bin/bash

set -ex

conda config --set always_yes yes --set changeps1 no
conda info -a
conda update -q conda
conda install -c conda-forge python=3.7 re2c bison m4 cython xonsh scikit-build
export MACOSX_DEPLOYMENT_TARGET="10.9"
xonsh ci/build.xsh
