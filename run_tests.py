#!/usr/bin/env python

import argparse
import os
import toml

from concurrent.futures import ThreadPoolExecutor
from functools import partial

import sys
sys.path.append("src/libasr")
from compiler_tester.tester import color, fg, log, run_test, style


def single_test(test, specific_test, verbose, no_llvm, update_reference):
    filename = test["filename"]
    if specific_test and specific_test not in filename:
        return
    show_verbose = "" if not verbose else "-v"
    tokens = test.get("tokens", False)
    ast = test.get("ast", False)
    asr = test.get("asr", False)
    llvm = test.get("llvm", False)
    cpp = test.get("cpp", False)
    c = test.get("c", False)
    pass_ = test.get("pass", None)
    optimization_passes = ["flip_sign", "div_to_mul", "fma", "sign_from_value",
                           "inline_function_calls", "loop_unroll",
                           "dead_code_removal", "loop_vectorise"]

    if pass_ and (pass_ not in ["do_loops", "global_stmts"] and
                  pass_ not in optimization_passes):
        raise Exception(f"Unknown pass: {pass_}")
    log.debug(f"{color(style.bold)} START TEST: {color(style.reset)} {filename}")

    extra_args = f"--no-error-banner {show_verbose}"

    if tokens:
        run_test(
            filename,
            "tokens",
            "lpython --no-color --show-tokens {infile} -o {outfile}",
            filename,
            update_reference,
            extra_args)

    if ast:
        run_test(
            filename,
            "ast",
            "lpython --show-ast --no-color {infile} -o {outfile}",
            filename,
            update_reference,
            extra_args)

    if asr:
        run_test(
            filename,
            "asr",
            "lpython --show-asr --no-color {infile} -o {outfile}",
            filename,
            update_reference,
            extra_args)

    if pass_ is not None:
        cmd = "lpython --pass=" + pass_ + \
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
                "lpython --no-color --show-llvm {infile} -o {outfile}",
                filename,
                update_reference,
                extra_args)

    if cpp:
        run_test(filename, "cpp", "lpython --no-color --show-cpp {infile}",
                 filename, update_reference, extra_args)

    if c:
        run_test(filename, "c", "lpython --no-color --show-c {infile}",
                 filename, update_reference, extra_args)


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
    parser.add_argument("-s", "--sequential", action="store_true",
                        help="Run all tests sequentially")
    args = parser.parse_args()
    update_reference = args.update
    list_tests = args.list
    specific_test = args.t
    verbose = args.verbose
    no_llvm = args.no_llvm

    # So that the tests find the `lpython` executable
    os.environ["PATH"] = os.path.join(os.getcwd(), "src", "bin") \
        + os.pathsep + os.environ["PATH"]
    test_data = toml.load(open("tests/tests.toml"))
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
