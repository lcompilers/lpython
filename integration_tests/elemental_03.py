from ltypes import i32, f32, f64
from numpy import empty, sqrt, reshape

def elemental_sqrt64():
    array: f64[16, 16, 16] = empty((16, 16, 16))
    observed: f64[4096] = empty(4096)
    shape: i32[1] = empty(1, dtype=int)
    eps: f64
    eps = 1e-12
    i: i32
    j: i32
    k: i32
    l: i32

    for i in range(16):
        for j in range(16):
            for k in range(16):
                array[i, j, k] = float(i + j + k)

    shape[0] = 4096
    observed = reshape(sqrt(array), shape)
    for l in range(4096):
        i = int(l/256)
        j = (l - i*256)//16
        k = (l - i*256 - j*16)
        assert abs(observed[l]**2 - i - j - k) <= eps

def elemental_sqrt32():
    array: f32[16, 16] = empty((16, 16))
    observed: f32[256] = empty(256)
    shape: i32[1] = empty(1, dtype=int)
    eps: f32
    eps = 2e-6
    i: i32
    j: i32
    l: i32

    for i in range(16):
        for j in range(16):
            array[i, j] = float(i + j)

    shape[0] = 256
    observed = reshape(sqrt(array), shape)
    for l in range(256):
        i = int(l/16)
        j = (l - i*16)
        assert abs(observed[l]**2 - i - j) <= eps

elemental_sqrt64()
elemental_sqrt32()
