#!/usr/bin/env python3

from struct import unpack
import sys
import re

lines = ""

with open('lines.dat', "rb") as f:
    lines = f.read()

list = []

for i in range(0, len(lines), 24):
    list.append(re.sub('[(),]', '', str(unpack("3Q", lines[i:i+24]))))

with open("lines_dat.txt", "w") as f:
    j = 0
    for i in list:
        # print(str.encode(i))
        f.write(i+'\n')
