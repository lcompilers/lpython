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

test_real_imag()
