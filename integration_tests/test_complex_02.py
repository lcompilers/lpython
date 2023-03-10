from ltypes import f64, c32, c64

def test_complex_abs():
    x: c32
    x = c32(complex(3, 4))
    eps: f64
    eps = 1e-12
    assert f64(abs(f64(abs(x)) - 5.0)) < eps
    y: c64
    y = complex(6, 8)
    assert abs(abs(y) - 10.0) < eps

def test_complex_binop_32():
    x: c32
    y: c32
    z: c32
    x = c32(c64(2) + 3j)
    y = c32(c64(4) + 5j)
    z = x + y
    z = x - y
    z = x * y
    # TODO:
    #z = x / y
    z = x ** y

def test_complex_binop_64():
    x: c64
    y: c64
    z: c64
    x = c64(2) + 3j
    y = c64(4) + 5j
    z = x + y
    z = x - y
    z = x * y
    # TODO:
    #z = x / y
    z = x ** y

def check():
    test_complex_abs()
    test_complex_binop_32()
    test_complex_binop_64()

check()
