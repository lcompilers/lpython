from lpython import i32, f64, c32, c64
from numpy import empty, int32, float64, complex64, complex128

def main0():
    x: i32[4, 5, 2] = empty([4, 5, 2], dtype=int32)
    y: f64[24, 100, 2, 5] = empty([24, 100, 2, 5], dtype=float64)
    print(x.size)
    print(y.size)

    assert x.size == 40
    assert y.size == 24000

def main1():
    a: c32[12] = empty([12], dtype=complex64)
    b: c64[15, 15, 10] = empty([15, 15, 10], dtype=complex128)
    print(a.size)
    print(b.size)

    assert a.size == 12
    assert b.size == 2250

main0()
main1()
