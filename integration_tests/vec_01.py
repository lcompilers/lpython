from lpython import f64, i32
from numpy import empty, float64

def loop_vec():
    a: f64[9216] = empty(9216, dtype=float64)
    b: f64[9216] = empty(9216, dtype=float64)
    i: i32

    for i in range(9216):
        b[i] = 5.0

    for i in range(9216):
        a[i] = b[i]

    for i in range(9216):
        assert a[i] == 5.0

loop_vec()
