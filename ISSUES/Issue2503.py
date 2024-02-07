from lpython import (i16, i32, Const)
from numpy import empty, int16
dim: Const[i32] = 10


def foo():
    """Negative indices produce random results each run."""
    A: i16[dim] = empty((dim,), dtype=int16)
    ww: i32
    for ww in range(dim):
        A[ww] = i16(ww + 1)
    print(A[0], A[1], A[2], "...", A[-3], A[-2], A[-1])


def bar(dim_: i32):
    """Negative indices always produce zero when 'dim' is a parameter."""
    A: i16[dim_] = empty((dim_,), dtype=int16)
    ww: i32
    for ww in range(dim_):
        A[ww] = i16(ww + 1)
    print(A[0], A[1], A[2], "...", A[-3], A[-2], A[-1])


foo()
bar(10)