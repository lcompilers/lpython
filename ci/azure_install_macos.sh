#!/usr/bin/env bash

set -ex

conda config --set always_yes yes --set changeps1 no
conda info -a
conda update -q conda
conda install -c conda-forge python=3.8 re2c bison=3.4 m4 xonsh=0.16.0 llvmdev=11.0.1 toml cmake=3.17.0 jupyter pytest xeus=5.1.0 xeus-zmq=3.0.0 nlohmann_json=3.11.3 jupyter_kernel_test
export MACOSX_DEPLOYMENT_TARGET="10.12"
export CONDA_PREFIX=/usr/local/miniconda
export LFORTRAN_CMAKE_GENERATOR="Unix Makefiles"
export WIN=0
export MACOS=1
xonsh ci/build.xsh
