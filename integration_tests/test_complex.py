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
    # TODO: below test should work
    # assert abs(b - 3) < eps

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

def check():
    test_real_imag()
    test_complex()

check()
