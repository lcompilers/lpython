from lpython import S
from sympy import pi, Symbol

def func() -> S:
    return pi

def test_func():
    z: S = func()
    print(z)
    assert z == pi

test_func()