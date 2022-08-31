from ltypes import i32, f32, f64
from numpy import empty, arcsin, arccos, sin, cos, sqrt
from math import pi

def verify1d_same(array: f32[:], result: f32[:], size: i32):
    i: i32
    eps: f32
    eps = 1e-6
    for i in range(size):
        assert abs(array[i] - result[i]) <= eps

def verify_arcsin_1d(array: f32[:], result: f32[:], size: i32):
    i: i32
    eps: f32
    eps = 1e-6
    for i in range(size):
        assert abs(arcsin(array[i])**2 - result[i]) <= eps

def verify_arcsin_2d(array: f64[:, :], result: f64[:, :], size1:i32, size2:i32):
    i: i32
    j: i32
    eps: f64
    eps = 1e-12
    for i in range(size1):
        for j in range(size2):
            assert abs(arcsin(array[i, j])**2 - result[i, j]) <= eps

def verify_arccos_1d(array: f32[:], result: f32[:], size: i32):
    i: i32
    eps: f32
    eps = 1e-6
    for i in range(size):
        assert abs(arccos(array[i])**2 - result[i]) <= eps

def verify_arccos_2d(array: f64[:, :], result: f64[:, :], size1:i32, size2:i32):
    i: i32
    j: i32
    eps: f64
    eps = 1e-12
    for i in range(size1):
        for j in range(size2):
            assert abs(arccos(array[i, j])**2 - result[i, j]) <= eps

def elemental_arcsin():
    i: i32
    j: i32
    array1d: f32[201] = empty(201)
    arcsin1d: f32[201] = empty(201)
    for i in range(201):
        array1d[i] =  float((i - 100)/100)
    arcsin1d = arcsin(array1d) ** 2
    verify_arcsin_1d(array1d, arcsin1d, 201)

    array2d: f64[64, 64] = empty((64, 64))
    arcsin2d: f64[64, 64] = empty((64, 64))
    for i in range(64):
        for j in range(64): # 2048 = 64 * 32
            array2d[i,j]= float((i * 64 + j - 2048 )/2048)

    arcsin2d = arcsin(array2d) ** 2
    verify_arcsin_2d(array2d, arcsin2d, 64, 64)

def elemental_arccos():
    i: i32
    j: i32
    array1d: f32[201] = empty(201)
    arccos1d: f32[201] = empty(201)
    for i in range(201):
        array1d[i] =  float((i - 100)/100)
    arccos1d = arccos(array1d) ** 2
    verify_arccos_1d(array1d, arccos1d, 201)

    array2d: f64[64, 64] = empty((64, 64))
    arccos2d: f64[64, 64] = empty((64, 64))
    for i in range(64):
        for j in range(64): # 2048 = 64 * 32
            array2d[i,j]= float((i * 64 + j - 2048 )/2048)

    arccos2d = arccos(array2d) ** 2
    verify_arccos_2d(array2d, arccos2d, 64, 64)

def elemental_trig_identity():
    i: i32
    eps: f32
    eps = 1e-6
    array1d: f32[201] = empty(201)
    observed1d: f32[201] = empty(201)
    for i in range(201):
        array1d[i] =  float((i - 100)/100)

    observed1d = arcsin(array1d) + arccos(array1d)
    for i in range(201):
        assert abs(observed1d[i] - pi / 2) <= eps

def elemental_reverse():
    i: i32
    array1d: f32[201] = empty(201)
    observed1d: f32[201] = empty(201)
    for i in range(201):
        array1d[i] =  float((i - 100)/100)
    observed1d = sin(arcsin(array1d))
    verify1d_same(observed1d, array1d, 201)

    observed1d = cos(arccos(array1d))
    verify1d_same(observed1d, array1d, 201)

def elemental_trig_identity_extra():
    i: i32
    array1d: f32[201] = empty(201)
    array_x: f32[201] = empty(201)
    array_y: f32[201] = empty(201)
    for i in range(201):
        array1d[i] =  float((i - 100)/100)
    array_x = sin(arccos(array1d))
    array_y = cos(arcsin(array1d))
    for i in range(201):
        array1d[i] =  1 - array1d[i] ** 2
    array1d = sqrt(array1d)
    verify1d_same(array_x, array_y, 201)
    verify1d_same(array_x, array1d, 201)


elemental_arcsin()
elemental_arccos()
elemental_trig_identity()
elemental_reverse()
elemental_trig_identity_extra()
