from lpython import i32, f64, c32, c64
from numpy import empty

def main0():
    x: i32[4, 5, 2] = empty([4, 5, 2])
    y: f64[24, 100, 2, 5] = empty([24, 100, 2, 5])
    print(x.size)
    print(y.size)

    assert x.size == 40
    assert y.size == 24000

def main1():
    a: c32[12] = empty([12])
    b: c64[15, 15, 10] = empty([15, 15, 10])
    print(a.size)
    print(b.size)

    assert a.size == 12
    assert b.size == 2250

main0()
main1()
