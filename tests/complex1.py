def test_complex():
    c: c128
    c1: c128
    c2: c128
    c3: c128
    b: bool

    # constant real or int as args
    c = complex()
    c = complex(3.4)
    c = complex(5., 4.3)
    c = complex(1)
    c1 = complex(3, 4)
    c2 = complex(2, 4.5)
    c3 = complex(3., 4.)
    c3 = complex(-3., 4.)
    b = c1 != c2
    b = c1 == c3

    # binary ops
    c = c1 + c2
    c = c2 - c1
    c = c1 * c2
    c = complex(1, 2) ** complex(3.34534, 4.8678678)
    c = complex(1, 2) * complex(3, 4)
    c = complex(4, 5) - complex(3, 4)
