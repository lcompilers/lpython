#!/usr/bin/env python

import argparse
import os

import toml

from compiler_tester.tester import run_test, color, style, fg

def main():
    parser = argparse.ArgumentParser(description="LPython Test Suite")
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

    # So that the tests find the `lpython` executable
    os.environ["PATH"] = os.path.join(os.getcwd(), "src", "bin") \
            + os.pathsep + os.environ["PATH"]

    d = toml.load(open("tests/tests.toml"))
    for test in d["test"]:
        filename = test["filename"]
        if specific_test and filename != specific_test:
            continue
        tokens = test.get("tokens", False)
        ast = test.get("ast", False)
        ast_indent = test.get("ast_indent", False)
        ast_f90 = test.get("ast_f90", False)
        ast_cpp = test.get("ast_cpp", False)
        ast_cpp_hip = test.get("ast_cpp_hip", False)
        ast_openmp = test.get("ast_openmp", False)
        ast_new = test.get("ast_new", False)
        asr = test.get("asr", False)
        asr_preprocess = test.get("asr_preprocess", False)
        asr_indent = test.get("asr_indent", False)
        mod_to_asr = test.get("mod_to_asr", False)
        llvm = test.get("llvm", False)
        cpp = test.get("cpp", False)
        c = test.get("c", False)
        obj = test.get("obj", False)
        x86 = test.get("x86", False)
        bin_ = test.get("bin", False)
        pass_ = test.get("pass", None)
        if pass_ and pass_ not in ["do_loops", "global_stmts",
                                   "loop_vectorise", "inline_function_calls"]:
            raise Exception("Unknown pass: %s" % pass_)

        print(color(style.bold)+"TEST:"+color(style.reset), filename)

        extra_args = "--no-error-banner"

        if ast:
            run_test("ast", "lpython --show-ast --no-color {infile} -o {outfile}",
                        filename, update_reference, extra_args)

        if ast_new:
            run_test("ast_new", "lpython --show-ast --new-parser --no-color {infile} -o {outfile}",
                        filename, update_reference, extra_args)

        if asr:
            run_test("asr", "lpython --show-asr --no-color {infile} -o {outfile}",
                    filename, update_reference, extra_args)

        if pass_ is not None:
            cmd = "lpython --pass=" + pass_ + " --show-asr --no-color {infile} -o {outfile}"
            run_test("pass_{}".format(pass_), cmd,
                     filename, update_reference, extra_args)
            if llvm:
                cmd = "lpython --pass=" + pass_ + " --show-llvm --no-color {infile} -o {outfile}"
                run_test("pass_llvm_{}".format(pass_), cmd,
                        filename, update_reference, extra_args)

        if llvm:
            if no_llvm:
                print("    * llvm   SKIPPED as requested")
            else:
                run_test("llvm", "lpython --no-color --show-llvm {infile} -o {outfile}",
                        filename, update_reference, extra_args)

        if cpp:
            run_test("cpp", "lpython --no-color --show-cpp {infile}",
                    filename, update_reference, extra_args)

        if c:
            run_test("c", "lpython --no-color --show-c {infile}",
                    filename, update_reference, extra_args)

        if tokens:
            run_test("tokens", "lpython --no-color --show-tokens {infile} -o {outfile}",
                    filename, update_reference, extra_args)

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
