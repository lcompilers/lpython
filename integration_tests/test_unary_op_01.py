from lpython import i32, f64

def f():
    eps: f64
    eps = 1e-12

    i: i32
    i = -67
    assert -i == 67

    f: f64
    f = -67.6457
    assert abs(-f - 67.6457) < eps

f()
