from ltypes import f64
from numpy import empty

def loop_vec():
    a: f64[9216] = empty(9216)
    b: f64[9216] = empty(9216)
    i: i32

    for i in range(9216):
        b[i] = 5.0

    for i in range(9216):
        a[i] = b[i]

    for i in range(9216):
        assert a[i] == 5.0

loop_vec()
