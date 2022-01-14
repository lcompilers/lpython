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
echo "PATH=$PATH"
llvm-config --components

# Generate the `version` file
bash ci/version.sh

# Generate a Fortran AST from AST.asdl (C++)
python grammar/asdl_cpp.py
# Generate a Fortran ASR from ASR.asdl (C++)
python grammar/asdl_cpp.py grammar/ASR.asdl src/libasr/asr.h
# Generate a Python AST from Python.asdl (C++)
python grammar/asdl_cpp.py grammar/Python.asdl src/lfortran/python_ast.h

# Generate the tokenizer and parser
pushd src/lfortran/parser && re2c -W -b tokenizer.re -o tokenizer.cpp && popd
pushd src/lfortran/parser && re2c -W -b preprocessor.re -o preprocessor.cpp && popd
pushd src/lfortran/parser && bison -Wall -d -r all parser.yy && popd

$lfortran_version=$(cat version).strip()
$dest="lfortran-" + $lfortran_version
bash ci/create_source_tarball0.sh
tar xzf dist/lfortran-$lfortran_version.tar.gz
cd lfortran-$lfortran_version

mkdir test-bld
cd test-bld
# Note: we have to build in Release mode on Windows, because `llvmdev` is
# compiled in Release mode and we get link failures if we mix and match build
# modes:
BUILD_TYPE = "Release"
cmake -G $LFORTRAN_CMAKE_GENERATOR -DCMAKE_VERBOSE_MAKEFILE=ON -DWITH_LLVM=yes -DWITH_XEUS=yes -DCMAKE_PREFIX_PATH=$CONDA_PREFIX -DCMAKE_INSTALL_PREFIX=$CONDA_PREFIX -DCMAKE_BUILD_TYPE=@(BUILD_TYPE) ..
cmake --build . --target install
./src/lfortran/tests/test_lfortran
./src/bin/lfortran < ../src/bin/example_input.txt
ctest --output-on-failure
cpack -V
cd ../..

jupyter kernelspec list --json
#python ci/test_fortran_kernel.py -v
#
cd share/lfortran/nb
jupyter nbconvert --to notebook --execute --ExecutePreprocessor.timeout=60 --output Demo1_out.ipynb Demo1.ipynb
jupyter nbconvert --to notebook --execute --ExecutePreprocessor.timeout=60 --output Demo2_out.ipynb Demo2.ipynb
cat Demo1_out.ipynb
jupyter nbconvert --to notebook --execute --ExecutePreprocessor.timeout=60 --output "Hello World_out.ipynb" "Hello World.ipynb"
jupyter nbconvert --to notebook --execute --ExecutePreprocessor.timeout=60 --output "Operators Control Flow_out.ipynb" "Operators Control Flow.ipynb"
jupyter nbconvert --to notebook --execute --ExecutePreprocessor.timeout=60 --output Variables_out.ipynb Variables.ipynb
cd ../../..

cp lfortran-$lfortran_version/test-bld/src/bin/lfortran src/bin
cp lfortran-$lfortran_version/test-bld/src/bin/cpptranslate src/bin
if $WIN == "1":
    cp lfortran-$lfortran_version/test-bld/src/runtime/legacy/lfortran_runtime* src/runtime/
else:
    cp lfortran-$lfortran_version/test-bld/src/runtime/liblfortran_runtime* src/runtime/
cp lfortran-$lfortran_version/test-bld/src/runtime/*.mod src/runtime/

# Run some simple compilation tests, works everywhere:
src/bin/lfortran --version
# Compile and link separately
src/bin/lfortran -c examples/expr2.f90 -o expr2.o
src/bin/lfortran -o expr2 expr2.o
./expr2

# Compile C and Fortran
src/bin/lfortran -c integration_tests/modules_15b.f90 -o modules_15b.o
src/bin/lfortran -c integration_tests/modules_15.f90 -o modules_15.o
if $WIN == "1": # Windows
    cl /MD /c integration_tests/modules_15c.c /Fomodules_15c.o
elif $MACOS == "1": # macOS
    clang -c integration_tests/modules_15c.c -o modules_15c.o
else: # Linux
    gcc -c integration_tests/modules_15c.c -o modules_15c.o
src/bin/lfortran modules_15.o modules_15b.o modules_15c.o -o modules_15
./modules_15


# Compile and link in one step
src/bin/lfortran integration_tests/intrinsics_04s.f90 -o intrinsics_04s
./intrinsics_04s

src/bin/lfortran integration_tests/intrinsics_04.f90 -o intrinsics_04
./intrinsics_04


# Run all tests (does not work on Windows yet):
cmake --version
if $WIN != "1":
    ./run_tests.py

    cd integration_tests
    mkdir build-lfortran-llvm
    cd build-lfortran-llvm
    $FC="../../src/bin/lfortran"
    cmake -DLFORTRAN_BACKEND=llvm ..
    make
    ctest -L llvm
    cd ../..
