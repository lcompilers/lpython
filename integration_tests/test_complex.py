from ltypes import i32, i64, f32, f64, c32, c64

def test_real_imag():
    x: c64
    x = c64(2) + 3j
    a: f64
    b: f64
    eps: f64
    eps = 1e-12
    a = x.real
    b = x.imag
    assert abs(a - 2.0) < eps
    assert abs(b - 3.0) < eps

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
    x = complex(a, -a) # (f64, f64)

    assert abs(x.real - 534.60000000000002274) < eps
    assert abs(x.imag - (-534.60000000000002274)) < eps

    a2: f32
    a2 = -f32(423.5430806348152437)
    a3: f32
    a3 = f32(34.5)
    x2: c32
    x2 = c32(complex(a2, a3)) # (f32, f32)

    assert f64(abs(x2.imag - f32(34.5))) < eps

    i1: i32
    i1 = -5
    i2: i64
    i2 = -i64(6)

    x = complex(a3, a) # (f32, f64)
    x = complex(a, a3) # (f64, f32)
    x = complex(i1, i2) # (i32, i64)
    x = complex(i1, -i1) # (i32, i32)
    x = complex(-i2, -i2) # (i64, i64)
    x = complex(i2, -i1) # (i64, i32)


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

def test_complex_unary_minus():
    c: c32
    c = c32(complex(3, 4.5))
    _c: c32
    _c = -c
    assert abs(f64(_c.real) - (-3.0)) < 1e-12
    assert abs(f64(_c.imag) - (-4.5)) < 1e-12
    _c = c32(complex(5, -78))
    _c = -_c
    assert abs(f64(_c.real) - (-5.0)) < 1e-12
    assert abs(f64(_c.imag) - 78.0) < 1e-12
    c2: c64
    c2 = complex(-4.5, -7.8)
    c2 = -c2
    assert abs(c2.real - 4.5) < 1e-12
    assert abs(c2.imag - 7.8) < 1e-12
    c2 = c64(3) + 4j
    c2 = -c2
    assert abs(c2.real - (-3.0)) < 1e-12
    assert abs(c2.imag - (-4.0)) < 1e-12

def test_complex_not():
    c: c32
    c = c32(complex(4, 5))
    b: bool
    b = not c
    assert not b

    c2: c64
    c2 = complex(0, 0)
    b = not c2
    assert b

def check():
    test_real_imag()
    test_complex()
    test_complex_abs()
    test_complex_binop_32()
    test_complex_binop_64()
    test_complex_unary_minus()
    test_complex_not()

check()
