from lpython import S
from sympy import Symbol, pi

def test_main():
    x: S = Symbol('x')
    if x != pi:
        print(x != pi)
        assert x != pi

test_main()