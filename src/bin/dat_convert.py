#!/usr/bin/env python3

from struct import unpack
from sys import argv
from re import sub

lines = ""
with open(argv[1], "rb") as f:
    lines = f.read()

list = []
for i in range(0, len(lines), 24):
    list.append(sub('[(),]', '', str(unpack("3Q", lines[i:i+24]))))

with open(argv[1] + ".txt", "w") as f:
    j = 0
    for i in list:
        f.write(i+'\n')
