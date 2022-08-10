#!/usr/bin/env bash

bash ci/version.sh
# Generate a Python AST from Python.asdl (Python)
python grammar/asdl_py.py
# Generate a Python AST from Python.asdl (C++)
python src/libasr/asdl_cpp.py grammar/Python.asdl src/lpython/python_ast.h
# Generate a Fortran ASR from ASR.asdl (C++)
python src/libasr/asdl_cpp.py src/libasr/ASR.asdl src/libasr/asr.h
