#!/usr/bin/env python

import sys

filename = sys.argv[1]

f = open(filename).read()

open(filename, "w").write(f)
