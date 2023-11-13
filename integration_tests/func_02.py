from lpython import S
from sympy import pi, Symbol

def func(r: Out[S]) -> None:
    r = pi

def test_func():
    z: S
    func(z)
    print(z)
    assert z == pi

test_func()
