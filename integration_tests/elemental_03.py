from lpython import i32, f32, f64
from numpy import empty, sqrt, reshape, int32, float32, float64

def elemental_sqrt64():
    array: f64[16, 16, 16] = empty((16, 16, 16), dtype=float64)
    observed: f64[4096] = empty(4096, dtype=float64)
    shape: i32[1] = empty(1, dtype=int32)
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
        i = i32(l/256)
        j = (l - i*256)//16
        k = (l - i*256 - j*16)
        assert abs(observed[l]**2.0 - f64(i + j + k)) <= eps

def elemental_sqrt32():
    array: f32[16, 16] = empty((16, 16), dtype=float32)
    observed: f32[256] = empty(256, dtype=float32)
    shape: i32[1] = empty(1, dtype=int32)
    eps: f32
    eps = f32(5e-6)
    i: i32
    j: i32
    l: i32

    for i in range(16):
        for j in range(16):
            array[i, j] = f32(i + j)

    shape[0] = 256
    observed = reshape(sqrt(array), shape)
    for l in range(256):
        i = i32(l/16)
        j = (l - i*16)
        assert abs(observed[l]**f32(2.0) - f32(i + j)) <= eps


def elemental_norm():
    array_a: f64[100] = empty(100, dtype=float64)
    array_b: f64[100] = empty(100, dtype=float64)
    array_c: f64[100] = empty(100, dtype=float64)

    i: i32
    j: i32

    for i in range(100):
        array_a[i] = float(i)

    for j in range(100):
        array_b[j] = float(j+5)

    array_c = sqrt(array_a**2.0 + array_b**2.0)

    eps: f64
    eps = 1e-12

    for i in range(100):
        assert abs(array_c[i] - sqrt(array_a[i]**2.0 + array_b[i]**2.0)) <= eps


elemental_sqrt64()
elemental_sqrt32()
elemental_norm()
