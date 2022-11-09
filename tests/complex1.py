from ltypes import c32, c64

def test_complex():
    c: c32
    c1: c32
    c2: c32
    c3: c64
    b: bool

    # constant real or int as args
    c = c32(complex())
    c = c32(complex(3.4))
    c = c32(complex(5., 4.3))
    c = c32(complex(1))
    c1 = c32(complex(3, 4))
    c2 = c32(complex(2, 4.5))
    c3 = complex(3., 4.)
    b = c1 != c2
    b = c64(c1) == c3

    # binary ops
    c = c1 + c2
    c = c2 - c1
    c = c1 * c2
    c = c32(complex(1, 2) ** complex(3.34534, 4.8678678))
    c = c32(complex(1, 2) * complex(3, 4))
    c = c32(complex(4, 5) - complex(3, 4))


def test():
    x: c64
    y: c64
    z: c32
    x = c64(2) + 3j
    y = c64(5) + 5j
    z = c32(x + y)
    z = c32(x - y)
    z = c32(c64(2) * x)
