#!/usr/bin/env python

import os
import sys

tests = [
    "expr_01.py",
    "expr_02.py",
    "expr_03.py",
    "expr_04.py",
    "expr_05.py",
    "modules_01.py",
    #"modules_02.py",
    "test_math.py",
]

def main():
    print("Compiling...")
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
    print("Running...")
    for pyfile in tests:
        basename = os.path.splitext(pyfile)[0]
        cmd = "integration_tests/%s" % (basename)
        print("+ " + cmd)
        r = os.system(cmd)
        if r != 0:
            print("Command '%s' failed." % cmd)
            sys.exit(1)
        cmd = "python integration_tests/%s" % (pyfile)
        print("+ " + cmd)
        r = os.system(cmd)
        if r != 0:
            print("Command '%s' failed." % cmd)
            sys.exit(1)

if __name__ == "__main__":
    main()
