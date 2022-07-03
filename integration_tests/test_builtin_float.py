from ltypes import i32, f64

def test_float():
    i: i32
    i = 34
    assert float() == 0.0
    assert float(34) == 34.0
    assert float(i) == 34.0
    i = -4235
    assert float(-4235) == -4235.0
    assert float(34) == 34.0
    assert float(-4235) == -4235.0
    assert float(True) == 1.0
    assert float(False) == 0.0
    b: bool
    b = True
    f: f64
    f = float(b)
    assert f == 1.0

test_float()
