#!/bin/bash

set -ex

conda config --set always_yes yes --set changeps1 no
conda info -a
conda update -q conda
conda install python=3.7 pytest llvmlite prompt_toolkit
pip install antlr4-python3-runtime

python grammar/asdl_py.py
python grammar/asdl_py.py grammar/ASR.asdl lfortran/asr/asr.py ..ast.utils

cd grammar
curl -O https://www.antlr.org/download/antlr-4.7-complete.jar
java -cp antlr-4.7-complete.jar org.antlr.v4.Tool -Dlanguage=Python3 -no-listener -visitor fortran.g4 -o ../lfortran/parser
cd ..

CFLAGS="-Wall -g -fPIC"
gcc $CFLAGS -o lfort_intrinsics.o -c lfort_intrinsics.c
ar rcs liblfortran.a lfort_intrinsics.o
gcc -shared -o liblfortran.so lfort_intrinsics.o
mkdir -p share/lfortran/lib
mv liblfortran.a share/lfortran/lib/
mv liblfortran.so share/lfortran/lib/

pytest
