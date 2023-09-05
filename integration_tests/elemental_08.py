from lpython import i32, f64, f32
from numpy import empty, reshape, int32, float32, float64, exp

def verify1d(array: f32[:], result: f32[:], size: i32):
    i: i32
    eps: f32 = f32(1e-6)

    for i in range(size):
        assert abs(exp(array[i]) - result[i]) <= eps

def verifynd(array: f64[:, :, :, :], result: f64[:, :, :, :], size1: i32, size2: i32, size3: i32, size4: i32):
    i: i32; size: i32;
    eps: f64 = 1e-12
    shape: i32[1] = empty((1,), dtype=int32)
    size = size1 * size2 * size3 * size4
    shape[0] = size
    array1d: f64[1024] = reshape(array, shape)
    result1d: f64[1024] = reshape(result, shape)

    for i in range(size):
        assert abs(( exp(array1d[i]) + exp(array1d[i] / 3.0) ) / float(2) - result1d[i]) <= eps

def elemental_exp():
    i: i32; j: i32; k: i32; l: i32; size: i32;

    array1d: f32[80] = empty(80, dtype=float32)
    exp1d: f32[80] = empty(80, dtype=float32)

    for i in range(80):
        array1d[i] = f32(f64(i) / 50.0)

    exp1d = exp(array1d)
    verify1d(array1d, exp1d, 80)

    arraynd: f64[16, 8, 4, 2] = empty((16, 8, 4, 2), dtype=float64)
    expnd: f64[16, 8, 4, 2] = empty((16, 8, 4, 2), dtype=float64)
    size = 32

    for i in range(16):
        for j in range(8):
            for k in range(4):
                for l in range(2):
                    arraynd[i, j, k, l] = float( f64(i) / 8.0 + f64(j) / 4.0 + f64(k) / 2.0 + f64(l) ) / float(size)

    expnd = (exp(arraynd) + exp(arraynd / 3.0)) / 2.0

    verifynd(arraynd, expnd, 16, 8, 4, 2)

elemental_exp()
