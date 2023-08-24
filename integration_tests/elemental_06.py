from lpython import i32, f32, f64
from numpy import empty, arcsin, arccos, sin, cos, sqrt, arctan, tan, degrees, radians, float32, float64
from math import pi

def verify1d_same(array: f32[:], result: f32[:], size: i32):
    i: i32
    eps: f32
    eps = f32(1e-6)
    for i in range(size):
        assert abs(array[i] - result[i]) <= eps

def verify_arcsin_1d(array: f32[:], result: f32[:], size: i32):
    i: i32
    eps: f32
    eps = f32(1e-6)
    for i in range(size):
        assert abs(arcsin(array[i])**f32(2.0) - result[i]) <= eps

def verify_arcsin_2d(array: f64[:, :], result: f64[:, :], size1:i32, size2:i32):
    i: i32
    j: i32
    eps: f64
    eps = 1e-12
    for i in range(size1):
        for j in range(size2):
            assert abs(arcsin(array[i, j])**2.0 - result[i, j]) <= eps

def verify_arccos_1d(array: f32[:], result: f32[:], size: i32):
    i: i32
    eps: f32
    eps = f32(1e-6)
    for i in range(size):
        assert abs(arccos(array[i])**f32(2.0) - result[i]) <= eps

def verify_arccos_2d(array: f64[:, :], result: f64[:, :], size1:i32, size2:i32):
    i: i32
    j: i32
    eps: f64
    eps = 1e-12
    for i in range(size1):
        for j in range(size2):
            assert abs(arccos(array[i, j])**2.0 - result[i, j]) <= eps

def verify_arctan_1d(array: f32[:], result: f32[:], size: i32):
    i: i32
    eps: f32
    eps = f32(1e-6)
    for i in range(size):
        assert abs(arctan(array[i])**f32(2.0) - result[i]) <= eps

def verify_arctan_2d(array: f64[:, :], result: f64[:, :], size1:i32, size2:i32):
    i: i32
    j: i32
    eps: f64
    eps = 1e-12
    for i in range(size1):
        for j in range(size2):
            assert abs(arctan(array[i, j])**2.0 - result[i, j]) <= eps

def elemental_arcsin():
    i: i32
    j: i32
    array1d: f32[201] = empty(201, dtype=float32)
    arcsin1d: f32[201] = empty(201, dtype=float32)
    for i in range(201):
        array1d[i] =  f32((i - 100)/100)
    arcsin1d = arcsin(array1d) ** f32(2.0)
    verify_arcsin_1d(array1d, arcsin1d, 201)

    array2d: f64[64, 64] = empty((64, 64), dtype=float64)
    arcsin2d: f64[64, 64] = empty((64, 64), dtype=float64)
    for i in range(64):
        for j in range(64): # 2048 = 64 * 32
            array2d[i,j]= float((i * 64 + j - 2048 )/2048)

    arcsin2d = arcsin(array2d) ** 2.0
    verify_arcsin_2d(array2d, arcsin2d, 64, 64)

def elemental_arccos():
    i: i32
    j: i32
    array1d: f32[201] = empty(201, dtype=float32)
    arccos1d: f32[201] = empty(201, dtype=float32)
    for i in range(201):
        array1d[i] =  f32((i - 100)/100)
    arccos1d = arccos(array1d) ** f32(2.0)
    verify_arccos_1d(array1d, arccos1d, 201)

    array2d: f64[64, 64] = empty((64, 64), dtype=float64)
    arccos2d: f64[64, 64] = empty((64, 64), dtype=float64)
    for i in range(64):
        for j in range(64): # 2048 = 64 * 32
            array2d[i,j]= float((i * 64 + j - 2048 )/2048)

    arccos2d = arccos(array2d) ** 2.0
    verify_arccos_2d(array2d, arccos2d, 64, 64)

def elemental_arctan():
    i: i32
    j: i32
    eps: f32
    eps = f32(1e-6)
    array1d: f32[201] = empty(201, dtype=float32)
    array1d_rec: f32[201] = empty(201, dtype=float32)
    arctan1d: f32[201] = empty(201, dtype=float32)
    for i in range(201):
        array1d[i] =  f32(i - 100)
    arctan1d = arctan(array1d) ** f32(2.0)
    verify_arctan_1d(array1d, arctan1d, 201)

    for i in range(201):
        array1d[i] =  f32(i + 1)
        array1d_rec[i] = f32(1.0/f64(i+1))
    arctan1d = arctan(array1d) + arctan(array1d_rec)
    for i in range(201):
        assert abs(arctan1d[i] - f32(f64(pi) / 2.0)) <= eps

    array2d: f64[64, 64] = empty((64, 64), dtype=float64)
    arctan2d: f64[64, 64] = empty((64, 64), dtype=float64)
    for i in range(64):
        for j in range(64):
            array2d[i,j]= float(64*i + j - 2048)

    arctan2d = arctan(array2d) ** 2.0
    verify_arctan_2d(array2d, arctan2d, 64, 64)

