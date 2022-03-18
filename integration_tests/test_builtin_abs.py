from ltypes import f32, f64, i32

def test_abs():
    x: f64
    x = 5.5
    assert abs(x) == 5.5
    x = -5.5
    assert abs(x) == 5.5
    assert abs(5.5) == 5.5
    assert abs(-5.5) == 5.5

    x2: f32
    x2 = -5.5
    assert abs(x2) == 5.5

    i: i32
    i = -5
    assert abs(i) == 5
    assert abs(-1) == 1

    b: bool
    b = True
    assert abs(b) == 1
    b = False
    assert abs(b) == 0

test_abs()
