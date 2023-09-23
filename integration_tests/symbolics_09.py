from sympy import Symbol, pi, sin, cos
from lpython import S, i32

def addInteger(x: S, y: S, z: S, i: i32):
    _i: S = S(i)
    print(x + y + z + _i)

def call_addInteger():
    a: S = Symbol("x")
    b: S = Symbol("y")
    c: S = pi
    d: S = sin(a)
    e: S = cos(b)
    addInteger(c, d, e, 2)
    addInteger(c, sin(a), cos(b), 2)
    addInteger(c, sin(Symbol("x")), cos(Symbol("y")), 2)

def main0():
    call_addInteger()

main0()
