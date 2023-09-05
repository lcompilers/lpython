from lpython import i32, f64, f32
from numpy import empty, sinh, cosh, reshape, int32, float64, sin, arcsinh


def verify1d(array: f32[:], result: f32[:], size: i32):
    i: i32
    eps: f32 = f32(1e-6)

    for i in range(size):
        assert abs(arcsinh(arcsinh(array[i])) - result[i]) <= eps

def verifynd(array: f64[:, :, :, :], result: f64[:, :, :, :], size1: i32, size2: i32, size3: i32, size4: i32):
    i: i32; size: i32;
    eps: f64 = 1e-12
    shape: i32[1] = empty((1,), dtype=int32)
    size = size1 * size2 * size3 * size4
    shape[0] = size
    array1d: f64[12800] = reshape(array, shape)
    result1d: f64[12800] = reshape(result, shape)

    for i in range(size):
        assert abs((arcsinh(array1d[i]) + 2.0)/2.0 - result1d[i]) <= eps


def elemental_arcsinh():
    i: i32; j: i32; k: i32; l: i32; size: i32;

    array1d: f32[10] = empty(10)
    arcsinh1d: f32[10] = empty(10)

    for i in range(10):
        array1d[i] = f32(f64(i)/10.0)

    arcsinh1d = arcsinh(arcsinh(array1d))
    verify1d(array1d, arcsinh1d, 10)

    arraynd: f64[40, 10, 16, 2] = empty((40, 10, 16, 2))
    arcsinhnd: f64[40, 10, 16, 2] = empty((40, 10, 16, 2))
    size = 40 * 10 * 16 * 2

    for i in range(40):
        for j in range(10):
            for k in range(16):
                for l in range(2):
                    arraynd[i, j, k, l] = float(i + 2*j + 3*k + 4*k)/float(size)

    arcsinhnd = (arcsinh(arraynd) + 2.0)/2.0

    verifynd(arraynd, arcsinhnd, 40, 10, 16, 2)


elemental_arcsinh()
