import argparse
from concurrent.futures import ThreadPoolExecutor
from functools import partial
import hashlib
import itertools
import json
import logging
import os
import re
import pathlib
import pprint
import shutil
import subprocess
import sys
import toml
from typing import Any, Mapping, List, Union

level = logging.DEBUG
log = logging.getLogger(__name__)
handler = logging.StreamHandler(sys.stdout)
handler.setFormatter(logging.Formatter('%(message)s'))
handler.setLevel(level)
log.addHandler(handler)
log.setLevel(level)


TESTER_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__)))
LIBASR_DIR = os.path.dirname(TESTER_DIR)
SRC_DIR = os.path.dirname(LIBASR_DIR)
ROOT_DIR = os.path.dirname(SRC_DIR)

no_color = False

class RunException(Exception):
    pass


class ExecuteException(Exception):
    pass


class style:
    reset = 0
    bold = 1
    dim = 2
    italic = 3
    underline = 4
    blink = 5
    rblink = 6
    reversed = 7
    conceal = 8
    crossed = 9


class fg:
    black = 30
    red = 31
    green = 32
    yellow = 33
    blue = 34
    magenta = 35
    cyan = 36
    gray = 37
    reset = 39


def color(value):
    return "\033[" + str(int(value)) + "m"


def check():
    return f"{(color(fg.green)+color(style.bold))}✓ {color(fg.reset)+color(style.reset)}"


def bname(base, cmd, filename):
    hstring = cmd
    if filename:
        hstring += filename
    h = hashlib.sha224(hstring.encode()).hexdigest()[:7]
    if filename:
        bname = os.path.basename(filename)
        bname, _ = os.path.splitext(bname)
        return f"{base}-{bname}-{h}"
    else:
        return f"{base}-{h}"


def _compare_eq_dict(
    left: Mapping[Any, Any], right: Mapping[Any, Any], verbose: int = 0
) -> List[str]:
    explanation: List[str] = []
    set_left = set(left)
    set_right = set(right)
    common = set_left.intersection(set_right)
    same = {k: left[k] for k in common if left[k] == right[k]}
    if same and verbose < 2:
        explanation += ["Omitting %s identical items" % len(same)]
    elif same:
        explanation += ["Common items:"]
        explanation += pprint.pformat(same).splitlines()
    diff = {k for k in common if left[k] != right[k]}
    if diff:
        explanation += ["Differing items:"]
        for k in diff:
            explanation += [repr({k: left[k]}) + " != " + repr({k: right[k]})]
    extra_left = set_left - set_right
    len_extra_left = len(extra_left)
    if len_extra_left:
        explanation.append(
            "Left contains %d more item%s:"
            % (len_extra_left, "" if len_extra_left == 1 else "s")
        )
        explanation.extend(
            pprint.pformat({k: left[k] for k in extra_left}).splitlines()
        )
    extra_right = set_right - set_left
    len_extra_right = len(extra_right)
    if len_extra_right:
        explanation.append(
            "Right contains %d more item%s:"
            % (len_extra_right, "" if len_extra_right == 1 else "s")
        )
        explanation.extend(
            pprint.pformat({k: right[k] for k in extra_right}).splitlines()
        )
    return explanation


def fixdir(s: bytes) -> bytes:
    local_dir = os.getcwd()
    return s.replace(local_dir.encode(), "$DIR".encode())


def unl_loop_del(b):
    return b.replace(bytes('\r\n', encoding='utf-8'),
                     bytes('\n', encoding='utf-8'))


