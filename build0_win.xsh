#!/usr/bin/env xonsh

#bash ci/version.sh
version=$(git describe --tags --dirty).strip()[1:]
echo @(version) > version

# Generate a Python AST from Python.asdl (Python)
python grammar/asdl_py.py
# Generate a Python AST from Python.asdl (C++)
python libasr/src/libasr/asdl_cpp.py grammar/Python.asdl src/lpython/python_ast.h
# Generate a Fortran ASR from ASR.asdl (C++)
python libasr/src/libasr/asdl_cpp.py libasr/src/libasr/ASR.asdl libasr/src/libasr/asr.h
# Generate a wasm_visitor.h from libasr/src/libasr/wasm_instructions.txt (C++)
python libasr/src/libasr/wasm_instructions_visitor.py
# Generate the intrinsic_function_registry_util.h (C++)
python libasr/src/libasr/intrinsic_func_registry_util_gen.py

# Generate the tokenizer and parser
pushd src/lpython/parser && re2c -W -b tokenizer.re -o tokenizer.cpp && popd
pushd src/lpython/parser && bison -Wall -d -r all parser.yy && popd
