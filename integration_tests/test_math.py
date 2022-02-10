from math import factorial, isqrt, perm, comb
from ltypes import i32

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


test_factorial_1()
test_comb()
test_isqrt()
test_perm()
