from math import (factorial, isqrt, perm, comb, degrees, radians, exp, pow,
                  ldexp, fabs, gcd, lcm, floor, ceil, remainder, expm1, fmod, log1p, trunc)
from ltypes import i32,f32, f64, i64

eps: f64
eps = 1e-12

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

    # Check for integer
    i = degrees(32)
    assert abs(i - 1833.4649444186343) < eps
    
    # Check for float
    i = degrees(32.2)
    assert abs(i - 1844.924100321251) < eps
  
def test_radians():
    i: f64

    # Check for integer
    i = radians(100)
    assert abs(i - 1.7453292519943295) < eps

    # Check for float
    i = radians(100.1)
    assert abs(i - 1.7470745812463238) < eps


def test_exp():
    i: f64
    i = exp(2.34)
    assert abs(i - 10.381236562731843) < eps


def test_pow():

    # i: f64
    assert abs(pow(2.4, 4.3) - 43.14280115650323) < eps
    assert abs(pow(2, 4) - 16) < eps

def test_ldexp():
    i: f64
    i = ldexp(23.3, 2)
    assert abs(i - 93.2) < eps


def test_fabs():

    eps: f64
    eps = 1e-12
    i: f64
    j: f64
    i = -10.5
    j = -7.0
    assert fabs(i + fabs(j)) == 3.5
    assert abs(fabs(10) - fabs(-10)) < eps
    assert abs(fabs(10.3) - fabs(-10.3)) < eps
    assert fabs(0.5) == 0.5
    assert fabs(-5) == 5.0


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


def test_floor():
    i: i64
    i = floor(10.02)
    assert i == 10
    i = floor(-13)
    assert i == -13
    i = floor(-13.31)
    assert i == -14


def test_ceil():
    i: i64
    i = ceil(10.02)
    assert i == 11
    i = ceil(-13)
    assert i == -13
    i = ceil(-13.31)
    assert i == -13


def test_remainder():
    assert remainder(9.0, 3.0) == 0.0
    assert remainder(12.0, 5.0) == 2.0
    assert remainder(13.0, 5.0) == -2.0

def test_fmod():
    assert fmod(20.5, 2.5) == 0.5
    assert fmod(-20.5, 2.5) == -0.5


def test_log1p():
    assert log1p(1.0) - 0.69314718055994529 < eps


def test_expm1():
    assert expm1(1.0) - 1.71828182845904509 < eps


def test_trunc():
    i: i64
    i = trunc(3.5)
    assert i == 3
    i = trunc(-4.5)
    assert i == -4
    i = trunc(5.5)
    assert i == 5
    i = trunc(-4.5)
    assert i == -4


def check():
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
    test_floor()
    test_ceil()
    test_remainder()
    test_fmod()
    test_expm1()
    test_log1p()
    test_trunc()


check()
