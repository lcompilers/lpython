#!/bin/bash

set -e
set -x

# Generate a Fortran AST from AST.asdl (Python)
python grammar/asdl_py.py
# Generate a Fortran AST from AST.asdl (C++)
python grammar/asdl_cpp.py
# Generate a Fortran ASR from ASR.asdl
python grammar/asdl_py.py grammar/ASR.asdl lfortran/asr/asr.py ..ast.utils

# Generate the tokenizer and parser
(cd src/lfortran/parser && re2c -W -b tokenizer.re -o tokenizer.cpp)
(cd src/lfortran/parser && bison -Wall -d -r all parser.yy)

# Generate a parse tree from fortran.g4
antlr4="java org.antlr.v4.Tool"
(cd grammar && $antlr4 -Dlanguage=Python3 -no-listener -visitor fortran.g4 -o ../lfortran/parser)
