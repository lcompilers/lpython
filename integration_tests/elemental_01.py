from ltypes import i32, f64
from numpy import empty, sin

def verify1d(array: f64[:], result: f64[:], size: i32):
    i: i32
    eps: f64
    eps = 1e-12

    for i in range(size):
        assert abs(sin(array[i]) - result[i]) <= eps

def verifynd(array: f64[:, :, :], result: f64[:, :, :], size1: i32, size2: i32, size3: i32):
    i: i32
    j: i32
    k: i32
    eps: f64
    eps = 1e-12

    for i in range(size1):
        for j in range(size2):
            for k in range(size3):
                assert abs(sin(array[i, j, k]) - result[i, j, k]) <= eps

def elemental_sin():
    i: i32
    j: i32
    k: i32

    array1d: f64[256] = empty(256)
    sin1d: f64[256] = empty(256)

    for i in range(256):
        array1d[i] = float(i)

    sin1d = sin(array1d)

    verify1d(array1d, sin1d, 256)

    arraynd: f64[256, 64, 16] = empty((256, 64, 16))
    sinnd: f64[256, 64, 16] = empty((256, 64, 16))

    for i in range(256):
        for j in range(64):
            for k in range(16):
                arraynd[i, j, k] = float(i + j + k)

    sinnd = sin(arraynd)

    verifynd(arraynd, sinnd, 256, 64, 16)

elemental_sin()
