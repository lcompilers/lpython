#!/bin/bash

set -ex

conda config --set always_yes yes --set changeps1 no
conda info -a
conda update -q conda
conda install -c conda-forge python=3.7 re2c bison m4 cython xonsh

xonsh ci\build.xsh
