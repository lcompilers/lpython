from sympy import Symbol, pi, S
from lpython import S, i32

def addInteger(x: S, y: S, z: S, i: i32):
    _i: S = S(i)
    print(x + y + z + _i)

def call_addInteger():
    a: S = Symbol("x")
    b: S = Symbol("y")
    c: S = pi
    addInteger(a, b, c, 2)

def main0():
    call_addInteger()

main0()
