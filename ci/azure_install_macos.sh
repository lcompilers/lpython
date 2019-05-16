#!/bin/bash

set -ex

conda config --set always_yes yes --set changeps1 no
conda info -a
conda update -q conda
conda install python=3.7 pytest llvmlite prompt_toolkit cmake make
conda install -c conda-forge scikit-build
pip install antlr4-python3-runtime

python grammar/asdl_py.py
python grammar/asdl_py.py grammar/ASR.asdl lfortran/asr/asr.py ..ast.utils

cd grammar
curl -O https://www.antlr.org/download/antlr-4.7-complete.jar
java -cp antlr-4.7-complete.jar org.antlr.v4.Tool -Dlanguage=Python3 -no-listener -visitor fortran.g4 -o ../lfortran/parser
cd ..

ci/version.sh
lfortran_version=$(<version)

python setup.py sdist
tar xzf dist/lfortran-${lfortran_version}.tar.gz
cd lfortran-${lfortran_version}
pip install -v .
cd ..
rm -rf lfortran-${lfortran_version} lfortran
ls -l
py.test --pyargs lfortran
