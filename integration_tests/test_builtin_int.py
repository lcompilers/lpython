from ltypes import f64

def test_int():
    f: f64
    f = 5.678
    assert int(5.678) == 5
    assert int(f) == 5
    f = -183745.23
    assert int(-183745.23) == -183745
    assert int(f) == -183745
    assert int(5.5) == 5
    assert int(-5.5) == -5


def test_issue254():
    i: f64
    j: i32
    i = 300.27
    j = int(i)
    assert j == 300


test_int()
test_issue254()
