#!/usr/bin/env python

import argparse
import subprocess
import os

# Initialization
DEFAULT_THREADS_TO_USE = 8 # default no of threads is 8
SUPPORTED_BACKENDS = ['llvm', 'c', 'wasm', 'cpython', 'x86', 'wasm_x86', 'wasm_x64', 'c_py', 'cpython_py', 'c_sym', 'cpython_sym']
BASE_DIR = os.path.dirname(os.path.realpath(__file__))
LPYTHON_PATH = f"{BASE_DIR}/../src/bin"

disable_fast = "no"

def run_cmd(cmd, cwd=None):
    print(f"+ {cmd}")
    process = subprocess.run(cmd, shell=True, cwd=cwd)
    if process.returncode != 0:
        print(f"Command failed: {cmd}")
        exit(1)


def run_test(backend):
    run_cmd(f"mkdir {BASE_DIR}/_lpython-tmp-test-{backend}", cwd=BASE_DIR)
    cwd = f"{BASE_DIR}/_lpython-tmp-test-{backend}"
    run_cmd(f"cmake -DKIND={backend} -DDISABLE_FAST={disable_fast} ..", cwd=cwd)
    run_cmd(f"make -j{DEFAULT_THREADS_TO_USE}", cwd=cwd)
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
                type=str, help="Test the requested backends (%s)" % \
                        ", ".join(SUPPORTED_BACKENDS))
    parser.add_argument("-nf", "--no_fast", action='store_true',
                help="Disable --fast testing of integration tests")
    return parser.parse_args()


def main():
    args = get_args()

    # Setup
    global DEFAULT_THREADS_TO_USE, disable_fast
    os.environ["PATH"] = LPYTHON_PATH + os.pathsep + os.environ["PATH"]
    # delete previously created directories (if any)
    for backend in SUPPORTED_BACKENDS:
        run_cmd(f"rm -rf {BASE_DIR}/_lpython-tmp-test-{backend}")

    DEFAULT_THREADS_TO_USE = args.no_of_threads or DEFAULT_THREADS_TO_USE
    disable_fast = "yes" if args.no_fast else "no"
    for backend in args.backends:
        test_backend(backend)


if __name__ == "__main__":
    main()
