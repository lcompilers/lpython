# This test handles various aspects of local arrays using the `numpy.empty()`
# function
from ltypes import f64
from numpy import empty

def test_local_arrays():
    a: f64[16]
    a = empty(16)
    i: i32
    for i in range(16):
        a[i] = i+0.5
    eps: f64
    eps = 1e-12
    print( a[0], (a[0] - 0.5) < eps and (a[0] - 0.5) > -eps )
    print( a[1], (a[1] - 1.5) < eps and (a[1] - 1.5) > -eps )
    print( a[10], (a[10] - 10.5) < eps and (a[10] - 10.5) > -eps )
    print( a[15], (a[15] - 15.5) < eps and (a[15] - 15.5) > -eps )

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
    print( a[0], (a[0] - 0) < eps and (a[0] - 0) > -eps )
    print( a[1], (a[1] - 1) < eps and (a[1] - 1) > -eps )
    print( a[2], (a[2] - 2) < eps and (a[2] - 2) > -eps )
    print( a[3], (a[3] - 3) < eps and (a[3] - 3) > -eps )

def check():
    test_local_arrays()
    test_return_arrays()

check()
