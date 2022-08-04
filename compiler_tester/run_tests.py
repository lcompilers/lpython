#!/usr/bin/env python

import argparse
import os
import toml

from concurrent.futures import ThreadPoolExecutor
from functools import partial

from tester import color, fg, log, run_test, style


CUR_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__)))
ROOT_DIR = os.path.dirname(CUR_DIR)


def single_test(test, specific_test, verbose, no_llvm, update_reference):
    filename = test["filename"]
    if specific_test and specific_test not in filename:
        return
    show_verbose = "" if not verbose else "-v"
    tokens = test.get("tokens", False)
    ast = test.get("ast", False)
    ast_indent = test.get("ast_indent", False)
    ast_f90 = test.get("ast_f90", False)
    ast_cpp = test.get("ast_cpp", False)
    ast_cpp_hip = test.get("ast_cpp_hip", False)
    ast_openmp = test.get("ast_openmp", False)
    asr = test.get("asr", False)
    asr_implicit_typing = test.get("asr_implicit_typing", False)
    asr_preprocess = test.get("asr_preprocess", False)
    asr_indent = test.get("asr_indent", False)
    mod_to_asr = test.get("mod_to_asr", False)
    llvm = test.get("llvm", False)
    cpp = test.get("cpp", False)
    c = test.get("c", False)
    wat = test.get("wat", False)
    obj = test.get("obj", False)
    x86 = test.get("x86", False)
    bin_ = test.get("bin", False)
    pass_ = test.get("pass", None)
    optimization_passes = ["flip_sign", "div_to_mul", "fma", "sign_from_value",
                           "inline_function_calls", "loop_unroll",
                           "dead_code_removal", "loop_vectorise"]
    other_passes = ["do_loops", "global_stmts",
                    "loop_vectorise", "inline_function_calls"]
    if pass_ and (pass_ not in other_passes and
                  pass_ not in optimization_passes):
        raise Exception(f"Unknown pass: {pass_}")
    log.debug(f"{color(style.bold)} START TEST: {color(style.reset)} {filename}")

    extra_args = f"--no-error-banner {show_verbose}"

    compiler = "lfortran"
    if filename.endswith(".py"):
        compiler = "lpython"

    if tokens:
        run_test(
            filename,
            "tokens",
            compiler + " --no-color --show-tokens {infile} -o {outfile}",
            filename,
            update_reference,
            extra_args)

    if ast:
        if filename.endswith(".py"):
            run_test(
                filename,
                "ast",
                "lpython --show-ast --no-color {infile} -o {outfile}",
                filename,
                update_reference,
                extra_args)
        elif filename.endswith(".f"):
            # Use fixed form
            run_test(
                filename,
                "ast",
                "lfortran --fixed-form --show-ast --no-color {infile} -o {outfile}",
                filename,
                update_reference,
                extra_args)
        else:
            # Use free form
            run_test(
                filename,
                "ast",
                "lfortran --show-ast --no-color {infile} -o {outfile}",
                filename,
                update_reference,
                extra_args)
    if ast_indent:
        run_test(
            filename,
            "ast_indent",
            compiler + " --show-ast --indent --no-color {infile} -o {outfile}",
            filename,
            update_reference,
            extra_args)

    if ast_f90:
        if filename.endswith(".f"):
            # Use fixed form
            run_test(
                filename,
                "ast_f90",
                "lfortran --fixed-form --show-ast-f90 --no-color {infile}",
                filename,
                update_reference,
                extra_args)
        else:
            # Use free form
            run_test(
                filename,
                "ast_f90",
                "lfortran --show-ast-f90 --no-color {infile}",
                filename,
                update_reference,
                extra_args)

    if ast_openmp:
        run_test(
            filename,
            "ast_openmp",
            "cpptranslate --show-ast-openmp {infile}",
            filename,
            update_reference)

    if asr:
        run_test(
            filename,
            "asr",
            compiler + " --show-asr --no-color {infile} -o {outfile}",
            filename,
            update_reference,
            extra_args)

    if asr_implicit_typing:
        run_test(
            filename,
            "asr",
            "lfortran --show-asr --implicit-typing --no-color {infile} -o {outfile}",
            filename,
            update_reference,
            extra_args)

    if asr_preprocess:
        run_test(
            filename,
            "asr_preprocess",
            compiler + " --cpp --show-asr --no-color {infile} -o {outfile}",
            filename,
            update_reference,
            extra_args)

    if asr_indent:
        run_test(
            filename,
            "asr_indent",
            compiler + " --show-asr --indent --no-color {infile} -o {outfile}",
            filename,
            update_reference,
            extra_args)

    if mod_to_asr:
        run_test(
            filename,
            "mod_to_asr",
            "lfortran mod --show-asr --no-color {infile}",
            filename,
            update_reference)

    if pass_ is not None:
        cmd = compiler + " --pass=" + pass_ + \
            " --show-asr --no-color {infile} -o {outfile}"
        run_test(filename, "pass_{}".format(pass_), cmd,
                 filename, update_reference, extra_args)
    if llvm:
        if no_llvm:
            log.info(f"{filename} * llvm   SKIPPED as requested")
        else:
            run_test(
                filename,
                "llvm",
                compiler + " --no-color --show-llvm {infile} -o {outfile}",
                filename,
                update_reference,
                extra_args)

    if cpp:
        run_test(filename, "cpp",
                 compiler + " --no-color --show-cpp {infile}",
                 filename, update_reference, extra_args)

    if obj:
        if no_llvm:
            log.info(f"{filename} * obj    SKIPPED as requested")
        else:
            run_test(
                filename,
                "obj",
                compiler + " --no-color -c {infile} -o output.o",
                filename,
                update_reference,
                extra_args)

    if c:
        run_test(filename, "c",
                 compiler + " --no-color --show-c {infile}",
                 filename, update_reference, extra_args)

    if wat:
        run_test(filename, "wat",
                 compiler + " --no-color --show-wat {infile}",
                 filename, update_reference, extra_args)

    if x86:
        run_test(
            filename,
            "x86",
            compiler + " --no-color --backend=x86 {infile} -o output",
            filename,
            update_reference,
            extra_args)

    if bin_:
        run_test(filename, "bin",
                 compiler + " --no-color {infile} -o {outfile}",
                 filename, update_reference, extra_args)


