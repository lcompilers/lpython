#!/usr/bin/env bash

set -ex

# Generate the `version` file
ci/version.sh

# Generate a Python AST from Python.asdl (Python)
python grammar/asdl_py.py
# Generate a Python AST from Python.asdl (C++)
python src/libasr/src/libasr/asdl_cpp.py grammar/Python.asdl src/lpython/python_ast.h
# Generate a Fortran ASR from ASR.asdl (C++)
python src/libasr/src/libasr/asdl_cpp.py src/libasr/src/libasr/ASR.asdl src/libasr/src/libasr/asr.h
# Generate a wasm_visitor.h from src/libasr/src/libasr/wasm_instructions.txt (C++)
python src/libasr/src/libasr/wasm_instructions_visitor.py
# Generate the intrinsic_function_registry_util.h (C++)
python src/libasr/src/libasr/intrinsic_func_registry_util_gen.py

# Generate the tokenizer and parser
(cd src/lpython/parser && re2c -W -b tokenizer.re -o tokenizer.cpp)
(cd src/lpython/parser && bison -Wall -d -r all parser.yy)

python -c "file = 'src/lpython/parser/parser.tab.cc'
with open(file, 'r') as f: text = f.read()
with open(file, 'w') as f:
    f.write('[[maybe_unused]] int yynerrs'.join(text.split('int yynerrs')))"

grep -n "'" src/lpython/parser/parser.yy && echo "Single quote not allowed" && exit 1
echo "OK"
