from lpython import f64, i32
from numpy import reshape, empty, int32, float64

def test_nd_to_1d(a: f64[:, :]):
    i: i32
    j: i32
    k: i32
    l: i32
    eps: f64
    eps = 1e-12

    b: f64[256] = empty(256, dtype=float64)
    newshape: i32[1] = empty(1, dtype=int32)
    newshape[0] = 256
    b = reshape(a, newshape)
    for k in range(256):
        i = k//16
        j = k - i*16
        assert abs(b[k] - f64(i + j) - 0.5) <= eps

    c: f64[16, 16, 16] = empty((16, 16, 16), dtype=float64)
    c = empty((16, 16, 16), dtype=float64)
    for i in range(16):
        for j in range(16):
            for k in range(16):
                c[i, j, k] = f64(i + j + k) + 0.5

    d: f64[4096] = empty(4096, dtype=float64)
    newshape1: i32[1] = empty(1, dtype=int32)
    newshape1[0] = 4096
    d = reshape(c, newshape1)
    for l in range(4096):
        i = i32(l/256)
        j = (l - i*256)//16
        k = (l - i*256 - j*16)
        assert abs(d[l] - f64(i + j + k) - 0.5) <= eps

def test_1d_to_nd(d: f64[:]):
    i: i32
    j: i32
    k: i32
    l: i32
    eps: f64
    eps = 1e-12

    b: f64[256] = empty(256, dtype=float64)
    for k in range(256):
        i = k//16
        j = k - i*16
        b[k] = f64(i + j) + 0.5

    a: f64[16, 16]
    a = empty((16, 16), dtype=float64)
    newshape: i32[2] = empty(2, dtype=int32)
    newshape[0] = 16
    newshape[1] = 16
    a = reshape(b, newshape)
    for i in range(16):
        for j in range(16):
            assert abs(a[i, j] - f64(i + j) - 0.5) <= eps

    c: f64[16, 16, 16]
    c = empty((16, 16, 16), dtype=float64)
    newshape1: i32[3] = empty(3, dtype=int32)
    newshape1[0] = 16
    newshape1[1] = 16
    newshape1[2] = 16
    c = reshape(d, newshape1)
    for i in range(16):
        for j in range(16):
            for k in range(16):
                assert abs(c[i, j, k] - f64(i + j + k) - 0.5) <= eps

def test_reshape_with_argument():
    i: i32
    j: i32
    k: i32
    l: i32

    a: f64[16, 16]
    a = empty((16, 16), dtype=float64)
    for i in range(16):
        for j in range(16):
            a[i, j] = f64(i + j) + 0.5

    test_nd_to_1d(a)

    d: f64[4096] = empty(4096, dtype=float64)
    for l in range(4096):
        i = i32(l/256)
        j = (l - i*256)//16
        k = (l - i*256 - j*16)
        d[l] = f64(i + j + k) + 0.5

    test_1d_to_nd(d)

test_reshape_with_argument()
