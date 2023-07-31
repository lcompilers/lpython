from numpy import array
from lpython import i32, i64, f64, lpython, TypeVar, Const

n = TypeVar("n")

@lpython(backend="c", backend_optimisation_flags=["-ffast-math", "-funroll-loops"])
def multiply_01(n: i32, x: f64[:]) -> f64[n]:
    i: i32
    for i in range(n):
        x[i] *= 5.0
    return x

@lpython
def multiply_02(n: i32, x: i64[:], y: i64[:]) -> i64[n]:
    z: i64[n]; i: i32
    for i in range(n):
        z[i] = x[i] * y[i]
    return z

@lpython
def const_arg_sum(x: Const[i32]) -> i32:
    return 10 + x


def test_01():
    size = 5
    x = array([11.0, 12.0, 13.0, 14.0, 15.0])
    y = multiply_01(size, x)
    assert y[2] == 65.

    size = 3
    x = array([11, 12, 13])
    y = array([14, 15, 16])
    z = multiply_02(size, x, y)
    for i in range(size):
        assert z[i] == x[i] * y[i]
    assert const_arg_sum(size) == size + 10

test_01()