def elemental_trig_identity():
    i: i32
    eps: f32
    eps = f32(1e-6)
    array1d: f32[201] = empty(201, dtype=float32)
    observed1d: f32[201] = empty(201, dtype=float32)
    for i in range(201):
        array1d[i] =  f32((i - 100)/100)

    observed1d = arcsin(array1d) + arccos(array1d)
    for i in range(201):
        assert abs(observed1d[i] - f32(pi / 2.0)) <= eps

def elemental_reverse():
    i: i32
    array1d: f32[201] = empty(201, dtype=float32)
    observed1d: f32[201] = empty(201, dtype=float32)
    for i in range(201):
        array1d[i] =  f32((i - 100)/100)
    observed1d = sin(arcsin(array1d))
    verify1d_same(observed1d, array1d, 201)

    observed1d = cos(arccos(array1d))
    verify1d_same(observed1d, array1d, 201)

    observed1d = tan(arctan(array1d))
    verify1d_same(observed1d, array1d, 201)

    observed1d = degrees(radians(array1d))
    verify1d_same(observed1d, array1d, 201)

def elemental_trig_identity_extra():
    i: i32
    array1d: f32[201] = empty(201, dtype=float32)
    array_x: f32[201] = empty(201, dtype=float32)
    array_y: f32[201] = empty(201, dtype=float32)
    for i in range(201):
        array1d[i] =  f32((i - 100)/100)
    array_x = sin(arccos(array1d))
    array_y = cos(arcsin(array1d))
    for i in range(201):
        array1d[i] =  f32(1.0) - array1d[i] ** f32(2.0)
    array1d = sqrt(array1d)
    verify1d_same(array_x, array_y, 201)
    verify1d_same(array_x, array1d, 201)

def elemental_degrees():
    i: i32
    j: i32
    eps_32: f32
    eps_64: f64
    eps_32 = f32(1e-6)
    eps_64 = 1e-12
    array1d: f32[200] = empty(200, dtype=float32)
    degrees1d: f32[200] = empty(200, dtype=float32)
    for i in range(200):
        array1d[i] =  f32(i)
    degrees1d = sin(degrees(array1d))

    for i in range(200):
        assert abs(degrees1d[i] - sin(degrees(array1d[i]))) <= eps_32

    array2d: f64[64, 64] = empty((64, 64), dtype=float64)
    degrees2d: f64[64, 64] = empty((64, 64), dtype=float64)
    for i in range(64):
        for j in range(64):
            array2d[i,j]= float(i*64+j)
    degrees2d = sin(degrees(array2d))
    for i in range(64):
        for j in range(64):
            assert abs(degrees2d[i, j] - sin(degrees(array2d[i, j]))) <= eps_64

def elemental_radians():
    i: i32
    j: i32
    eps_32: f32
    eps_64: f64
    eps_32 = f32(1e-6)
    eps_64 = 1e-12
    array1d: f32[200] = empty(200, dtype=float32)
    radians1d: f32[200] = empty(200, dtype=float32)
    for i in range(200):
        array1d[i] =  f32(i)
    radians1d = cos(radians(array1d))

    for i in range(200):
        assert abs(radians1d[i] - cos(radians(array1d[i]))) <= eps_32

    array2d: f64[64, 64] = empty((64, 64), dtype=float64)
    radians2d: f64[64, 64] = empty((64, 64), dtype=float64)
    for i in range(64):
        for j in range(64):
            array2d[i,j]= float(i*64+j)
    radians2d = cos(radians(array2d))
    for i in range(64):
        for j in range(64):
            assert abs(radians2d[i, j] - cos(radians(array2d[i, j]))) <= eps_64


elemental_arcsin()
elemental_arccos()
elemental_arctan()
elemental_degrees()
elemental_radians()
elemental_trig_identity()
elemental_reverse()
elemental_trig_identity_extra()
