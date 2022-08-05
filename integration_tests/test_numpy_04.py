from ltypes import i32, f64
from numpy import array

def test_array_01():
    x: f64[3] = array([1.0, 2.0, 3.0])
    eps: f64 = 1e-16
    assert abs(x[0] - 1.0) < eps
    assert abs(x[1] - 2.0) < eps
    assert abs(x[2] - 3.0) < eps

def test_array_02():
    x: i32[3] = array([1, 2, 3])
    eps: f64 = 1e-16
    assert abs(x[0] - 1) < eps
    assert abs(x[1] - 2) < eps
    assert abs(x[2] - 3) < eps

def check():
    test_array_01()
    test_array_02()

check()
