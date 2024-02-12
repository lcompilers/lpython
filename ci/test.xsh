#!/usr/bin/env xonsh
#
$RAISE_SUBPROC_ERROR = True
trace on

# Run some simple compilation tests, works everywhere:
src/bin/lpython --version
# Compile and link separately
src/bin/lpython -c examples/expr2.py -o expr2.o
src/bin/lpython -o expr2 expr2.o
./expr2

# Test the new Python frontend, manually for now:
src/bin/lpython --show-ast tests/doconcurrentloop_01.py
src/bin/lpython --show-asr tests/doconcurrentloop_01.py
src/bin/lpython --show-cpp tests/doconcurrentloop_01.py

if $WIN == "1":
    python run_tests.py --skip-run-with-dbg --no-color
else:
    python run_tests.py
    src/bin/lpython examples/expr2.py
    src/bin/lpython --backend=c examples/expr2.py
    cd integration_tests

    if $(uname).strip() == "Linux":
        python run_tests.py -j16 -b llvm cpython c wasm
        python run_tests.py -j16 -b llvm cpython c wasm -f
        python run_tests.py -j16 -b x86 wasm_x86 wasm_x64
        python run_tests.py -j16 -b x86 wasm_x86 wasm_x64 -f
    else:
        python run_tests.py -j1 -b llvm cpython c wasm
        python run_tests.py -j1 -b llvm cpython c wasm -f
