from lpython import S
from sympy import pi, Symbol

def func1() -> S:
    return pi

def test_func1():
    z: S = func1()
    print(z)
    assert z == pi

def func2(r: Out[S]) -> None:
    r = pi

def test_func2():
    z: S
    func2(z)
    print(z)
    assert z == pi

test_func1()
test_func2()