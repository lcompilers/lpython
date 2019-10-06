#!/bin/bash

set -ex

conda config --set always_yes yes --set changeps1 no
conda info -a
conda update -q conda
conda install -c conda-forge python=3.7 re2c bison m4 cython xonsh scikit-build llvmdev pytest llvmlite prompt_toolkit antlr-python-runtime pygments
export MACOSX_DEPLOYMENT_TARGET="10.9"
export CONDA_PREFIX=/usr/local/miniconda
export LFORTRAN_CMAKE_GENERATOR="Unix Makefiles"
export WIN=0
export MACOS=1
xonsh ci/build.xsh
