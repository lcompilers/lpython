from lpython import (i32, Const)
from numpy import empty, int32

dim: Const[i32] = 2
dim2: Const[i32] = 3

def g():
    a: i32[dim, dim2] = empty((dim, dim2), dtype=int32)
    i1: i32 = 0
    i2: i32 = 0
    for i1 in range(dim):
        for i2 in range(dim2):
            a[i1, i2] = i32(i1 * dim2 + i2)
    # a: [[0, 1, 2], [3, 4, 5]]
    print(a)
    assert a[-1, -1] == 5
    assert a[-1, -2] == 4
    assert a[-1, -3] == 3
    assert a[-2, -1] == 2
    assert a[-2, -2] == 1
    assert a[-2, -3] == 0

g()