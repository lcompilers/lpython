from ltypes import f64, i32
from numpy import reshape, empty

def test_nd_to_1d():
    a: f64[16, 16]
    a = empty((16, 16))
    i: i32
    j: i32
    k: i32
    eps: f64
    eps = 1e-12

    for i in range(16):
        for j in range(16):
            a[i, j] = i + j + 0.5

    b: f64[256]
    newshape: i32[1] = empty(1, dtype=int)
    newshape[0] = 256
    b = reshape(a, newshape)
    for k in range(256):
        i = k//16
        j = k - i*16
        assert abs(b[k] - i - j - 0.5) <= eps

test_nd_to_1d()
