from ltypes import f64, c64

def test_real_imag():
    x: c64
    x = 2 + 3j
    a: f64
    b: f64
    eps: f64
    eps = 1e-12
    a = x.real
    b = x.imag
    assert abs(a - 2) < eps
    assert abs(b - 3) < eps

def test_complex():
    x: c64
    x = complex(4.5, 6.7)
    eps: f64
    eps = 1e-12
    assert abs(x.real - 4.5) < eps
    assert abs(x.imag - 6.7) < eps

    x = complex(-4, 2)
    assert abs(x.real - (-4.0)) < eps
    assert abs(x.imag - 2.0) < eps

    x = complex(4, 7.89)
    assert abs(x.real - 4.0) < eps
    assert abs(x.imag - 7.89) < eps

    x = complex(5.6, 0)
    assert abs(x.real - 5.6) < eps
    assert abs(x.imag - 0.0) < eps

    a: f64
    a = 534.6
    x = complex(a, -a)
    print(x)
    print(x.real)
    print(x.imag)


def test_complex_abs():
    x: c32
    x = complex(3, 4)
    eps: f64
    eps = 1e-12
    assert abs(abs(x) - 5.0) < eps
    y: c64
    y = complex(6, 8)
    assert abs(abs(y) - 10.0) < eps

def test_complex_binop_32():
    x: c32
    y: c32
    z: c32
    x = 2 + 3j
    y = 4 + 5j
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
    x = 2 + 3j
    y = 4 + 5j
    z = x + y
    z = x - y
    z = x * y
    # TODO:
    #z = x / y
    z = x ** y

def check():
    test_real_imag()
    test_complex()
    test_complex_abs()
    test_complex_binop_32()
    test_complex_binop_64()

check()
