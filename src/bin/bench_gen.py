#!/usr/bin/env python

N = 10000

A_functions = ""
calls = ""
for i in range(N):
    func_A = f"""
def A{i}(x: i32) -> i32:
    y: i32
    z: i32
    y = {i}
    z = 5
    x = x + y * z
    return x
"""
    A_functions += func_A
    calls += f"    y = A{i}(y)\n"

source = f"""\
from lpython import i32

{A_functions}

def Driver(test_val: i32) -> i32:
    y: i32
    y = test_val
{calls}
    return y

def Main0():
    print(Driver(5))

Main0()
"""

print(source)
