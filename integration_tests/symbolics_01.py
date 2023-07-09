from sympy import Symbol, pi
from lpython import S

def main0():
    x: S = Symbol('x')
    y: S = Symbol('y')
    x = pi
    z: S = x + y
    print(z)
    assert(z == pi + y)
    assert(z != S(2)*pi + y)

main0()