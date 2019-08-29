#!/usr/bin/env xonsh

# A platform independent Xonsh script to build LFortran. Works on Linux, macOS
# and Windows. The prerequisites such as bison, re2c or cython must be already
# installed.

$RAISE_SUBPROC_ERROR = True
trace on

# Generate a Fortran AST from AST.asdl (Python)
python grammar/asdl_py.py
# Generate a Fortran AST from AST.asdl (C++)
python grammar/asdl_cpp.py
# Generate a Fortran ASR from ASR.asdl
python grammar/asdl_py.py grammar/ASR.asdl lfortran/asr/asr.py ..ast.utils

# Generate the tokenizer and parser
pushd src/lfortran/parser && re2c -W -b tokenizer.re -o tokenizer.cpp && popd
pushd src/lfortran/parser && bison -Wall -d -r all parser.yy && popd

#grep -n "'" src/lfortran/parser/parser.yy && echo "Single quote not allowed" && exit 1

pushd lfortran/parser && cython -3 -I. cparser.pyx && popd

# Generate a parse tree from fortran.g4
cd grammar
curl -O https://www.antlr.org/download/antlr-4.7-complete.jar
java -cp antlr-4.7-complete.jar org.antlr.v4.Tool -Dlanguage=Python3 -no-listener -visitor fortran.g4 -o ../lfortran/parser
cd ..

$lfortran_version=$(cat version)
pip install scikit-build
python setup.py sdist
pip uninstall -y scikit-build
tar xzf dist/lfortran-$lfortran_version.tar.gz
cd lfortran-$lfortran_version

mkdir test-bld
cd test-bld
cmake ..
cmake --build .
ctest --output-on-failure
cd ..

pip install .
cd ..

from shutil import rmtree
rmtree("lfortran")
rmtree("dist")
ls
pytest --pyargs lfortran
