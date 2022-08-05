#!/usr/bin/env bash

set -e
set -x

# Generate the `version` file
ci/version.sh

# Generate a Python AST from Python.asdl (Python)
python grammar/asdl_py.py
# Generate a Python AST from Python.asdl (C++)
python src/libasr/asdl_cpp.py grammar/Python.asdl src/lpython/python_ast.h
# Generate a Fortran ASR from ASR.asdl (C++)
python src/libasr/asdl_cpp.py src/libasr/ASR.asdl src/libasr/asr.h
# Generate a wasm_visitor.h from src/libasr/wasm_instructions.txt (C++)
python src/libasr/wasm_instructions_visitor.py

# Generate the tokenizer and parser
(cd src/lpython/parser && re2c -W -b tokenizer.re -o tokenizer.cpp)
(cd src/lpython/parser && bison -Wall -d -r all parser.yy)

grep -n "'" src/lpython/parser/parser.yy && echo "Single quote not allowed" && exit 1
echo "OK"
