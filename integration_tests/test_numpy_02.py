# This test handles actual LPython implementations of functions from the numpy
# module.
from ltypes import i32, f64, TypeVar
from numpy import empty

n: i32
n = TypeVar("n")
def zeros(n: i32) -> f64[n]:
    A: f64[n]
    A = empty(n)
    i: i32
    for i in range(n):
        A[i] = 0.0
    return A

def arange(n: i32) -> f64[n]:
    A: f64[n]
    A = empty(n)
    i: i32
    for i in range(n):
        A[i] = 1.0 * i
    return A

def test_zeros():
    a: f64[4]
    a = zeros(4)
    eps: f64
    eps = 1e-12
    assert abs(a[0] - 0.0) < eps
    assert abs(a[1] - 0.0) < eps
    assert abs(a[2] - 0.0) < eps
    assert abs(a[3] - 0.0) < eps

def test_arange():
    a: f64[4]
    a = arange(4)
    eps: f64
    eps = 1e-12
    assert abs(a[0] - 0.0) < eps
    assert abs(a[1] - 1.0) < eps
    assert abs(a[2] - 2.0) < eps
    assert abs(a[3] - 3.0) < eps

def check():
    test_zeros()
    test_arange()

check()
