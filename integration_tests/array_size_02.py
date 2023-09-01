from lpython import i32, f64, c32, c64, u32, u64
from numpy import empty, size, int32, uint32, uint64, float64, complex64, complex128

def main0():
    x: i32[4, 5, 2] = empty([4, 5, 2], dtype=int32)
    y: f64[24, 100, 2, 5] = empty([24, 100, 2, 5], dtype=float64)
    z: i32
    w: i32
    z = 2
    w = 3
    print(size(x))
    print(size(x, 0))
    print(size(x, 1))
    print(size(x, 2))
    print(size(y))
    print(size(y, 0))
    print(size(y, 1))
    print(size(y, z))
    print(size(y, w))

    assert size(x) == 40
    assert size(x, 0) == 4
    assert size(x, 1) == 5
    assert size(x, 2) == 2
    assert size(y) == 24000
    assert size(y, 0) == 24
    assert size(y, 1) == 100
    assert size(y, z) == 2
    assert size(y, w) == 5

def main1():
    a: c32[12] = empty([12], dtype=complex64)
    b: c64[15, 15, 10] = empty([15, 15, 10], dtype=complex128)
    c: i32
    d: i32
    c = 1
    d = 2
    print(size(a))
    print(size(a, 0))
    print(size(b))
    print(size(b, 0))
    print(size(b, c))
    print(size(b, d))

    assert size(a) == 12
    assert size(a, 0) == 12
    assert size(b) == 2250
    assert size(b, 0) == 15
    assert size(b, c) == 15
    assert size(b, d) == 10

def main2():
    a: i32[2, 3] = empty([2, 3], dtype=int32)
    print(size(a))
    print(size(a, 0))
    print(size(a, 1))

    assert size(a) == 2*3
    assert size(a, 0) == 2
    assert size(a, 1) == 3

def main3():
    a: u32[2, 3, 4] = empty([2, 3, 4], dtype=uint32)
    b: u64[10, 5] = empty([10, 5], dtype=uint64)
    c: i32
    d: i32
    c = 1
    d = 2
    print(size(a))
    print(size(a, 0))
    print(size(a, c))
    print(size(a, d))

    print(size(b))
    print(size(b, 0))
    print(size(b, c))

    assert size(a) == 2*3*4
    assert size(a, 0) == 2
    assert size(a, c) == 3
    assert size(a, d) == 4

    assert size(b) == 50
    assert size(b, 0) == 10
    assert size(b, c) == 5

main0()
main1()
main2()
main3()
