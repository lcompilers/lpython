#!/usr/bin/env python

import sys
sys.path.append("src/libasr")
from compiler_tester.tester import color, fg, log, run_test, style, tester_main


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



if __name__ == "__main__":
    tester_main("LPython", single_test)
