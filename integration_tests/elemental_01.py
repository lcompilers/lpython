from lpython import i32, f64, f32
from numpy import empty, sin, cos, reshape, int32, float32, float64

def verify1d(array: f32[:], result: f32[:], size: i32):
    i: i32
    eps: f32
    eps = f32(1e-6)

    for i in range(size):
        assert abs(sin(sin(array[i])) - result[i]) <= eps

def verifynd(array: f32[:, :, :], result: f32[:, :, :], size1: i32, size2: i32, size3: i32):
    i: i32
    j: i32
    k: i32
    eps: f32
    eps = f32(1e-6)

    for i in range(size1):
        for j in range(size2):
            for k in range(size3):
                assert abs(sin(array[i, j, k])**f32(2) - result[i, j, k]) <= eps

def verify2d(array: f64[:, :], result: f64[:, :], size1: i32, size2: i32):
    i: i32
    j: i32
    eps: f64
    eps = 1e-12

    for i in range(size1):
        for j in range(size2):
            assert abs(cos(array[i, j])**2.0 - result[i, j]) <= eps


def verify1d_sum(array_a: f64[:], array_b: f64[:], result: f64[:], size: i32):
    i: i32
    eps: f64
    eps = 1e-9

    for i in range(size):
        assert abs(array_a[i]**2.0 + 5.0*array_b[i]**3.0 - result[i]) <= eps


def verify1d_mul(array_a: f64[:], array_b: f64[:], result: f64[:], size: i32):
    i: i32
    eps: f64
    eps = 1e-5

    for i in range(size):
        assert abs(array_a[i]**2.0 * 5.0*array_b[i]**3.0 - result[i]) <= eps


def elemental_sum():
    i: i32
    j: i32
    k: i32

    array_a: f64[100] = empty(100, dtype=float64)
    array_b: f64[100] = empty(100, dtype=float64)
    array_c: f64[100] = empty(100, dtype=float64)

    for i in range(100):
        array_a[i] = float(i)

    for j in range(100):
        array_b[j] = float(j+5)

    array_c = array_a**2.0 + 5.0*array_b**3.0
    verify1d_sum(array_a, array_b, array_c, 100)


def elemental_mul():
    i: i32
    j: i32
    k: i32

    array_a: f64[100] = empty(100, dtype=float64)
    array_b: f64[100] = empty(100, dtype=float64)
    array_c: f64[100] = empty(100, dtype=float64)

    for i in range(100):
        array_a[i] = float(i)

    for j in range(100):
        array_b[j] = float(j+5)

    array_c = array_a**2.0 * 5.0*array_b**3.0
    verify1d_mul(array_a, array_b, array_c, 100)


def elemental_sin():
    i: i32
    j: i32
    k: i32

    array1d: f32[256] = empty(256, dtype=float32)
    sin1d: f32[256] = empty(256, dtype=float32)

    for i in range(256):
        array1d[i] = f32(i)

    sin1d = sin(sin(array1d))

    verify1d(array1d, sin1d, 256) # working fine

    arraynd: f32[256, 64, 16] = empty((256, 64, 16), dtype=float32)
    sinnd: f32[256, 64, 16] = empty((256, 64, 16), dtype=float32)

    for i in range(256):
        for j in range(64):
            for k in range(16):
                arraynd[i, j, k] = f32(i + j + k)

    sinnd = sin(arraynd)**f32(2)

    verifynd(arraynd, sinnd, 256, 64, 16)

def elemental_cos():
    i: i32
    j: i32

    array2d: f64[256, 64] = empty((256, 64), dtype=float64)
    cos2d: f64[256, 64] = empty((256, 64), dtype=float64)

    for i in range(256):
        for j in range(64):
                array2d[i, j] = float(i + j)

    cos2d = cos(array2d)**2.0

    verify2d(array2d, cos2d, 256, 64)

def elemental_trig_identity():
    i: i32
    j: i32
    k: i32
    l: i32
    eps: f32
    eps = f32(1e-6)

    arraynd: f32[64, 32, 8, 4] = empty((64, 32, 8, 4), dtype=float32)
    observed: f32[64, 32, 8, 4] = empty((64, 32, 8, 4), dtype=float32)
    observed1d: f32[65536] = empty(65536, dtype=float32)

    for i in range(64):
        for j in range(32):
            for k in range(8):
                for l in range(4):
                    arraynd[i, j, k, l] = f32(i + j + k + l)

    observed = sin(arraynd)**f32(2) + cos(arraynd)**f32(2)

    newshape: i32[1] = empty(1, dtype=int32)
    newshape[0] = 65536
    observed1d = reshape(observed, newshape)

    for i in range(65536):
        assert abs(observed1d[i] - f32(1.0)) <= eps


elemental_sin()
elemental_cos()
elemental_trig_identity()
elemental_sum()
elemental_mul()
