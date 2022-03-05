from math import (factorial, isqrt, perm, comb, degrees, radians, exp, pow,
                  ldexp, fabs, gcd, lcm, copysign)
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


def test_gcd():
    i: i32
    i = gcd(10, 4)
    assert i == 2
    i = gcd(21, 14)
    assert i == 7
    i = gcd(21, -12)
    assert i == 3

def test_lcm():
    i: i32
    i = lcm(10, 4)
    assert i == 20
    i = lcm(21, 14)
    assert i == 42
    i = lcm(21, -12)
    assert i == 84


def test_copysign():
    f: f64
    f = copysign(-8.56, 97.21)
    assert f == 8.56
    f = copysign(-43.0, -76.67)
    assert f == -43.0


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
test_gcd()
test_lcm()
test_copysign()
