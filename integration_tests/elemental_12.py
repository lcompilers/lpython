from lpython import i32, f32, f64
from numpy import empty, floor, ceil, sqrt, reshape, int32, float32, float64

def elemental_floor64():
    i: i32
    j: i32
    k: i32
    l: i32
    eps: f32
    eps = f32(1e-6)

    arraynd: f64[32, 16, 8, 4] = empty((32, 16, 8, 4), dtype=float64)

    newshape: i32[1] = empty(1, dtype=int32)
    newshape[0] = 16384

    for i in range(32):
        for j in range(16):
            for k in range(8):
                for l in range(4):
                    arraynd[i, j, k, l] = f64((-1)**l) * sqrt(float(i + j + k + l))

    observed: f64[32, 16, 8, 4] = empty((32, 16, 8, 4), dtype=float64)
    observed = floor(arraynd)

    observed1d: f64[16384] = empty(16384, dtype=float64)
    observed1d = reshape(observed, newshape)

    array: f64[16384] = empty(16384, dtype=float64)
    array = reshape(arraynd, newshape)

    for i in range(16384):
        assert f32(abs(floor(array[i]) - observed1d[i])) <= eps


def elemental_floor32():
    i: i32
    j: i32
    k: i32
    l: i32
    eps: f32
    eps = f32(1e-6)

    arraynd: f32[32, 16, 8, 4] = empty((32, 16, 8, 4), dtype=float32)

    for i in range(32):
        for j in range(16):
            for k in range(8):
                for l in range(4):
                    arraynd[i, j, k, l] = f32(f64((-1)**l) * sqrt(float(i + j + k + l)))

    observed: f32[32, 16, 8, 4] = empty((32, 16, 8, 4), dtype=float32)
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
    eps = f32(1e-6)

    arraynd: f64[32, 16, 8, 4] = empty((32, 16, 8, 4), dtype=float64)

    newshape: i32[1] = empty(1, dtype=int32)
    newshape[0] = 16384

    for i in range(32):
        for j in range(16):
            for k in range(8):
                for l in range(4):
                    arraynd[i, j, k, l] = f64((-1)**l) * sqrt(float(i + j + k + l))

    observed: f64[32, 16, 8, 4] = empty((32, 16, 8, 4), dtype=float64)
    observed = ceil(arraynd)

    observed1d: f64[16384] = empty(16384, dtype=float64)
    observed1d = reshape(observed, newshape)

    array: f64[16384] = empty(16384, dtype=float64)
    array = reshape(arraynd, newshape)

    for i in range(16384):
        assert f32(abs(ceil(array[i]) - observed1d[i])) <= eps


def elemental_ceil32():
    i: i32
    j: i32
    k: i32
    l: i32
    eps: f32
    eps = f32(1e-6)

    arraynd: f32[32, 16, 8, 4] = empty((32, 16, 8, 4), dtype=float32)

    for i in range(32):
        for j in range(16):
            for k in range(8):
                for l in range(4):
                    arraynd[i, j, k, l] = f32(f64((-1)**l) * sqrt(float(i + j + k + l)))

    observed: f32[32, 16, 8, 4] = empty((32, 16, 8, 4), dtype=float32)
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
