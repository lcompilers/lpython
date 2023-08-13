#!/usr/bin/env python

import argparse
import subprocess
import os

# Initialization
DEFAULT_THREADS_TO_USE = 8 # default no of threads is 8
SUPPORTED_BACKENDS = ['llvm', 'c', 'wasm', 'cpython', 'x86', 'wasm_x86', 'wasm_x64', 'c_py', 'c_sym', 'cpython_sym', 'llvm_sym', 'llvm_py']
BASE_DIR = os.path.dirname(os.path.realpath(__file__))
LPYTHON_PATH = f"{BASE_DIR}/../src/bin"

fast_tests = "no"
python_libs_req = "no"

def run_cmd(cmd, cwd=None):
    print(f"+ {cmd}")
    process = subprocess.run(cmd, shell=True, cwd=cwd)
    if process.returncode != 0:
        print(f"Command failed: {cmd}")
        exit(1)


def run_test(backend):
    run_cmd(f"mkdir {BASE_DIR}/_lpython-tmp-test-{backend}", cwd=BASE_DIR)
    cwd = f"{BASE_DIR}/_lpython-tmp-test-{backend}"
    run_cmd(f"cmake -DKIND={backend} -DFAST={fast_tests} -DPYTHON_LIBS_REQ={python_libs_req} ..", cwd=cwd)
    run_cmd(f"cmake --build . --parallel {DEFAULT_THREADS_TO_USE}", cwd=cwd)
    run_cmd(f"ctest -j{DEFAULT_THREADS_TO_USE} --output-on-failure",
            cwd=cwd)


def test_backend(backend):
    if backend not in SUPPORTED_BACKENDS:
        print(f"Unsupported Backend: {backend}\n")
        return
    run_test(backend)


def get_args():
    parser = argparse.ArgumentParser(description="LPython Integration Test Suite")
    parser.add_argument("-j", "-n", "--no_of_threads", type=int,
                help="Parallel testing on given number of threads")
    parser.add_argument("-b", "--backends", nargs="*", default=["llvm", "cpython"],
                type=str, help="Test the requested backends (%s), default: llvm, cpython" % \
                        ", ".join(SUPPORTED_BACKENDS))
    parser.add_argument("-f", "--fast", action='store_true',
                help="Run supported tests with --fast")
    return parser.parse_args()


def main():
    args = get_args()

    # Setup
    global DEFAULT_THREADS_TO_USE, fast_tests, python_libs_req
    os.environ["PATH"] = LPYTHON_PATH + os.pathsep + os.environ["PATH"]
    # delete previously created directories (if any)
    for backend in SUPPORTED_BACKENDS:
        run_cmd(f"rm -rf {BASE_DIR}/_lpython-tmp-test-{backend}")

    DEFAULT_THREADS_TO_USE = args.no_of_threads or DEFAULT_THREADS_TO_USE
    fast_tests = "yes" if args.fast else "no"
    for backend in args.backends:
        python_libs_req = "yes" if backend in ["c_py", "c_sym", "llvm_sym", 'llvm_py'] else "no"
        test_backend(backend)


if __name__ == "__main__":
    main()
