#!/bin/bash

set -e
set -x

# Generate a Fortran AST from Fortran.asdl
python grammar/asdl_py.py
# Generate a Fortran ASG from ASG.asdl
python grammar/asdl_py.py grammar/ASG.asdl lfortran/asg/asg.py ..ast.utils

# Generate a parse tree from fortran.g4
antlr4="java org.antlr.v4.Tool"
(cd grammar; $antlr4 -Dlanguage=Python3 -no-listener -visitor fortran.g4 -o ../lfortran/parser)
