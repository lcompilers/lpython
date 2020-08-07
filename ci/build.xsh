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
#
# * One must be careful to ensure this Xonsh script is in the path. If Xonsh
#   cannot find the script, it will return success, making the CI tests
#   "succeed" (https://github.com/xonsh/xonsh/issues/3292)

$RAISE_SUBPROC_ERROR = True
trace on

echo "CONDA_PREFIX=$CONDA_PREFIX"
llvm-config --components

# Generate a Fortran AST from AST.asdl (C++)
python grammar/asdl_cpp.py
# Generate a Fortran ASR from ASR.asdl (C++)
python grammar/asdl_cpp.py grammar/ASR.asdl src/lfortran/asr.h

# Generate the tokenizer and parser
pushd src/lfortran/parser && re2c -W -b tokenizer.re -o tokenizer.cpp && popd
pushd src/lfortran/parser && bison -Wall -d -r all parser.yy && popd

$lfortran_version=$(cat version).strip()
python setup.py sdist
tar xzf dist/lfortran-$lfortran_version.tar.gz
cd lfortran-$lfortran_version

mkdir test-bld
cd test-bld
cmake -G $LFORTRAN_CMAKE_GENERATOR -DCMAKE_VERBOSE_MAKEFILE=ON -DWITH_LLVM=yes -DCMAKE_PREFIX_PATH=$CONDA_PREFIX -DCMAKE_BUILD_TYPE=Release ..
if $WIN == "1":
    cmake --build . --config Release
    ./src/lfortran/tests/Release/test_llvm -s
else:
    cmake --build .
    ./src/lfortran/tests/test_llvm -s
./src/bin/lfortran < ../src/bin/example_input.txt
ctest --output-on-failure
cpack -V
cd ../..

if $WIN != "1":
    cp lfortran-$lfortran_version/test-bld/src/bin/lfortran src/bin
    cp lfortran-$lfortran_version/test-bld/src/bin/cpptranslate src/bin
    ./run_tests.py