def run(basename: str, cmd: Union[pathlib.Path, str],
        out_dir: Union[pathlib.Path, str], infile=None, extra_args=None):
    """
    Runs the `cmd` and collects stdout, stderr, exit code.

    The stdout, stderr and outfile are saved in the `out_dir` directory and
    all metadata is saved in a json file, whose path is returned from the
    function.

    The idea is to use this function to test the compiler by running it with
    an option to save the AST, ASR or LLVM IR or binary, and then ensure that
    the output does not change.

    Arguments:

    basename ... name of the run
    cmd ........ command to run, can use {infile} and {outfile}
    out_dir .... output directory to store output
    infile ..... optional input file. If present, it will check that it exists
                 and hash it.
    extra_args . extra arguments, not part of the hash

    Examples:

    >>> run("cat2", "cat tests/cat.txt > {outfile}", "output", "tests/cat.txt")
    >>> run("ls4", "ls --wrong-option", "output")

    """
    assert basename is not None and basename != ""
    pathlib.Path(out_dir).mkdir(parents=True, exist_ok=True)
    if infile and not os.path.exists(infile):
        raise RunException("The input file %s does not exist" % (infile))
    outfile = os.path.join(out_dir, basename + "." + "out")

    infile = infile.replace("\\\\", "\\").replace("\\", "/")

    cmd2 = cmd.format(infile=infile, outfile=outfile)
    if extra_args:
        cmd2 += " " + extra_args
    r = subprocess.run(cmd2, shell=True,
                       stdout=subprocess.PIPE,
                       stderr=subprocess.PIPE)
    if not os.path.exists(outfile):
        outfile = None
    if len(r.stdout):
        stdout_file = os.path.join(out_dir, basename + "." + "stdout")
        open(stdout_file, "wb").write(fixdir(r.stdout))
    else:
        stdout_file = None
    if len(r.stderr):
        stderr_file = os.path.join(out_dir, basename + "." + "stderr")
        open(stderr_file, "wb").write(fixdir(r.stderr))
    else:
        stderr_file = None

    if infile:
        temp = unl_loop_del(open(infile, "rb").read())
        infile_hash = hashlib.sha224(temp).hexdigest()
    else:
        infile_hash = None
    if outfile:
        temp = unl_loop_del(open(outfile, "rb").read())
        outfile_hash = hashlib.sha224(temp).hexdigest()
        outfile = os.path.basename(outfile)
    else:
        outfile_hash = None
    if stdout_file:
        temp = unl_loop_del(open(stdout_file, "rb").read())
        stdout_hash = hashlib.sha224(temp).hexdigest()
        stdout_file = os.path.basename(stdout_file)
    else:
        stdout_hash = None
    if stderr_file:
        temp = unl_loop_del(open(stderr_file, "rb").read())
        stderr_hash = hashlib.sha224(temp).hexdigest()
        stderr_file = os.path.basename(stderr_file)
    else:
        stderr_hash = None
    data = {
        "basename": basename,
        "cmd": cmd,
        "infile": infile,
        "infile_hash": infile_hash,
        "outfile": outfile,
        "outfile_hash": outfile_hash,
        "stdout": stdout_file,
        "stdout_hash": stdout_hash,
        "stderr": stderr_file,
        "stderr_hash": stderr_hash,
        "returncode": r.returncode,
    }
    json_file = os.path.join(out_dir, basename + "." + "json")
    json.dump(data, open(json_file, "w"), indent=4)
    return json_file


def get_error_diff(reference_file, output_file, full_err_str) -> str:
    diff_list = subprocess.Popen(
        f"diff {reference_file} {output_file}",
        stdout=subprocess.PIPE,
        shell=True,
        encoding='utf-8')
    diff_str = ""
    diffs = diff_list.stdout.readlines()
    for d in diffs:
        diff_str += d
    full_err_str += f"\nDiff against: {reference_file}\n"
    full_err_str += diff_str
    return full_err_str


def do_update_reference(jo, jr, do):
    shutil.copyfile(jo, jr)
    for f in ["outfile", "stdout", "stderr"]:
        if do[f]:
            f_o = os.path.join(os.path.dirname(jo), do[f])
            f_r = os.path.join(os.path.dirname(jr), do[f])
            shutil.copyfile(f_o, f_r)


def run_test(testname, basename, cmd, infile, update_reference=False,
             extra_args=None):
    """
    Runs the test `cmd` and compare against reference results.

    The `cmd` is executed via `run` (passing in `basename` and `infile`) and
    the output is saved in the `output` directory. The generated json file is
    then compared against reference results and if it differs, the
    RunException is thrown.

    Arguments:

    basename ........... name of the run
    cmd ................ command to run, can use {infile} and {outfile}
    infile ............. optional input file. If present, it will check that
                         it exists and hash it.
    update_reference ... if True, it will copy the output into the reference
                         directory as reference results, overwriting old ones
    extra_args ......... Extra arguments to append to the command that are not
                         part of the hash

    Examples:

    >>> run_test("cat12", "cat {infile} > {outfile}", "cat.txt",
    ...     update_reference=True)
    >>> run_test("cat12", "cat {infile} > {outfile}", "cat.txt")
    """
    s = f"{testname} * {basename}"
    basename = bname(basename, cmd, infile)
    infile = os.path.join("tests", infile)
    jo = run(basename, cmd, os.path.join("tests", "output"), infile=infile,
             extra_args=extra_args)
    jr = os.path.join("tests", "reference", os.path.basename(jo))
    if not os.path.exists(jo):
        raise FileNotFoundError(
            f"The output json file '{jo}' for {testname} does not exist")

    do = json.load(open(jo))
    if update_reference:
        do_update_reference(jo, jr, do)
        return

    if not os.path.exists(jr):
        raise FileNotFoundError(
            f"The reference json file '{jr}' for {testname} does not exist")

    dr = json.load(open(jr))
    if do != dr:
        # This string builds up the error message. Print test name in red in the beginning.
        # More information is added afterwards.
        full_err_str = f"\n{(color(fg.red)+color(style.bold))}{s}{color(fg.reset)+color(style.reset)}\n"
        e = _compare_eq_dict(do, dr)
        full_err_str += "The JSON metadata differs against reference results\n"
        full_err_str += "Reference JSON: " + jr + "\n"
        full_err_str += "Output JSON:    " + jo + "\n"
        full_err_str += "\n".join(e)

        for field in ["outfile", "stdout", "stderr"]:
            hash_field = field + "_hash"
            if not do[hash_field] and dr[hash_field]:
                full_err_str += f"No output {hash_field} available for {testname}\n"
                break
            if not dr[hash_field] and do[hash_field]:
                full_err_str += f"No reference {hash_field} available for {testname}\n"
                break
            if do[hash_field] != dr[hash_field]:
                output_file = os.path.join("tests", "output", do[field])
                reference_file = os.path.join("tests", "reference", dr[field])
                full_err_str = get_error_diff(
                    reference_file, output_file, full_err_str)
                break
        raise RunException(
            "Testing with reference output failed." +
            full_err_str)
    if no_color:
        log.debug(s + " PASS")
    else:
        log.debug(s + " " + check())


