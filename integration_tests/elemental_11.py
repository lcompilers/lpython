from lpython import i32, f64, f32
from numpy import empty, arctanh, reshape, int32, float32, float64, sinh, sqrt, sin,  cosh

def verify1d_arctanh(array: f32[:], result: f32[:], size: i32):
    i: i32
    eps: f32
    eps = f32(1e-6)

    for i in range(size):
        assert abs(arctanh(array[i]) - result[i]) <= eps

def verifynd_arctanh(array: f64[:, :, :], result: f64[:, :, :], size1: i32, size2: i32, size3: i32):
    i: i32
    j: i32
    k: i32
    eps: f64
    eps = 1e-12

    for i in range(size1):
        for j in range(size2):
            for k in range(size3):
                assert abs( arctanh(array[i, j, k])**2.0 - result[i, j, k]) <= eps


def elemental_arctanh():
    i: i32
    j: i32
    k: i32

    array1d: f32[999] = empty(999, dtype=float32)
    arctanh1d: f32[999] = empty(999, dtype=float32)

    for i in range(999):
        array1d[i] = f32(f64((-1)**i) * (float(i)/1000.0))

    arctanh1d = arctanh(array1d)
    verify1d_arctanh(array1d, arctanh1d, 999)

    arraynd: f64[100, 50, 10] = empty((100, 50, 10), dtype=float64)
    arctanhnd: f64[100, 50, 10] = empty((100, 50, 10), dtype=float64)

    for i in range(100):
        for j in range(50):
            for k in range(10):
                arraynd[i, j, k] = sin(f64((-1)**k) * float(i + j + k))

    arctanhnd =  arctanh(arraynd)**2.0
    verifynd_arctanh(arraynd, arctanhnd, 100, 50, 10)



def elemental_trig_identity():
    i: i32; j: i32; k: i32; l: i32
    eps: f64 = 1e-12

    arraynd: f64[10, 5, 2, 4] = empty((10, 5, 2, 4), dtype=float64)

    identity1: f64[10, 5, 2, 4] = empty((10, 5, 2, 4), dtype=float64)
    identity2: f64[10, 5, 2, 4] = empty((10, 5, 2, 4), dtype=float64)

    observed1d_1: f64[400] = empty(400, dtype=float64)
    observed1d_2: f64[400] = empty(400, dtype=float64)

    for i in range(10):
        for j in range(5):
            for k in range(2):
                for l in range(4):
                    arraynd[i, j, k, l] = ((float(i + j + k + l)))/25.0

    identity1 = 2.0 * arctanh(arraynd) - arctanh((2.0 * arraynd) / ( 1.0 + arraynd**2.0))
    identity2 =  cosh(arctanh(arraynd)) - (sqrt(1.0 - (arraynd**2.0)))**(-1.0)

    newshape: i32[1] = empty(1, dtype=int32)
    newshape[0] = 400

    observed1d_1 = reshape(identity1, newshape)
    observed1d_2 = reshape(identity2, newshape)

    for i in range(400):
        assert abs(observed1d_1[i] - 0.0) <= eps
        assert abs(observed1d_2[i] - 0.0) <= eps


elemental_arctanh()
elemental_trig_identity()
