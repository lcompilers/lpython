from math import factorial, isqrt, perm, comb, degrees, radians
from ltypes import i32, f64

def test_factorial_1():
    i: i32
    i = factorial(10)
    print(i)


def test_comb():
    i: i32
    i = comb(10, 2)
    print(i)


def test_perm():
    i: i32
    i = perm(5, 2)


def test_isqrt():
    i: i32
    i = isqrt(15)
    print(i)


def test_degrees():
    i: f64
    i = degrees(32.2)
    print(i)


def test_radians():
    i: f64
    i = degrees(100.1)
    print(i)


test_factorial_1()
test_comb()
test_isqrt()
test_perm()
test_degrees()
test_radians()
