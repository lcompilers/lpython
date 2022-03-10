from ltypes import i32, f64

def test_pow():
    # TODO: the commented tests should also work
    a: i32
    b: i32
    eps: f64
    eps = 1e-12
    a = 2
    b = 5
    assert pow(a, b) == 32
    a = 6
    b = 3
    assert pow(a, b) == 216
    a = 2
    b = 0
    assert pow(a, b) == 1
    a = 2
    b = -1
    assert abs(pow(2, -1) - 0.5) < eps
    # assert abs(pow(a, b) - 0.5) < eps
    a = 6
    b = -4
    assert abs(pow(6, -4) - 0.0007716049382716049) < eps
    # assert abs(pow(a, b) - 0.0007716049382716049) < eps
    assert abs(pow(-3, -5) + 0.00411522633744856) < eps
    assert abs(pow(6, -4) - 0.0007716049382716049) < eps


test_pow()
