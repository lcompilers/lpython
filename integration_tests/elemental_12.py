from ltypes import i32, f32, f64
from numpy import empty, floor, ceil, sqrt, reshape

def elemental_floor64():
    i: i32
    j: i32
    k: i32
    l: i32
    eps: f32
    eps = 1e-6

    arraynd: f64[32, 16, 8, 4] = empty((32, 16, 8, 4))

    newshape: i32[1] = empty(1, dtype=int)
    newshape[0] = 16384

    for i in range(32):
        for j in range(16):
            for k in range(8):
                for l in range(4):
                    arraynd[i, j, k, l] = ((-1)**l) * sqrt(float(i + j + k + l))

    observed: f64[32, 16, 8, 4] = empty((32, 16, 8, 4))
    observed = floor(arraynd)

    observed1d: f64[16384] = empty(16384)
    observed1d = reshape(observed, newshape)

    array: f64[16384] = empty(16384)
    array = reshape(arraynd, newshape)

    for i in range(16384):
        assert abs(floor(array[i]) - observed1d[i]) <= eps


def elemental_floor32():
    i: i32
    j: i32
    k: i32
    l: i32
    eps: f32
    eps = 1e-6

    arraynd: f32[32, 16, 8, 4] = empty((32, 16, 8, 4))

    for i in range(32):
        for j in range(16):
            for k in range(8):
                for l in range(4):
                    arraynd[i, j, k, l] = ((-1)**l) * sqrt(float(i + j + k + l))

    observed: f32[32, 16, 8, 4] = empty((32, 16, 8, 4))
    observed = floor(arraynd)

    for i in range(32):
        for j in range(16):
            for k in range(8):
                for l in range(4):
                    assert abs(floor(arraynd[i, j, k, l]) - observed[i, j, k, l]) <= eps


def elemental_ceil64():
    i: i32
    j: i32
    k: i32
    l: i32
    eps: f32
    eps = 1e-6

    arraynd: f64[32, 16, 8, 4] = empty((32, 16, 8, 4))

    newshape: i32[1] = empty(1, dtype=int)
    newshape[0] = 16384

    for i in range(32):
        for j in range(16):
            for k in range(8):
                for l in range(4):
                    arraynd[i, j, k, l] = ((-1)**l) * sqrt(float(i + j + k + l))

    observed: f64[32, 16, 8, 4] = empty((32, 16, 8, 4))
    observed = ceil(arraynd)

    observed1d: f64[16384] = empty(16384)
    observed1d = reshape(observed, newshape)

    array: f64[16384] = empty(16384)
    array = reshape(arraynd, newshape)

    for i in range(16384):
        assert abs(ceil(array[i]) - observed1d[i]) <= eps


def elemental_ceil32():
    i: i32
    j: i32
    k: i32
    l: i32
    eps: f32
    eps = 1e-6

    arraynd: f32[32, 16, 8, 4] = empty((32, 16, 8, 4))

    for i in range(32):
        for j in range(16):
            for k in range(8):
                for l in range(4):
                    arraynd[i, j, k, l] = ((-1)**l) * sqrt(float(i + j + k + l))

    observed: f32[32, 16, 8, 4] = empty((32, 16, 8, 4))
    observed = ceil(arraynd)

    for i in range(32):
        for j in range(16):
            for k in range(8):
                for l in range(4):
                    assert abs(ceil(arraynd[i, j, k, l]) - observed[i, j, k, l]) <= eps


elemental_floor64()
elemental_floor32()
elemental_ceil64()
elemental_ceil32()