def tester_main(compiler, single_test):
    parser = argparse.ArgumentParser(description=f"{compiler} Test Suite")
    parser.add_argument("-u", "--update", action="store_true",
                        help="update all reference results")
    parser.add_argument("-l", "--list", action="store_true",
                        help="list all tests")
    parser.add_argument("-t", "--test",
                        action="append", nargs="*",
                        help="Run specific tests")
    parser.add_argument("-b", "--backend",
                        action="append", nargs="*",
                        help="Run specific backends")
    parser.add_argument("-v", "--verbose", action="store_true",
                        help="increase test verbosity")
    parser.add_argument("--exclude-test", metavar="TEST",
                        action="append", nargs="*",
                        help="Exclude specific tests"),
    parser.add_argument("--exclude-backend", metavar="BACKEND",
                        action="append", nargs="*",
                        help="Exclude specific backends, only works when -b is not specified"),
    parser.add_argument("--no-llvm", action="store_true",
                        help="Skip LLVM tests")
    parser.add_argument("--skip-run-with-dbg", action="store_true",
                        help="Skip runtime tests with debugging information enabled")
    parser.add_argument("-s", "--sequential", action="store_true",
                        help="Run all tests sequentially")
    parser.add_argument("--no-color", action="store_true",
                    help="Turn off colored tests output")
    args = parser.parse_args()
    update_reference = args.update
    list_tests = args.list
    specific_tests = list(
        itertools.chain.from_iterable(
            args.test)) if args.test else None
    specific_backends = set(
        itertools.chain.from_iterable(
            args.backend)) if args.backend else None
    excluded_tests = list(itertools.chain.from_iterable(
        args.exclude_test)) if args.exclude_test else None
    excluded_backends = set(itertools.chain.from_iterable(
        args.exclude_backend)) if args.exclude_backend and specific_backends is None else None
    verbose = args.verbose
    no_llvm = args.no_llvm
    skip_run_with_dbg = args.skip_run_with_dbg
    global no_color
    no_color = args.no_color

    # So that the tests find the `lcompiler` executable
    os.environ["PATH"] = os.path.join(SRC_DIR, "bin") \
        + os.pathsep + os.environ["PATH"]
    test_data = toml.load(open(os.path.join(ROOT_DIR, "tests", "tests.toml")))
    filtered_tests = test_data["test"]
    if specific_tests:
        filtered_tests = [test for test in filtered_tests if any(
            re.search(t, test["filename"]) for t in specific_tests)]
    if excluded_tests:
        filtered_tests = [test for test in filtered_tests if not any(
            re.search(t, test["filename"]) for t in excluded_tests)]
    if specific_backends:
        filtered_tests = [
            test for test in filtered_tests if any(
                b in test for b in specific_backends)]
    if excluded_backends:
        filtered_tests = [test for test in filtered_tests if any(
            b not in excluded_backends and b != "filename" for b in test)]

    for test in filtered_tests:
        if 'extrafiles' in test:
            single_test(test,
                update_reference=update_reference,
                specific_backends=specific_backends,
                excluded_backends=excluded_backends,
                verbose=verbose,
                no_llvm=no_llvm,
                skip_run_with_dbg=True,
                no_color=True)
    filtered_tests = [test for test in filtered_tests if 'extrafiles' not in test]

    if args.sequential:
        for test in filtered_tests:
            single_test(test,
                        update_reference=update_reference,
                        specific_backends=specific_backends,
                        excluded_backends=excluded_backends,
                        verbose=verbose,
                        no_llvm=no_llvm,
                        skip_run_with_dbg=skip_run_with_dbg,
                        no_color=no_color)
    # run in parallel
    else:
        single_tester_partial_args = partial(
            single_test,
            update_reference=update_reference,
            specific_backends=specific_backends,
            excluded_backends=excluded_backends,
            verbose=verbose,
            no_llvm=no_llvm,
            skip_run_with_dbg=skip_run_with_dbg,
            no_color=no_color)
        with ThreadPoolExecutor() as ex:
            futures = ex.map(single_tester_partial_args, filtered_tests)
            for f in futures:
                if not f:
                    ex.shutdown(wait=False)
    if list_tests:
        return

    if update_reference:
        log.info("Test references updated.")
    else:
        if no_color:
            log.info("TESTS PASSED")
        else:
            log.info(
                f"{(color(fg.green) + color(style.bold))}TESTS PASSED"
                f"{color(fg.reset) + color(style.reset)}")
