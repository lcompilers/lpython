from ltypes import f64

def test_int():
    f: f64
    f = 5.678
    assert int(f) == 5
    f = -183745.23
    assert int(f) == -183745
    assert int(5.5) == 5
    assert int(-5.5) == -5
