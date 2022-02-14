from math import (factorial, isqrt, perm, comb, degrees, radians, exp, pow,
                  ldexp, fabs)
from ltypes import i32, f64


def test_factorial_1():
    i: i32
    i = factorial(10)
    assert i == 3628800


def test_comb():
    i: i32
    i = comb(10, 2)
    assert i == 45


def test_perm():
    i: i32
    i = perm(5, 2)
    assert i == 20


def test_isqrt():
    i: i32
    i = isqrt(15)
    assert i == 3


def test_degrees():
    i: f64
    i = degrees(32.2)
    assert i == 1844.924100321251


def test_radians():
    i: f64
    i = radians(100.1)
    assert i == 1.7470745812463238


def test_exp():
    i: f64
    i = exp(2.34)
    assert i == 10.381236562731843


def test_pow():
    i: f64
    i = pow(2.4, 4.3)
    assert i == 43.14280115650323


def test_ldexp():
    i: f64
    i = ldexp(23.3, 2)
    assert i == 93.2


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
