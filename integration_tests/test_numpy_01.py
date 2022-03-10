# This test handles various aspects of local arrays using the `numpy.empty()`
# function
from ltypes import f32
from numpy import empty

def test_local_arrays():
    a: f32[16]
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

def check():
    test_local_arrays()

check()
