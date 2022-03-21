#!/usr/bin/env bash

bash ci/version.sh
# Generate a Python AST from Python.asdl (Python)
python grammar/asdl_py.py
# Generate a Python AST from Python.asdl (C++)
python grammar/asdl_cpp.py grammar/Python.asdl src/lpython/python_ast.h
# Generate a Fortran ASR from ASR.asdl (C++)
python grammar/asdl_cpp.py grammar/ASR.asdl src/libasr/asr.h
