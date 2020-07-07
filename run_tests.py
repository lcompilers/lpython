#!/usr/bin/env python

import argparse
import hashlib
import os
import subprocess

import toml

from compiler_tester.tester import (RunException, run, run_test, color,
    style, print_check, fg)

def main():
    parser = argparse.ArgumentParser(description="LFortran Test Suite")
    parser.add_argument("-u", "--update", action="store_true",
            help="update all reference results")
    parser.add_argument("-l", "--list", action="store_true",
            help="list all tests")
    parser.add_argument("-t", metavar="TEST",
            help="Run a specific test")
    parser.add_argument("-v", "--verbose", action="store_true",
            help="increase test verbosity")
    parser.add_argument("--no-llvm", action="store_true",
            help="Skip LLVM tests")
    args = parser.parse_args()
    update_reference = args.update
    list_tests = args.list
    specific_test = args.t
    verbose = args.verbose
    no_llvm = args.no_llvm

    # So that the tests find the `lfortran` executable
    os.environ["PATH"] = os.path.join(os.getcwd(), "src", "bin") \
            + os.pathsep + os.environ["PATH"]

    d = toml.load(open("tests/tests.toml"))
    for test in d["test"]:
        filename = test["filename"]
        if specific_test and filename != specific_test:
            continue
        tokens = test.get("tokens", False)
        ast = test.get("ast", False)
        ast_f90 = test.get("ast_f90", False)
        asr = test.get("asr", False)
        llvm = test.get("llvm", False)
        bin_ = test.get("bin", False)

        print(color(style.bold)+"TEST:"+color(style.reset), filename)

        if tokens:
            run_test("tokens", "lfortran --show-tokens {infile} -o {outfile}",
                    filename, update_reference)

        if ast:
            run_test("ast", "lfortran --show-ast {infile} -o {outfile}",
                    filename, update_reference)

        if ast_f90:
            run_test("ast_f90", "cpptranslate --show-ast-f90 {infile}",
                    filename, update_reference)

        if asr:
            run_test("asr", "lfortran --show-asr {infile} -o {outfile}",
                    filename, update_reference)

        if llvm:
            if no_llvm:
                print("    * llvm   SKIPPED as requested")
            else:
                run_test("llvm", "lfortran --show-llvm {infile} -o {outfile}",
                        filename, update_reference)

        if bin_:
            run_test("bin", "lfortran {infile} -o {outfile}",
                    filename, update_reference)


        print()

    if list_tests:
        return

    if update_reference:
        print("Reference tests updated.")
    else:
        print("%sTESTS PASSED%s" % (color(fg.green)+color(style.bold),
            color(fg.reset)+color(style.reset)))

if __name__ == "__main__":
    main()