def main():
    parser = argparse.ArgumentParser(description="LCompilers Test Suite")
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
    parser.add_argument("-s", "--sequential", action="store_true",
                        help="Run all tests sequentially")
    args = parser.parse_args()
    update_reference = args.update
    list_tests = args.list
    specific_test = args.t
    verbose = args.verbose
    no_llvm = args.no_llvm

    # So that the tests find the `lfortran/lpython` executable
    os.environ["PATH"] = os.path.join(ROOT_DIR, "src", "bin") \
        + os.pathsep + os.environ["PATH"]
    test_data = toml.load(open(os.path.join(ROOT_DIR, "tests", "tests.toml")))
    if specific_test:
        # some fuzzy comparison to get all seemingly fitting tests tested
        specific = [test for test in test_data["test"]
                    if specific_test in test["filename"]]
        # no concurrent execution
        for test in specific:
            single_test(test,
                        update_reference=update_reference,
                        specific_test=specific_test,
                        verbose=verbose,
                        no_llvm=no_llvm)
    elif args.sequential:
        for test in test_data["test"]:
            single_test(test,
                        update_reference=update_reference,
                        specific_test=specific_test,
                        verbose=verbose,
                        no_llvm=no_llvm)
    # run in parallel
    else:
        single_tester_partial_args = partial(
            single_test,
            update_reference=update_reference,
            specific_test=specific_test,
            verbose=verbose,
            no_llvm=no_llvm)
        with ThreadPoolExecutor() as ex:
            futures = ex.map(
                single_tester_partial_args, [
                    test for test in test_data["test"]])
            for f in futures:
                if not f:
                    ex.shutdown(wait=False)
    if list_tests:
        return

    if update_reference:
        log.info("Test references updated.")
    else:
        log.info(
            f"{(color(fg.green) + color(style.bold))}TESTS PASSED{color(fg.reset) + color(style.reset)}")


if __name__ == "__main__":
    main()
