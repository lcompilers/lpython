from lpython import S, Out
from sympy import pi

def func(r: Out[S]) -> None:
    r = pi

def test_func():
    z: S
    func(z)
    print(z)
    assert z == pi

test_func()
