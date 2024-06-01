#!/usr/bin/env xonsh

# A platform independent Xonsh script to build LFortran. Works on Linux, macOS
# and Windows. The prerequisites such as bison, re2c or cython must be already
# installed.
#
# Some issues (with Xonsh):
#
# * Some output seems to be lost on Windows
#   (https://github.com/xonsh/xonsh/issues/3291)
#
# * The commands that are executed are sometimes printed after the output,
#   which makes the log unreadable (https://github.com/xonsh/xonsh/issues/3293)
#
# * This script is too slow (due to https://github.com/xonsh/xonsh/issues/3064)
#   to be suitable for day to day development, but on the CI the few extra
#   seconds do not matter much

$RAISE_SUBPROC_ERROR = True
trace on

echo "CONDA_PREFIX=$CONDA_PREFIX"
echo "PATH=$PATH"
llvm-config --components

# Generate the `version` file
bash ci/version.sh

# Generate a Fortran ASR from ASR.asdl (C++)
python src/libasr/asdl_cpp.py src/libasr/ASR.asdl src/libasr/asr.h
# Generate a Python AST from Python.asdl (C++)
python src/libasr/asdl_cpp.py grammar/Python.asdl src/lpython/python_ast.h
# Generate a Python AST from Python.asdl (Python)
python grammar/asdl_py.py
# Generate a wasm_visitor.h from src/libasr/wasm_instructions.txt (C++)
python src/libasr/wasm_instructions_visitor.py
# Generate the intrinsic_function_registry_util.h (C++)
python src/libasr/intrinsic_func_registry_util_gen.py

# Generate the tokenizer and parser
pushd src/lpython/parser && re2c -W -b tokenizer.re -o tokenizer.cpp && popd
pushd src/lpython/parser && bison -Wall -d -r all parser.yy && popd

$lpython_version=$(cat version).strip()
$dest="lpython-" + $lpython_version
bash ci/create_source_tarball0.sh
tar xzf dist/lpython-$lpython_version.tar.gz
cd lpython-$lpython_version

mkdir test-bld
cd test-bld
# Note: we have to build in Release mode on Windows, because `llvmdev` is
# compiled in Release mode and we get link failures if we mix and match build
# modes:
BUILD_TYPE = "Release"
cmake -G $LFORTRAN_CMAKE_GENERATOR -DCMAKE_VERBOSE_MAKEFILE=ON -DWITH_LLVM=yes -DWITH_LSP=yes -DWITH_XEUS=yes -DCMAKE_PREFIX_PATH=$CONDA_PREFIX -DCMAKE_INSTALL_PREFIX=$CONDA_PREFIX -DWITH_LFORTRAN_BINARY_MODFILES=no -DCMAKE_BUILD_TYPE=@(BUILD_TYPE) -DWITH_RUNTIME_STACKTRACE=$ENABLE_RUNTIME_STACKTRACE ..
cmake --build . --target install -j16
./src/lpython/tests/test_lpython
#./src/bin/lpython < ../src/bin/example_input.txt
ctest --output-on-failure -j16
cpack -V
cd ../..

jupyter kernelspec list --json

cp lpython-$lpython_version/test-bld/src/bin/lpython src/bin
if $WIN == "1":
    cp lpython-$lpython_version/test-bld/src/runtime/lpython_runtime* src/runtime/
else:
    cp lpython-$lpython_version/test-bld/src/runtime/liblpython_runtime* src/runtime/
