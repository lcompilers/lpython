from lpython import i32, f64, f32
from numpy import empty, arcsinh, arccosh, reshape, int32, float32, float64, sinh, sqrt, sin

def verify1d_arcsinh(array: f32[:], result: f32[:], size: i32):
    i: i32
    eps: f32
    eps = f32(1e-6)

    for i in range(size):
        assert abs(arcsinh(arcsinh(array[i])) - result[i]) <= eps

def verifynd_arcsinh(array: f64[:, :, :], result: f64[:, :, :], size1: i32, size2: i32, size3: i32):
    i: i32
    j: i32
    k: i32
    eps: f64
    eps = 1e-12

    for i in range(size1):
        for j in range(size2):
            for k in range(size3):
                assert abs( (1.0 + arcsinh(array[i, j, k])) - result[i, j, k]) <= eps


def elemental_arcsinh():
    i: i32
    j: i32
    k: i32

    array1d: f32[256] = empty(256, dtype=float32)
    arcsinh1d: f32[256] = empty(256, dtype=float32)

    for i in range(256):
        array1d[i] = f32(i)

    arcsinh1d = arcsinh(arcsinh(array1d))
    verify1d_arcsinh(array1d, arcsinh1d, 256)

    arraynd: f64[256, 64, 4] = empty((256, 64, 4), dtype=float64)
    arcsinhnd: f64[256, 64, 4] = empty((256, 64, 4), dtype=float64)

    for i in range(256):
        for j in range(64):
            for k in range(4):
                arraynd[i, j, k] = float(i + j + k)

    arcsinhnd = (1.0 + arcsinh(arraynd))
    verifynd_arcsinh(arraynd, arcsinhnd, 256, 64, 4)

def verify2d_arccosh(array: f64[:, :], result: f64[:, :], size1: i32, size2: i32):
    i: i32
    j: i32
    eps: f64
    eps = 1e-12

    for i in range(size1):
        for j in range(size2):
            assert abs(arccosh(array[i, j])**2.0 - result[i, j]) <= eps

def verifynd_arccosh(array: f64[:, :, :, :], result: f64[:, :, :, :], size1: i32, size2: i32, size3: i32, size4: i32):
    i: i32
    j: i32
    k: i32
    l: i32
    eps: f64
    eps = 1e-12

    for i in range(size1):
        for j in range(size2):
            for k in range(size3):
                for l in range(size4):
                    assert abs(100.0 + arccosh(array[i, j, k, l]) / 100.0 - result[i, j, k, l]) <= eps

def elemental_arccosh():
    i: i32
    j: i32
    k: i32
    l: i32

    array2d: f64[256, 64] = empty((256, 64), dtype=float64)
    arccosh2d: f64[256, 64] = empty((256, 64), dtype=float64)

    for i in range(256):
        for j in range(64):
                array2d[i, j] = 2.0 + float(i + j * 2)

    arccosh2d = arccosh(array2d)**2.0
    verify2d_arccosh(array2d, arccosh2d, 256, 64)

    arraynd: f64[32, 16, 4, 2] = empty((32, 16, 4, 2), dtype=float64)
    arccosh_nd: f64[32, 16, 4, 2] = empty((32, 16, 4, 2), dtype=float64)

    for i in range(32):
        for j in range(16):
            for k in range(4):
                for l in range(2):
                    arraynd[i, j, k, l] = 2.0 + float(i / 4 + j / 3 + k / 2 + f64(l)) / 100.0

    arccosh_nd =  100.0 + arccosh(arraynd) / 100.0
    verifynd_arccosh(arraynd, arccosh_nd, 32, 16, 4, 2)

def elemental_trig_identity():
    i: i32; j: i32; k: i32; l: i32
    eps: f64 = 1e-12

    arraynd: f64[10, 5, 2, 4] = empty((10, 5, 2, 4), dtype=float64)

    identity1: f64[10, 5, 2, 4] = empty((10, 5, 2, 4), dtype=float64)
    identity2: f64[10, 5, 2, 4] = empty((10, 5, 2, 4), dtype=float64)
    identity3: f64[10, 5, 2, 4] = empty((10, 5, 2, 4), dtype=float64)


    observed1d_1: f64[400] = empty(400, dtype=float64)
    observed1d_2: f64[400] = empty(400, dtype=float64)
    observed1d_3: f64[400] = empty(400, dtype=float64)


    for i in range(10):
        for j in range(5):
            for k in range(2):
                for l in range(4):
                    arraynd[i, j, k, l] = 2.0 + sin((float(i + j + k + l)))

    identity1 = 2.0 * arccosh(arraynd) - arccosh((arraynd**2.0) * 2.0 - 1.0)
    identity2 =  sinh(arccosh(arraynd)) - sqrt((arraynd**2.0) - 1.0)
    identity3 =  2.0 * arcsinh(arraynd) - arccosh((arraynd**2.0) * 2.0 + 1.0)


    newshape: i32[1] = empty(1, dtype=int32)
    newshape[0] = 400

    observed1d_1 = reshape(identity1, newshape)
    observed1d_2 = reshape(identity2, newshape)
    observed1d_3 = reshape(identity3, newshape)


    for i in range(400):
        assert abs(observed1d_1[i] - 0.0) <= eps
        assert abs(observed1d_2[i] - 0.0) <= eps
        assert abs(observed1d_3[i] - 0.0) <= eps


elemental_arcsinh()
elemental_arccosh()
elemental_trig_identity()
