from numpy import array
from lpython import i32, f64, lpython, TypeVar

n = TypeVar("n")

@lpython
def multiply(n: i32, x: f64[:]) -> f64[n]:
    i: i32
    for i in range(n):
        x[i] *= 5.0
    return x

def test():
    size: i32 = 5
    x: f64[5] = array([11.0, 12.0, 13.0, 14.0, 15.0])
    y: f64[5] = multiply(size, x)
    assert y[2] == 65.

test()
