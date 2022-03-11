from ltypes import f64

def test_int():
    # TODO: the commented tests should also work
    f: f64
    f = 5.678
    assert int(5.678) == 5
    # assert int(f) == 5
    f = -183745.23
    assert int(-183745.23) == -183745
    # assert int(f) == -183745
    assert int(5.5) == 5
    assert int(-5.5) == -5

test_int()
