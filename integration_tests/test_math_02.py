from math import sin, pi

def test_sin():
    # TODO: importing from `math` doesn't work here yet:
    pi: f64 = 3.141592653589793238462643383279502884197
    eps: f64 = 1e-12
    assert abs(sin(0.0)-0) < eps
    assert abs(sin(pi/2)-1) < eps

test_sin()
