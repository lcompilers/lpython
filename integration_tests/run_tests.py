#!/usr/bin/env python

import os
import sys

# LPython and CPython tests
tests = [
    "exit_01.py",
    "expr_01.py",
    "expr_02.py",
    "expr_03.py",
    "expr_04.py",
    "expr_05.py",
    "test_types_01.py",
    "test_str_01.py",
    "test_str_02.py",
    "test_list_01.py",
    "modules_01.py",
    #"modules_02.py",
    "test_math.py",
    "test_numpy_01.py",
    "test_numpy_02.py",
    "test_random.py",
    "test_os.py",
    "test_builtin.py",
    "test_builtin_abs.py",
    "test_builtin_bool.py",
    "test_builtin_pow.py",
    "test_builtin_int.py",
    "test_builtin_len.py",
    "test_builtin_float.py",
    "test_builtin_str_02.py",
    "test_builtin_round.py",
    # "test_builtin_hex.py",
    # "test_builtin_oct.py",
    # "test_builtin_str.py",
    "test_math1.py",
    "test_math_02.py",
    "test_c_interop_01.py",
    "test_generics_01.py",
    "test_cmath.py",
    "test_complex.py",
    "test_max_min.py",
    "test_integer_bitnot.py",
    "test_unary_minus.py",
    "test_unary_plus.py",
    "test_issue_518.py"

]

# CPython tests only
test_cpython = [
    "test_builtin_bin.py",
]

CUR_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__)))

def main():
    if not os.path.exists(os.path.join(CUR_DIR, 'tmp')):
        os.makedirs(os.path.join(CUR_DIR, 'tmp'))
    _print_msg("Compiling LPython tests...", 30)
    for pyfile in tests:
        basename = os.path.splitext(pyfile)[0]
        cmd = os.path.join("..", "src", "bin", "lpython") + " %s -o tmp/%s" % (pyfile, basename)
        msg = "Running: %s" % cmd
        _print_msg(msg, 33)
        os.chdir("integration_tests")
        r = os.system(cmd)
        os.chdir("..")
        if r != 0:
            msg = "Command '%s' failed." % cmd
            _print_msg(msg, 31)
            sys.exit(1)

    _print_msg("Running LPython and CPython tests...", 30)
    python_path="src/runtime/ltypes"
    for pyfile in tests:
        basename = os.path.splitext(pyfile)[0]
        cmd = "integration_tests/tmp/%s" % (basename)
        msg = "Running: %s" % cmd
        _print_msg(msg, 33)
        r = os.system(cmd)
        if r != 0:
            msg = "Command '%s' failed." % cmd
            _print_msg(msg, 31)
            sys.exit(1)
        cmd = "PYTHONPATH=%s python integration_tests/%s" % (python_path,
                    pyfile)
        msg = "Running: %s" % cmd
        _print_msg(msg, 33)
        r = os.system(cmd)
        if r != 0:
            msg = "Command '%s' failed." % cmd
            _print_msg(msg, 31)
            sys.exit(1)

    _print_msg("Running CPython tests...", 30)
    for pyfile in test_cpython:
        cmd = "PYTHONPATH=%s python integration_tests/%s" % (python_path,
                    pyfile)
        msg = "Running: %s" % cmd
        _print_msg(msg, 33)
        r = os.system(cmd)
        if r != 0:
            msg = "Command '%s' failed." % cmd
            _print_msg(msg, 31)
            sys.exit(1)
    _print_msg("ALL TESTS PASSED", 32)


def _color(value):
    return "\033[" + str(int(value)) + "m"

def _print_msg(string, color_int):
    print("%s%s%s" % (_color(color_int) + _color(1), string, _color(39) + _color(0)))

if __name__ == "__main__":
    main()
