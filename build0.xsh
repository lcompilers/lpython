#!/usr/bin/env xonsh

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
pushd grammar && java org.antlr.v4.Tool -Dlanguage=Python3 -no-listener -visitor fortran.g4 -o ../lfortran/parser && popd
