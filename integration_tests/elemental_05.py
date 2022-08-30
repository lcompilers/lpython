from ltypes import i32, f64, f32
from numpy import empty, sinh, cosh, reshape, int32, float64, sin

def verify1d(array: f32[:], result: f32[:], size: i32):
    i: i32
    eps: f32 = 1e-6

    for i in range(size):
        assert abs(sinh(sinh(array[i])) - result[i]) <= eps

def verifynd(array: f64[:, :, :, :], result: f64[:, :, :, :], size1: i32, size2: i32, size3: i32, size4: i32):
    i: i32; size: i32;
    eps: f64 = 1e-12
    shape: i32[1] = empty((1,), dtype=int32)
    size = size1 * size2 * size3 * size4
    shape[0] = size
    array1d: f64[12800] = reshape(array, shape)
    result1d: f64[12800] = reshape(result, shape)

    for i in range(size):
        assert abs((sinh(array1d[i]) + 2)/2 - result1d[i]) <= eps


def elemental_sinh():
    i: i32; j: i32; k: i32; l: i32; size: i32;

    array1d: f32[10] = empty(10)
    sinh1d: f32[10] = empty(10)

    for i in range(10):
        array1d[i] = i/10.0

    sinh1d = sinh(sinh(array1d))
    verify1d(array1d, sinh1d, 10)

    arraynd: f64[40, 10, 16, 2] = empty((40, 10, 16, 2))
    sinhnd: f64[40, 10, 16, 2] = empty((40, 10, 16, 2))
    size = 40 * 10 * 16 * 2

    for i in range(40):
        for j in range(10):
            for k in range(16):
                for l in range(2):
                    arraynd[i, j, k, l] = float(i + 2*j + 3*k + 4*k)/size

    sinhnd = (sinh(arraynd) + 2)/2

    verifynd(arraynd, sinhnd, 40, 10, 16, 2)

def verify2d(array: f64[:, :], result: f64[:, :], size1: i32, size2: i32):
    i: i32; j: i32;
    eps: f64 = 1e-12

    for i in range(size1):
        for j in range(size2):
            assert abs(cosh(5 + array[i, j])**2 - result[i, j]) <= eps

def elemental_cosh():
    i: i32; j: i32

    array2d: f64[20, 10] = empty((20, 10))
    cosh2d: f64[20, 10] = empty((20, 10))

    for i in range(20):
        for j in range(10):
                array2d[i, j] = (i + 2*j)/200.0

    cosh2d = cosh(5 + (array2d))**2
    verify2d(array2d, cosh2d, 20, 10)

def elemental_cosh_():
    i: i32
    j: i32

    array2d: f64[20, 10] = empty((20, 10))
    cosh2d: f64[20, 10] = empty((20, 10))

    for i in range(20):
        for j in range(10):
                array2d[i, j] = (i + 2*j)/200.0

    cosh2d = cosh(5 + (array2d))**2
    verify2d(array2d, cosh2d, 20, 10)

def elemental_trig_identity():
    i: i32; j: i32; k: i32; l: i32
    eps: f64 = 1e-12

    arraynd: f64[10, 5, 2, 4] = empty((10, 5, 2, 4), dtype=float64)

    identity1: f64[10, 5, 2, 4] = empty((10, 5, 2, 4), dtype=float64)
    identity2: f64[10, 5, 2, 4] = empty((10, 5, 2, 4), dtype=float64)
    identity3: f64[10, 5, 2, 4] = empty((10, 5, 2, 4), dtype=float64)
    identity4: f64[10, 5, 2, 4] = empty((10, 5, 2, 4), dtype=float64)

    observed1d_1: f64[400] = empty(400, dtype=float64)
    observed1d_2: f64[400] = empty(400, dtype=float64)
    observed1d_3: f64[400] = empty(400, dtype=float64)
    observed1d_4: f64[400] = empty(400, dtype=float64)

    for i in range(10):
        for j in range(5):
            for k in range(2):
                for l in range(4):
                    arraynd[i, j, k, l] = sin(float(i + j + k + l))

    identity1 = 1.0 - cosh(arraynd)**2 + sinh(arraynd)**2
    identity2 =  cosh(-1 * arraynd) - cosh(arraynd)
    identity3 =  sinh(-1 * arraynd) + sinh(arraynd)
    identity4 =  (cosh(arraynd/4 + arraynd/2) -
                  cosh(arraynd/4) * cosh(arraynd/2) -
                  sinh(arraynd/4) * sinh(arraynd/2))

    newshape: i32[1] = empty(1, dtype=int)
    newshape[0] = 400

    observed1d_1 = reshape(identity1, newshape)
    observed1d_2 = reshape(identity2, newshape)
    observed1d_3 = reshape(identity3, newshape)
    observed1d_4 = reshape(identity4, newshape)

    for i in range(400):
        assert abs(observed1d_1[i] - 0.0) <= eps
        assert abs(observed1d_2[i] - 0.0) <= eps
        assert abs(observed1d_3[i] - 0.0) <= eps
        assert abs(observed1d_4[i] - 0.0) <= eps


elemental_sinh()
elemental_cosh()
elemental_trig_identity()
