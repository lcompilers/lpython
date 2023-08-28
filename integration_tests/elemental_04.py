from lpython import i32, f32, f64
from numpy import empty, log, log10, log2, reshape, int32, float32, float64
from math import exp

def elemental_log():
    array: f64[100] = empty(100, dtype=float64)
    observed: f64[100] = empty(100, dtype=float64)
    i: i32
    eps: f64
    eps = 1e-12

    for i in range(100):
        array[i] = float(i + 1)

    observed = log(array)

    for i in range(100):
        assert abs(exp(observed[i]) - f64(i + 1)) <= eps

def verify(observed: f32[:], base: i32, eps: f32):
    k: i32
    i: i32
    j: i32

    for k in range(100):
        i = i32(k/10)
        j = (k - i*10)
        assert abs(f32(base)**(observed[k]) - f32(i + j + 1)) <= eps

def elemental_log2_log10():
    array: f32[10, 10] = empty((10, 10), dtype=float32)
    observed: f32[100] = empty(100, dtype=float32)
    shape: i32[1] = empty(1, dtype=int32)
    i: i32
    j: i32
    eps: f32
    eps = f32(5e-6)

    for i in range(10):
        for j in range(10):
            array[i, j] = f32(i + j + 1)

    shape[0] = 100
    observed = reshape(log2(array), shape)

    verify(observed, 2, eps)

    observed = reshape(log10(array), shape)

    verify(observed, 10, eps)

elemental_log()
elemental_log2_log10()
