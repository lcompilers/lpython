from lpython import i32, f32, TypeVar
from numpy import empty, sqrt, float32

n = TypeVar("n")

def modify(array_a: f32[:], n: i32) -> f32[n]:
    return sqrt(array_a)

def verify(array_a: f32[:], array_b: f32[:], result: f32[:], size: i32):
    i: i32
    eps: f32
    eps = f32(1e-6)

    for i in range(size):
        assert abs(array_a[i] * array_a[i] + sqrt(array_b[i]) - result[i]) <= eps

def f():
    i: i32
    j: i32

    array_a: f32[256] = empty(256, dtype=float32)
    array_b: f32[256] = empty(256, dtype=float32)
    array_c: f32[256] = empty(256, dtype=float32)

    for i in range(256):
        array_a[i] = f32(i)

    for j in range(256):
        array_b[j] = f32(j + 5)

    array_c = array_a**f32(2) + modify(array_b, 256)
    verify(array_a, array_b, array_c, 256)


f()
