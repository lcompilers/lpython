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
    "modules_01.py",
    #"modules_02.py",
    "test_math.py",
    #"test_builtin.py",
    "test_builtin_abs.py",
    "test_builtin_bool.py",
    "test_builtin_pow.py",
    "test_builtin_int.py",
    "test_builtin_float.py",
    "test_math1.py"
]

# CPython tests only
test_cpython = [
    "test_generics_01.py",
    "test_c_interop_01.py"
]


def main():
    print("Compiling LPython tests...")
    for pyfile in tests:
        basename = os.path.splitext(pyfile)[0]
        cmd = os.path.join("..", "src", "bin", "lpython") + " %s -o %s" % (pyfile, basename)
        print("+ " + cmd)
        os.chdir("integration_tests")
        r = os.system(cmd)
        os.chdir("..")
        if r != 0:
            print("Command '%s' failed." % cmd)
            sys.exit(1)

    print("Running LPython and CPython tests...")
    python_path="src/runtime/ltypes"
    for pyfile in tests:
        basename = os.path.splitext(pyfile)[0]
        cmd = "integration_tests/%s" % (basename)
        print("+ " + cmd)
        r = os.system(cmd)
        if r != 0:
            print("Command '%s' failed." % cmd)
            sys.exit(1)
        cmd = "PYTHONPATH=%s python integration_tests/%s" % (python_path,
                    pyfile)
        print("+ " + cmd)
        r = os.system(cmd)
        if r != 0:
            print("Command '%s' failed." % cmd)
            sys.exit(1)

    print("Running CPython tests...")
    for pyfile in test_cpython:
        cmd = "PYTHONPATH=%s python integration_tests/%s" % (python_path,
                    pyfile)
        print("+ " + cmd)
        r = os.system(cmd)
        if r != 0:
            print("Command '%s' failed." % cmd)
            sys.exit(1)

if __name__ == "__main__":
    main()
