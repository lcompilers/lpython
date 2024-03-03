from math import (cbrt, sqrt)
from lpython import f32, f64, i32, i64

eps: f64
eps = 1e-12

def test_cbrt():
    eps: f64 = 1e-12
    a : i32 = 64
    b : i64 = i64(64)
    c : f32 = f32(64.0)
    d : f64 = f64(64.0)
    assert abs(cbrt(124.0) - 4.986630952238646) < eps
    assert abs(cbrt(39.0) - 3.3912114430141664) < eps
    assert abs(cbrt(39) - 3.3912114430141664) < eps
    assert abs(cbrt(a) - 4.0) < eps
    assert abs(cbrt(b) - 4.0) < eps
    assert abs(cbrt(c) - 4.0) < eps
    assert abs(cbrt(d) - 4.0) < eps

def test_sqrt():
    eps: f64 = 1e-12
    a : i32 = 64
    b : i64 = i64(64)
    c : f32 = f32(64.0)
    d : f64 = f64(64.0)
    assert abs(sqrt(a) - 8.0) < eps
    assert abs(sqrt(b) - 8.0) < eps
    assert abs(sqrt(c) - 8.0) < eps
    assert abs(sqrt(d) - 8.0) < eps

def check():
    test_cbrt()
    test_sqrt()

check()