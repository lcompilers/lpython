#!/usr/bin/env python

import sys

filename = sys.argv[1]

file = open(filename, "r")
f = file.read()
file.close()

file = open(filename, "w")
file.write(f)
file.close()
