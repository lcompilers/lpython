# This test handles actual LPython implementations of functions from the numpy
# module.
from ltypes import f32
from numpy import zeros

def test_zeros():
    a: f32[4]
    a = zeros(4)
    eps: f64
    eps = 1e-12
    assert abs(a[0] - 0.0) < eps
    assert abs(a[1] - 0.0) < eps
    assert abs(a[2] - 0.0) < eps
    assert abs(a[3] - 0.0) < eps

def check():
    test_zeros()

check()
