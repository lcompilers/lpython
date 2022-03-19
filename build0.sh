#!/usr/bin/env bash

set -e
set -x

# Generate the `version` file
ci/version.sh

# Generate a Fortran AST from AST.asdl (C++)
python grammar/asdl_cpp.py
# Generate a Python AST from Python.asdl (Python)
python grammar/asdl_py.py
# Generate a Python AST from Python.asdl (C++)
python grammar/asdl_cpp.py grammar/Python.asdl src/lpython/python_ast.h
# Generate a Fortran ASR from ASR.asdl (C++)
python grammar/asdl_cpp.py grammar/ASR.asdl src/libasr/asr.h
