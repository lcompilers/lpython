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
        pysrc = open("integration_tests/%s" % pyfile).read()
        pysrc = """\
i32 = None
i64 = None
f32 = None
f64 = None
""" + pysrc
        open("integration_tests/py_%s" % pyfile, "w").write(pysrc)

        cmd = "python integration_tests/py_%s" % (pyfile)
        print("+ " + cmd)
        r = os.system(cmd)
        if r != 0:
            print("Command '%s' failed." % cmd)
            sys.exit(1)

if __name__ == "__main__":
    main()
