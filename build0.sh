#!/bin/bash

set -e
set -x

# Generate a Fortran AST from AST.asdl (C++)
python grammar/asdl_cpp.py
# Generate a Fortran ASR from ASR.asdl (C++)
python grammar/asdl_cpp.py grammar/ASR.asdl src/lfortran/asr.h

# Generate the tokenizer and parser
(cd src/lfortran/parser && re2c -W -b tokenizer.re -o tokenizer.cpp)
(cd src/lfortran/parser && bison -Wall -d -r all parser.yy)

grep -n "'" src/lfortran/parser/parser.yy && echo "Single quote not allowed" && exit 1
echo "OK"
