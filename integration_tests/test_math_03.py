from math import (cbrt, exp2)
from ltypes import f64

eps: f64
eps = 1e-12

def test_exp2():
    i: f64
    i = exp2(4.3)
    assert abs(i - 19.698310613518657) < eps

def test_cbrt():
    eps: f64 = 1e-12
    assert abs(cbrt(124.0) - 4.986630952238646) < eps
    assert abs(cbrt(39.0) - 3.3912114430141664) < eps

def check():
    test_cbrt()
    test_exp2()

check()