from ltypes import f64

def test_abs():
    x: f64
    x = 5.5
    assert abs(x) == 5.5
    x = -5.5
    assert abs(x) == 5.5
    assert abs(5.5) == 5.5
    assert abs(-5.5) == 5.5


test_abs()
