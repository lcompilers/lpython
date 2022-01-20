#!/usr/bin/env python

import os
import sys

tests = [
    "expr_01.py",
    "expr_02.py",
]

def main():
    print("Compiling...")
    for pyfile in tests:
        basename = os.path.splitext(pyfile)[0]
        cmd = "src/bin/lpython integration_tests/%s -o integration_tests/%s" % (pyfile, basename)
        print("+ " + cmd)
        r = os.system(cmd)
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
