from numpy import array
from lpython import i32, f64, lpython

@lpython("c", ["-ffast-math", "-funroll-loops", "-O3"])
def fast_sum(n: i32, x: f64[:]) -> f64:
    s: f64 = 0.0
    i: i32
    for i in range(n):
        s += x[i]
    return s

def test():
    x: f64[3] = array([1.0, 2.0, 3.0])
    assert fast_sum(3, x) == 6.0

test()
