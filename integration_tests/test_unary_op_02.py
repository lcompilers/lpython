from lpython import i32, f64, c32

def f():
    eps: f64
    eps = 1e-12

    i: i32
    i = -67
    assert +i == -67

    f: f64
    f = -67.6457
    assert abs(+f + 67.6457) < eps

    c: c32
    c = c32(4) - c32(5j)
    c = +c
    assert abs(f64(c.real) - 4.000000) < eps
    assert abs(f64(c.imag) - (-5.000000)) < eps

    assert +True == 1
    b: bool
    b = False
    assert -b == 0

f()
