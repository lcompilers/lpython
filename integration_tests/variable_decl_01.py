from ltypes import i32, i64
from numpy import empty, int64

def f(n: i32, m: i32):
    l: i32 = 2
    a: i64[n, m, l] = empty((n, m, l), dtype=int64)
    i: i32; j: i32; k: i32;
    for i in range(n):
        for j in range(m):
            for k in range(l):
                a[i, j, k] = i64(i + j + k)

    for i in range(n):
        for j in range(m):
            for k in range(l):
                print(a[i, j, k])
                assert a[i, j, k] == i64(i + j + k)

f(5, 10)
