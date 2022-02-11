from math import (factorial, isqrt, perm, comb, degrees, radians, exp, pow,
                  ldexp, fabs)
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


def test_exp():
    i: f64
    i = exp(2.34)
    print(i)


def test_pow():
    i: f64
    i = pow(2.4, 4.3)
    print(i)


def test_ldexp():
    i: f64
    i = ldexp(23.3, 2)
    print(i)


def test_fabs():
    i: f64
    j: f64
    i = fabs(10.3)
    j = fabs(-10.3)
    print(i, j)


test_factorial_1()
test_comb()
test_isqrt()
test_perm()
test_degrees()
test_radians()
test_exp()
test_pow()
test_fabs()
test_ldexp()
