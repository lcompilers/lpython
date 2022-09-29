# This test handles various aspects of local arrays using the `numpy.empty()`
# function
from ltypes import f64, i32
from numpy import empty

def test_local_arrays():
    a: f64[16]
    a = empty(16)
    i: i32
    for i in range(16):
        a[i] = i+0.5
    eps: f64
    eps = 1e-12
    assert abs(a[0] - 0.5) < eps
    assert abs(a[1] - 1.5) < eps
    assert abs(a[10] - 10.5) < eps
    assert abs(a[15] - 15.5) < eps

def f() -> f64[4]:
    a: f64[4]
    a = empty(4)
    i: i32
    for i in range(4):
        a[i] = 1.0 * i
    return a

def test_return_arrays():
    a: f64[4]
    a = f()
    eps: f64
    eps = 1e-12
    assert abs(a[0] - 0) < eps
    assert abs(a[1] - 1) < eps
    assert abs(a[2] - 2) < eps
    assert abs(a[3] - 3) < eps

def check():
    test_local_arrays()
    test_return_arrays()

check()
