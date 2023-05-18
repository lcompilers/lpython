from lpython import Allocatable, f64, i32
from numpy import empty, float64, int32

def f():
    n: i32 = 5
    a: Allocatable[f64[:]] = empty((n,), dtype=float64)
    i: i32
    for i in range(n):
        a[i] = f64(i+1)
    b: Allocatable[i32[:]]
    n = 10
    b = empty((n,), dtype=int32)
    for i in range(n):
        b[i] = i+1
    print(a)
    for i in range(n):
        assert b[i] == i+1

f()
